#include "Plan.h"

#include <iostream>
#include <map>
#include <iomanip>


// ---------- helpers ----------
template <typename T>
static bool getOFString(T* ds, const DcmTagKey& key, std::string& out)
{
    if(!ds) return false;
    OFString s;
    if(ds->findAndGetOFString(key, s).good()) {
        out = s.c_str();
        return true;
    }
    return false;
}

template <typename T>
static bool getSint32(T* ds, const DcmTagKey& key, int& out)
{
    if(!ds) return false;
    Sint32 v;
    if(ds->findAndGetSint32(key, v).good()) {
        out = (int)v;
        return true;
    }
    return false;
}

static bool getFloat64_3(DcmItem* it, const DcmTagKey& key, std::array<double,3>& out3)
{
    double a,b,c;
    if(it->findAndGetFloat64(key,a,0).good() &&
       it->findAndGetFloat64(key,b,1).good() &&
       it->findAndGetFloat64(key,c,2).good())
    {
        out3 = {a,b,c};
        return true;
    }
    return false;
}

//TODO:
Plan::Plan(DcmDataset* ds)
{
    std::cout <<"Hello\n";
    if(!ds)
        return;

    std::cout <<"Hello\n";
    // --- Patient ---
    getOFString(ds, DCM_PatientName, patientName);
    getOFString(ds, DCM_PatientID, patientId);

    // --- UIDs ---
    getOFString(ds, DCM_StudyInstanceUID, studyInstanceUid);
    getOFString(ds, DCM_SeriesInstanceUID, seriesInstanceUid);
    getOFString(ds, DCM_SOPInstanceUID, sopInstanceUid);
    getOFString(ds, DCM_FrameOfReferenceUID, frameOfReferenceUid);

    // --- Plan identity ---
    getOFString(ds, DCM_RTPlanLabel, rtPlanLabel);
    getOFString(ds, DCM_RTPlanName, rtPlanName);

    {
        std::string s;
        if(getOFString(ds, DCM_RTPlanDescription, s))
            rtPlanDescription = s;
        if(getOFString(ds, DCM_RTPlanGeometry, s))
            rtPlanGeometry = s;
        if(getOFString(ds, DCM_ApprovalStatus, s))
            approvalStatus = s;
        if(getOFString(ds, DCM_RTPlanDate, s))
            rtPlanDate = s;
        if(getOFString(ds, DCM_RTPlanTime, s))
            rtPlanTime = s;
    }

    {
        DcmSequenceOfItems* seq = nullptr;
        if(ds->findAndGetSequence(DCM_ReferencedStructureSetSequence, seq).good() && seq && seq->card() > 0)
        {
            DcmItem* item = seq->getItem(0);
            std::string uid;
            if(item && getOFString(item, DCM_ReferencedSOPInstanceUID, uid))
                referencedStructSetSOPInstanceUid = uid;
        }
    }

    // ---- Fraction group (store MU info temporarily) ----
    struct FGBeamInfo{
        double mu = 0.0;
        double dose = 0.0;
        std::array<double, 3> specPoint{};
        bool hasSpec = false;
    };

    std::map<int, FGBeamInfo> fgInfo;

    {
        DcmSequenceOfItems* fgSeq = nullptr;
        if(ds->findAndGetSequence(DCM_FractionGroupSequence, fgSeq).good() && fgSeq && fgSeq->card() >  0)
        {
            DcmItem* fgItem = fgSeq->getItem(0);
            if(fgItem)
            {
                getSint32(fgItem, DCM_FractionGroupNumber, fractionGroupNumber);
                getSint32(fgItem, DCM_NumberOfFractionsPlanned, numFractionsPlanned);

                DcmSequenceOfItems* refBeamSeq = nullptr;
                if(fgItem->findAndGetSequence(DCM_ReferencedBeamSequence, refBeamSeq).good() && refBeamSeq)
                {
                    for(unsigned int i = 0; i < refBeamSeq->card(); i++)
                    {
                        DcmItem* rb = refBeamSeq->getItem(i);
                        if(!rb) continue;

                        int beamNum = -1;
                        rb->findAndGetSint32(DCM_ReferencedBeamNumber, beamNum);

                        FGBeamInfo info;
                        rb->findAndGetFloat64(DCM_BeamMeterset, info.mu);
                        rb->findAndGetFloat64(DCM_BeamDose, info.dose);
                        info.hasSpec = getFloat64_3(rb, DCM_BeamDoseSpecificationPoint, info.specPoint);

                        fgInfo[beamNum] = info;

                    }
                }
            }
        }
    }

    // ---- Beam Sequence ----
    {
        DcmSequenceOfItems* beamSeq = nullptr;
        if(ds->findAndGetSequence(DCM_BeamSequence, beamSeq).good() && beamSeq)
        {
            beams.reserve(beamSeq->card());
            for(unsigned long i=0; i < beamSeq->card(); i++)
            {
                DcmItem* beamItem = beamSeq->getItem(i);
                if(!beamItem) continue;

                Beam b(beamItem);

                // attach fraction group info
                auto it = fgInfo.find(b.beamNumber);
                if(it != fgInfo.end())
                {
                    b.beamMetersetMU = it->second.mu;
                    b.beamDoseGy = it->second.dose;
                    if(it->second.hasSpec)
                        b.beamDoseSpecPointMm = it->second.specPoint;

                    totalPlannedMetersetMU = 
                            (totalPlannedMetersetMU ? *totalPlannedMetersetMU : 0.0) + it->second.mu;
                }

                beams.push_back(std::move(b));
            }
        }
    }

    // ---- Primary isocenter (convenience) ----
    if(!beams.empty() && !beams.front().controlPoints.empty())
        primaryIsocenterMm = beams.front().controlPoints.front().isocenterMm;

    print();
}





void Plan::print(std::ostream& os) const
{
    os << "================ RT PLAN ================\n";

    os << "File            : " << filePath << "\n";

    os << "Patient Name    : " << patientName << "\n";
    os << "Patient ID      : " << patientId << "\n";

    os << "Study UID       : " << studyInstanceUid << "\n";
    os << "Series UID      : " << seriesInstanceUid << "\n";
    os << "SOP UID         : " << sopInstanceUid << "\n";
    os << "FrameRef UID    : " << frameOfReferenceUid << "\n";

    os << "Plan Label      : " << rtPlanLabel << "\n";
    os << "Plan Name       : " << rtPlanName << "\n";

    if(rtPlanDescription)
        os << "Description     : " << *rtPlanDescription << "\n";

    if(rtPlanGeometry)
        os << "Geometry        : " << *rtPlanGeometry << "\n";

    if(approvalStatus)
        os << "Approval Status : " << *approvalStatus << "\n";

    os << "Fraction Group  : " << fractionGroupNumber << "\n";
    os << "Fractions       : " << numFractionsPlanned << "\n";

    if(referencedStructSetSOPInstanceUid)
        os << "RTSTRUCT SOP    : " << *referencedStructSetSOPInstanceUid << "\n";

    if(primaryIsocenterMm)
    {
        const auto& iso = *primaryIsocenterMm;
        os << "Isocenter (mm)  : ["
           << iso[0] << ", " << iso[1] << ", " << iso[2] << "]\n";
    }

    if(totalPlannedMetersetMU)
        os << "Total MU        : " << *totalPlannedMetersetMU << "\n";

    os << "Number of Beams : " << beams.size() << "\n";

    os << "-----------------------------------------\n";
    for(const auto& b : beams)
    {
        b.print(os);
        os << "-----------------------------------------\n";
    }

    os << "=========================================\n";
}
