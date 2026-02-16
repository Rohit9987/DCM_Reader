#include <ControlPoint.h>

#include <dcmtk/dcmdata/dcdeftag.h>
#include <algorithm> 
#include "dicom/DicomUtils.h"

ControlPoint::ControlPoint(DcmItem* cpItem)
{
    using namespace dicom;
    leafPairs = 60;

    getInt(cpItem, DCM_ControlPointIndex, cpIndex);
    getDouble(cpItem, DCM_CumulativeMetersetWeight, cumulativeMetersetWeight);

    //TODO: gantryrotationDirection
    getDouble(cpItem, DCM_GantryAngle, gantryAngleDeg);
    {
    std::string s;
    if(getString(cpItem, DCM_GantryRotationDirection, s))
            gantryRotationDirection=s;
    }
    getDouble(cpItem, DCM_BeamLimitingDeviceAngle, collimatorAngleDeg);
    getDouble(cpItem, DCM_PatientSupportAngle, couchAngleDeg);

    {
        std::array<double, 3> iso;
        if(getDouble3(cpItem, DCM_IsocenterPosition, iso))
            isocenterMm = iso;

        double ssd;
        if(getDouble(cpItem, DCM_SourceToSurfaceDistance, ssd))
            ssdMm = ssd;
    }
    
    getDouble(cpItem, DCM_NominalBeamEnergy, nominalEnergyMV);
    getDouble(cpItem, DCM_DoseRateSet, doseRate);

    DcmSequenceOfItems* posSeq = nullptr;
    if(cpItem && cpItem->findAndGetSequence(DCM_BeamLimitingDevicePositionSequence, posSeq).good() && posSeq)
    {
       for(unsigned long i = 0; i < posSeq->card(); ++i)
        {
            DcmItem* posItem = posSeq->getItem(i);
            if(!posItem) continue;

            std::string devType;
            if(!getString(posItem, DCM_RTBeamLimitingDeviceType, devType)) continue;

            std::vector<double> vals;
            if(!getDoubleVector(posItem, DCM_LeafJawPositions, vals)) continue;

            if(devType == "ASYMX" && vals.size() >= 2)
            {
                jawX = std::array<double,2>{vals[0], vals[1]};
            }
            else if(devType == "ASYMY" && vals.size() >= 2)
            {
                jawY = std::array<double,2>{vals[0], vals[1]};
            }
            else if(devType == "MLCX")
            {
                const int n = leafPairs;
                if(n > 0 && (int)vals.size() >= 2*n)
                {
                    mlcA.assign(vals.begin(), vals.begin() + n);
                    mlcB.assign(vals.begin() + n, vals.begin() + 2*n);
                }
            }
        }
    }
}
void ControlPoint::print(std::ostream& os) const
{
    os << "    ControlPoint index=" << cpIndex
       << "  CMW=" << cumulativeMetersetWeight << "\n";

    auto pOptD = [&](const char* label, const std::optional<double>& v){
        os << "    " << std::left << std::setw(28) << label << ": ";
        if(v) os << *v; else os << "<missing>";
        os << "\n";
    };
    auto pOptS = [&](const char* label, const std::optional<std::string>& v){
        os << "    " << std::left << std::setw(28) << label << ": ";
        if(v && !v->empty()) os << *v; else os << "<missing>";
        os << "\n";
    };

    pOptD("GantryAngle (deg)", gantryAngleDeg);
    pOptS("GantryRotationDirection", gantryRotationDirection);
    pOptD("CollimatorAngle (deg)", collimatorAngleDeg);
    pOptS("CollimatorRotationDirection", collimatorRotationDirection);
    pOptD("CouchAngle (deg)", couchAngleDeg);
    pOptS("CouchRotationDirection", couchRotationDirection);

    if(isocenterMm) {
        os << "    " << std::left << std::setw(28) << "Isocenter (mm)" << ": ["
           << (*isocenterMm)[0] << ", " << (*isocenterMm)[1] << ", " << (*isocenterMm)[2] << "]\n";
    } else {
        os << "    " << std::left << std::setw(28) << "Isocenter (mm)" << ": <missing>\n";
    }

    pOptD("SSD (mm)", ssdMm);
    pOptD("NominalEnergy (MV)", nominalEnergyMV);
    pOptD("DoseRateSet", doseRate);

    if(jawX) {
        os << "    " << std::left << std::setw(28) << "Jaws X (mm)" << ": ["
           << (*jawX)[0] << ", " << (*jawX)[1] << "]\n";
    } else {
        os << "    " << std::left << std::setw(28) << "Jaws X (mm)" << ": <missing>\n";
    }

    if(jawY) {
        os << "    " << std::left << std::setw(28) << "Jaws Y (mm)" << ": ["
           << (*jawY)[0] << ", " << (*jawY)[1] << "]\n";
    } else {
        os << "    " << std::left << std::setw(28) << "Jaws Y (mm)" << ": <missing>\n";
    }

    os << "    " << std::left << std::setw(28) << "MLC (leaf pairs)" << ": " << leafPairs << "\n";
    if(hasMLC()) {
        os << "    MLC A[0..min(4)] (mm): ";
        for(int i=0; i<std::min(5, leafPairs); ++i) os << mlcA[i] << (i< std::min(5,leafPairs)-1 ? ", " : "");
        os << "\n";
        os << "    MLC B[0..min(4)] (mm): ";
        for(int i=0; i<std::min(5, leafPairs); ++i) os << mlcB[i] << (i< std::min(5,leafPairs)-1 ? ", " : "");
        os << "\n";
    } else {
        os << "    MLC: <missing>\n";
    }
}
