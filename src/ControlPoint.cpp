#include <ControlPoint.h>

#include <dcmtk/dcmdata/dcdeftag.h>
#include <algorithm> 



// ---- Small helpers that work with DcmItem* (dataset OR sequence items) ----
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

static bool getFloat64_2(DcmItem* it, const DcmTagKey& key, std::array<double,2>& out2)
{
    double v0, v1;
    if(getFloat64(it, key, v0, 0) && getFloat64(it, key, v1, 1)) {
        out2 = {v0, v1};
        return true;
    }
    return false;
}

// Reads LeafJawPositions (300A,011C) as a vector<double>
static bool getFloat64Vector(DcmItem* it, const DcmTagKey& key, std::vector<double>& out)
{
    if(!it) return false;

    DcmElement* elem = nullptr;
    if(it->findAndGetElement(key, elem).bad() || !elem)
        return false;

    const unsigned long vm = elem->getVM();
    if(vm == 0) return false;

    out.clear();
    out.reserve(static_cast<size_t>(vm));

    for(unsigned long i = 0; i < vm; ++i)
    {
        Float64 v;
        if(elem->getFloat64(v, i).bad())  // read i-th component directly from the element
            return false;

        out.push_back(static_cast<double>(v));
    }
    return true;
}

ControlPoint::ControlPoint(DcmItem* cpItem)
{
    leafPairs = 60;

    getSint32(cpItem, DCM_ControlPointIndex, cpIndex);
    getFloat64(cpItem, DCM_CumulativeMetersetWeight, cumulativeMetersetWeight);

    //TODO: gantryrotationDirection
    getFloat64(cpItem, DCM_GantryAngle, gantryAngleDeg);
    {
    std::string s;
    if(getOFString(cpItem, DCM_GantryRotationDirection, s))
            gantryRotationDirection=s;
    }
    getFloat64(cpItem, DCM_BeamLimitingDeviceAngle, collimatorAngleDeg);
    getFloat64(cpItem, DCM_PatientSupportAngle, couchAngleDeg);

    {
        std::array<double, 3> iso;
        if(getFloat64_3(cpItem, DCM_IsocenterPosition, iso))
            isocenterMm = iso;

        double ssd;
        if(getFloat64(cpItem, DCM_SourceToSurfaceDistance, ssd))
            ssdMm = ssd;
    }
    
    getFloat64(cpItem, DCM_NominalBeamEnergy, nominalEnergyMV);
    getFloat64(cpItem, DCM_DoseRateSet, doseRate);

    DcmSequenceOfItems* posSeq = nullptr;
    if(cpItem && cpItem->findAndGetSequence(DCM_BeamLimitingDevicePositionSequence, posSeq).good() && posSeq)
    {
       for(unsigned long i = 0; i < posSeq->card(); ++i)
        {
            DcmItem* posItem = posSeq->getItem(i);
            if(!posItem) continue;

            std::string devType;
            if(!getOFString(posItem, DCM_RTBeamLimitingDeviceType, devType)) continue;

            std::vector<double> vals;
            if(!getFloat64Vector(posItem, DCM_LeafJawPositions, vals)) continue;

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
