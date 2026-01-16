#include "Beam.h"

#include <iomanip>

// ---- helpers (same pattern as ControlPoint.cpp) ----
static bool getOFString(DcmItem* it, const DcmTagKey& key, std::string& out)
{
    if(!it) return false;
    OFString s;
    if(it->findAndGetOFString(key, s).good()) { out = s.c_str(); return true; }
    return false;
}

static bool getFloat64(DcmItem* it, const DcmTagKey& key, double& out, unsigned long pos = 0)
{
    if(!it) return false;
    Float64 v;
    if(it->findAndGetFloat64(key, v, pos).good()) { out = (double)v; return true; }
    return false;
}

static bool getSint32(DcmItem* it, const DcmTagKey& key, int& out, unsigned long pos = 0)
{
    if(!it) return false;
    Sint32 v;
    if(it->findAndGetSint32(key, v, pos).good()) { out = (int)v; return true; }
    return false;
}

static bool getFloat64_3(DcmItem* it, const DcmTagKey& key, std::array<double,3>& out3)
{
    double v0, v1, v2;
    if(getFloat64(it, key, v0, 0) && getFloat64(it, key, v1, 1) && getFloat64(it, key, v2, 2)) {
        out3 = {v0, v1, v2};
        return true;
    }
    return false;
}

// ---- constructor ----
Beam::Beam(DcmItem* beamItem)
{
    leafPairs = 60;

    // Identity
    getSint32(beamItem, DCM_BeamNumber, beamNumber);

    {
        std::string s;
        if(getOFString(beamItem, DCM_BeamName, s)) beamName = s;
        if(getOFString(beamItem, DCM_BeamType, s)) beamType = s;
        if(getOFString(beamItem, DCM_RadiationType, s)) radiationType = s;
        if(getOFString(beamItem, DCM_TreatmentDeliveryType, s)) treatmentDeliveryType = s;
    }

    // Machine / SAD / dosimeter unit
    {
        std::string s;
        if(getOFString(beamItem, DCM_TreatmentMachineName, s)) treatmentMachineName = s;
        if(getOFString(beamItem, DCM_PrimaryDosimeterUnit, s)) primaryDosimeterUnit = s;

        double d;
        if(getFloat64(beamItem, DCM_SourceAxisDistance, d)) sourceAxisDistanceMm = d;
        if(getFloat64(beamItem, DCM_FinalCumulativeMetersetWeight, d)) finalCumulativeMetersetWeight = d;
    }

    // Number of CPs
    getSint32(beamItem, DCM_NumberOfControlPoints, numberOfControlPoints);

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

    if(beamMetersetMU) os << "  BeamMeterset (MU): " << *beamMetersetMU << "\n";
    if(beamDoseGy)     os << "  BeamDose (Gy): " << *beamDoseGy << "\n";
    if(beamDoseSpecPointMm) {
        os << "  BeamDoseSpecPoint (mm): ["
           << (*beamDoseSpecPointMm)[0] << ", "
           << (*beamDoseSpecPointMm)[1] << ", "
           << (*beamDoseSpecPointMm)[2] << "]\n";
    }
}

