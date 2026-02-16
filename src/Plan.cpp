#include "Plan.h"

#include <iostream>
#include <map>
#include <iomanip>
#include "dicom/DicomUtils.h"

Plan::Plan(DcmDataset* ds)
{
    if(!ds)
        return;

    using namespace dicom;


    // --- Patient ---
    getString(ds, DCM_PatientName, patientName);
    getString(ds, DCM_PatientID, patientId);

    // --- UIDs ---
    getString(ds, DCM_StudyInstanceUID, studyInstanceUid);
    getString(ds, DCM_SeriesInstanceUID, seriesInstanceUid);
    getString(ds, DCM_SOPInstanceUID, sopInstanceUid);
    getString(ds, DCM_FrameOfReferenceUID, frameOfReferenceUid);

    // --- Plan identity ---
    getString(ds, DCM_RTPlanLabel, rtPlanLabel);
    getString(ds, DCM_RTPlanName, rtPlanName);

    {
        std::string s;
        if(getString(ds, DCM_RTPlanDescription, s))
            rtPlanDescription = s;
        if(getString(ds, DCM_RTPlanGeometry, s))
            rtPlanGeometry = s;
        if(getString(ds, DCM_ApprovalStatus, s))
            approvalStatus = s;
        if(getString(ds, DCM_RTPlanDate, s))
            rtPlanDate = s;
        if(getString(ds, DCM_RTPlanTime, s))
            rtPlanTime = s;
    }

    {
        DcmSequenceOfItems* seq = nullptr;
        if(ds->findAndGetSequence(DCM_ReferencedStructureSetSequence, seq).good() && seq && seq->card() > 0)
        {
            DcmItem* item = seq->getItem(0);
            std::string uid;
            if(item && getString(item, DCM_ReferencedSOPInstanceUID, uid))
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
                getInt(fgItem, DCM_FractionGroupNumber, fractionGroupNumber);
                getInt(fgItem, DCM_NumberOfFractionsPlanned, numFractionsPlanned);

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
                        info.hasSpec = getDouble3(rb, DCM_BeamDoseSpecificationPoint, info.specPoint);

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
