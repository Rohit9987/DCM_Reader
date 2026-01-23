#include "RtPlanReader.h"
#include "ControlPoint.h"

#include <dcmtk/dcmdata/dctk.h>
#include <dcmtk/dcmdata/dcuid.h>
#include <dcmtk/dcmdata/dcdeftag.h>


#include "Beam.h"

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

static bool getFloat64_3(DcmItem* it, const DcmTagKey& key, double out3[3])
{
    if(!it) return false;

    Float64 v0, v1, v2;
    if(it->findAndGetFloat64(key, v0, 0).good() &&
       it->findAndGetFloat64(key, v1, 1).good() &&
       it->findAndGetFloat64(key, v2, 2).good())
    {
        out3[0] = v0; out3[1] = v1; out3[2] = v2;
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
                            Plan& outPlan,
                            std::string& errorMessage)
{
    outPlan = Plan{};
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


    // Basic identifiers --> all of this needs to go into a a struct
    getOFString(ds, DCM_PatientName, outPlan.patientName);
    getOFString(ds, DCM_PatientID, outPlan.patientId);
    getOFString(ds, DCM_StudyInstanceUID, outPlan.studyInstanceUid);
    getOFString(ds, DCM_SeriesInstanceUID, outPlan.seriesInstanceUid);
    getOFString(ds, DCM_SOPInstanceUID, outPlan.sopInstanceUid);

    // Plan label/name (both exists in the standard; exporters vary)
    getOFString(ds, DCM_RTPlanLabel, outPlan.rtPlanLabel);
    getOFString(ds, DCM_RTPlanName, outPlan.rtPlanName);

    // FractionGroupSequence (300A, 0070)

    DcmSequenceOfItems* beamSeq = nullptr;
    if(ds->findAndGetSequence(DCM_BeamSequence, beamSeq).bad() || !beamSeq)
    {
        // some plans (e.g., setup only exports) may still have beams, but if not present, that's okay
        return true;
    }
    
    const unsigned long nBeams = beamSeq->card();
    

    for(unsigned long i = 0; i < nBeams; ++i)       
    {
        DcmItem* beamItem = beamSeq->getItem(i);
        if(!beamItem) continue;

        Beam b(beamItem);
        outPlan.beams.push_back(b);
        //b.print();
    }

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
            DcmSequenceOfItems* refBeamSeq = nullptr;
            if(item0->findAndGetSequence(DCM_ReferencedBeamSequence, refBeamSeq).good() && refBeamSeq)
            {
                for(unsigned long rb = 0; rb < refBeamSeq->card(); ++rb)
                {
                    DcmItem* rbItem = refBeamSeq->getItem(rb);
                    if(!rbItem) continue;

                    outPlan.beams[rb].storeFractionSequence(rbItem);
                }
            }
        }
    }

    std::cout << "----------------------------------------------\n";
    
    
    // TODO: completed beams and control points,
    // now move to storing all parts of the plan
    // also beam weighting needs to organized
    
    return true;
}


