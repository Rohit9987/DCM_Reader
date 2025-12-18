#include "RtPlanReader.h"

#include <dcmtk/dcmdata/dctk.h>
#include <dcmtk/dcmdata/dcuid.h>
#include <dcmtk/dcmdata/dcdeftag.h>

#include <iostream>

static bool getOFString(DcmItem* ds, const DcmTagKey& key, std::string& out)
{
    OFString v;
    if (ds->findAndGetOFString(key, v).good())
    {
        out = v.c_str();
        return true;
    }
    return false;
}

static bool getSint32(DcmItem* ds, const DcmTagKey& key, int& out)
{
    Sint32 v = 0;
    if (ds->findAndGetSint32(key, v).good()) {
        out = static_cast<int>(v);
        return true;
    }
    return false;
}

static bool getFloat64(DcmItem* ds, const DcmTagKey& key, double& out)
{
    Float64 v = 0.0;
    if (ds->findAndGetFloat64(key, v).good()) {
        out = static_cast<double>(v);
        return true;
    }
    return false;
}

bool RtPlanReader::IsRtPlanSopClass(const char* sopClassUid)
{
    return sopClassUid && std::string(sopClassUid) == UID_RTPlanStorage;
}

bool RtPlanReader::GetStr(void* dataset, const unsigned short group, const unsigned short element, std::string& out)
{
    auto* ds = reinterpret_cast<DcmDataset*>(dataset);
    return getOFString(ds, DcmTagKey(group, element), out);
}

bool RtPlanReader::GetInt(void* dataset, const unsigned short group, const unsigned short element, int& out)
{
    auto* ds = reinterpret_cast<DcmDataset*>(dataset);
    return getSint32(ds, DcmTagKey(group, element), out);
}

bool RtPlanReader::GetFloat64(void* dataset, const unsigned short group, const unsigned short element, double& out)
{
    auto* ds = reinterpret_cast<DcmDataset*>(dataset);
    return getFloat64(ds, DcmTagKey(group, element), out);
}

bool RtPlanReader::ReadPlan(const std::string& rtplanPath,
                            RtPlanSummary& outPlan,
                            std::string& errorMessage)
{
    outPlan = RtPlanSummary{};
    outPlan.filePath = rtplanPath;

    DcmFileFormat ff;
    OFCondition st = ff.loadFile(rtplanPath.c_str());
    if(!st.good())
    {
        errorMessage = std::string("loadFile failed: ") +st.text();
        return false;
    }

    DcmDataset* ds = ff.getDataset();
    
    std::string sopClassUid;
    if(!getOFString(ds, DCM_SOPClassUID, sopClassUid) || !IsRtPlanSopClass(sopClassUid.c_str()))
    {
        errorMessage = "Not an RTPLAN file (SOPClassUID != RT Plan Storage).";
        return false;
    }


    // Basic identifiers
    getOFString(ds, DCM_PatientName, outPlan.patientName);
    getOFString(ds, DCM_PatientID, outPlan.patientId);
    getOFString(ds, DCM_StudyInstanceUID, outPlan.studyInstanceUid);
    getOFString(ds, DCM_SeriesInstanceUID, outPlan.seriesInstanceUid);
    getOFString(ds, DCM_SOPInstanceUID, outPlan.sopInstanceUid);

    // Plan label/name (both exists in the standard; exporters vary)
    getOFString(ds, DCM_RTPlanLabel, outPlan.rtPlanLabel);
    getOFString(ds, DCM_RTPlanName, outPlan.rtPlanName);

    // FractionGroupSequence (300A, 0070)
    DcmSequenceOfItems* seq = nullptr;
    if(ds->findAndGetSequence(DCM_FractionGroupSequence, seq).good() && seq && seq->card() > 0)
    {
        DcmItem* item0 = seq->getItem(0);
        if(item0)
        {
            int nfx = -1;
            if(getSint32(item0, DCM_NumberOfFractionsPlanned, nfx))
            {
                outPlan.numFractionsPlanned = nfx;
            }
        }
    }

    //Beam Sequence (300A, 00B0)
    //
    DcmSequenceOfItems* beamSeq = nullptr;
    if(ds->findAndGetSequence(DCM_BeamSequence, beamSeq).bad() || !beamSeq)
    {
        // some plans (e.g., setup only exports) may still have beams, but if not present, that's okay
        return true;
    }
    
    const unsigned long nBeams = beamSeq->card();
    outPlan.beams.reserve(static_cast<size_t>(nBeams));

    for(unsigned long i = 0; i < nBeams; ++i)
    {
        DcmItem* beamItem = beamSeq->getItem(i);
        if(!beamItem) continue;

        RtBeamSummary beam;

        getSint32(beamItem, DCM_BeamNumber, beam.beamNumber);
        getOFString(beamItem, DCM_BeamName, beam.beamName);
        getOFString(beamItem, DCM_BeamType, beam.beamType);
        getOFString(beamItem, DCM_RadiationType, beam.radiationType);
        getOFString(beamItem, DCM_TreatmentDeliveryType, beam.treatmentDeliveryType);

        //ControlPointSequence (300A, 0111)
        DcmSequenceOfItems* cpSeq = nullptr;
        if(beamItem->findAndGetSequence(DCM_ControlPointSequence, cpSeq).good() && cpSeq)
        {
            beam.numControlPoints = static_cast<int>(cpSeq->card());

            //Optional: sample first and last control point angles
            auto sampleCP = [&](unsigned long cpIndex)
            {
                if(cpIndex >= cpSeq->card()) return;
                DcmItem* cpItem = cpSeq->getItem(cpIndex);
                if(!cpItem) return;

                RtControlPointSummary cp;
                cp.index = static_cast<int>(cpIndex);

                getFloat64(cpItem, DCM_GantryAngle, cp.gantryAngleDeg);
                getFloat64(cpItem, DCM_BeamLimitingDeviceAngle, cp.collimatorAngleDeg);
                getFloat64(cpItem, DCM_PatientSupportAngle, cp.couchAngleDeg);

                beam.ControlPoints.push_back(cp);
            };

            if(cpSeq->card() > 0) sampleCP(0);
            if(cpSeq->card() > 1) sampleCP(cpSeq->card() - 1);
        }
        
        outPlan.beams.push_back(std::move(beam));
    }
    
    return true;
}


