#include "Beam.h"

#include <iomanip>

#include "dicom/DicomUtils.h"


// ---- constructor ----
Beam::Beam(DcmItem* beamItem)
{
    using namespace dicom;
    leafPairs = 60;

    // Identity
    getInt(beamItem, DCM_BeamNumber, beamNumber);

    {
        std::string s;
        if(getString(beamItem, DCM_BeamName, s)) beamName = s;
        if(getString(beamItem, DCM_BeamType, s)) beamType = s;
        if(getString(beamItem, DCM_RadiationType, s)) radiationType = s;
        if(getString(beamItem, DCM_TreatmentDeliveryType, s)) treatmentDeliveryType = s;
    }

    // Machine / SAD / dosimeter unit
    {
        std::string s;
        if(getString(beamItem, DCM_TreatmentMachineName, s)) treatmentMachineName = s;
        if(getString(beamItem, DCM_PrimaryDosimeterUnit, s)) primaryDosimeterUnit = s;

        double d;
        if(getDouble(beamItem, DCM_SourceAxisDistance, d)) sourceAxisDistanceMm = d;
        if(getDouble(beamItem, DCM_FinalCumulativeMetersetWeight, d)) finalCumulativeMetersetWeight = d;
    }

    // Number of CPs
    getInt(beamItem, DCM_NumberOfControlPoints, numberOfControlPoints);

    // Parse ControlPointSequence
    DcmSequenceOfItems* cpSeq = nullptr;
    if(beamItem &&
       beamItem->findAndGetSequence(DCM_ControlPointSequence, cpSeq).good() &&
       cpSeq)
    {
        controlPoints.clear();
        controlPoints.reserve(static_cast<size_t>(cpSeq->card()));

        for(unsigned long i = 0; i < cpSeq->card(); ++i)
        {
            DcmItem* cpItem = cpSeq->getItem(i);
            if(!cpItem) continue;

            ControlPoint cp(cpItem);
            controlPoints.push_back(std::move(cp));
        }

        // Optional robustness: inherit missing aperture/geometry from previous CP
        // (useful if vendor omits repeated values)
        for(size_t i = 1; i < controlPoints.size(); ++i)
        {
            auto& cur = controlPoints[i];
            const auto& prev = controlPoints[i-1];

            if(!cur.jawX && prev.jawX) cur.jawX = prev.jawX;
            if(!cur.jawY && prev.jawY) cur.jawY = prev.jawY;

            if(!cur.isocenterMm && prev.isocenterMm) cur.isocenterMm = prev.isocenterMm;
            if(!cur.ssdMm && prev.ssdMm) cur.ssdMm = prev.ssdMm;

            if(!cur.gantryAngleDeg && prev.gantryAngleDeg) cur.gantryAngleDeg = prev.gantryAngleDeg;
            if(!cur.collimatorAngleDeg && prev.collimatorAngleDeg) cur.collimatorAngleDeg = prev.collimatorAngleDeg;
            if(!cur.couchAngleDeg && prev.couchAngleDeg) cur.couchAngleDeg = prev.couchAngleDeg;

            if(cur.mlcA.empty() && !prev.mlcA.empty()) cur.mlcA = prev.mlcA;
            if(cur.mlcB.empty() && !prev.mlcB.empty()) cur.mlcB = prev.mlcB;
        }
    }
}

void Beam::print(std::ostream& os) const
{
    auto optS = [&](const char* k, const std::optional<std::string>& v){
        os << "  " << std::left << std::setw(26) << k << ": " << (v ? *v : "<missing>") << "\n";
    };
    auto optD = [&](const char* k, const std::optional<double>& v){
        os << "  " << std::left << std::setw(26) << k << ": " << (v ? std::to_string(*v) : "<missing>") << "\n";
    };

    os << "Beam #" << beamNumber << "\n";
    optS("BeamName", beamName);
    optS("BeamType", beamType);
    optS("RadiationType", radiationType);
    optS("TreatmentDeliveryType", treatmentDeliveryType);
    optS("TreatmentMachineName", treatmentMachineName);
    optS("PrimaryDosimeterUnit", primaryDosimeterUnit);
    optD("SAD (mm)", sourceAxisDistanceMm);
    optD("Final CMW", finalCumulativeMetersetWeight);

    os << "  " << std::left << std::setw(26) << "ControlPoints" << ": " << controlPoints.size() << "\n";

    if(beamMetersetMU) os << "  BeamMeterset (MU): " << beamMetersetMU << "\n";
    if(beamDoseGy)     os << "  BeamDose (Gy): " << beamDoseGy << "\n";
    if(true) {
        os << "  BeamDoseSpecPoint (mm): ["
           << (beamDoseSpecPointMm)[0] << ", "
           << (beamDoseSpecPointMm)[1] << ", "
           << (beamDoseSpecPointMm)[2] << "]\n";
    }
}

void Beam::storeFractionSequence(DcmItem* item)
{
    using namespace dicom; 
    int refBeamNo;
    getInt(item, DCM_ReferencedBeamNumber, refBeamNo);
    if(refBeamNo != beamNumber)
    {
        std::cout << "Beam Number " << beamNumber
                  << "Fraction group beam : " << refBeamNo;

        return;
    }

    getDouble(item, DCM_BeamMeterset, beamMetersetMU);
    getDouble(item, DCM_BeamDose, beamDoseGy);
    getDouble3(item, DCM_BeamDoseSpecificationPoint, beamDoseSpecPointMm);
}
