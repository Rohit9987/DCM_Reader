#include "dicom/DicomUtils.h"
#include <dcmtk/dcmdata/dcdeftag.h>
#include <dcmtk/dcmdata/dcelem.h>

namespace dicom
{

bool getString(DcmItem* it,
               const DcmTagKey& key,
               std::string& out,
               unsigned long pos)
{
    if(!it) return false;

    OFString s;
    if(it->findAndGetOFString(key, s, pos).good())
    {
        out = s.c_str();
        return true;
    }
    return false;
}

bool getInt(DcmItem* it,
            const DcmTagKey& key,
            int& out,
            unsigned long pos)
{
    if(!it) return false;

    Sint32 v;
    if(it->findAndGetSint32(key, v, pos).good())
    {
        out = static_cast<int>(v);
        return true;
    }
    return false;
}

bool getDouble(DcmItem* it,
               const DcmTagKey& key,
               double& out,
               unsigned long pos)
{
    if(!it) return false;

    Float64 v;
    if(it->findAndGetFloat64(key, v, pos).good())
    {
        out = static_cast<double>(v);
        return true;
    }
    return false;
}

bool getDouble2(DcmItem* it,
                const DcmTagKey& key,
                std::array<double,2>& out2)
{
    double v0, v1;
    if(getDouble(it, key, v0, 0) &&
       getDouble(it, key, v1, 1))
    {
        out2 = {v0, v1};
        return true;
    }
    return false;
}

bool getDouble3(DcmItem* it,
                const DcmTagKey& key,
                std::array<double,3>& out3)
{
    double v0, v1, v2;
    if(getDouble(it, key, v0, 0) &&
       getDouble(it, key, v1, 1) &&
       getDouble(it, key, v2, 2))
    {
        out3 = {v0, v1, v2};
        return true;
    }
    return false;
}

bool getDoubleVector(DcmItem* it,
                     const DcmTagKey& key,
                     std::vector<double>& out)
{
    if(!it) return false;

    DcmElement* elem = nullptr;
    if(it->findAndGetElement(key, elem).bad() || !elem)
        return false;

    const unsigned long vm = elem->getVM();
    if(vm == 0) return false;

    out.clear();
    out.reserve(vm);

    for(unsigned long i = 0; i < vm; ++i)
    {
        Float64 v;
        if(elem->getFloat64(v, i).bad())
            return false;

        out.push_back(static_cast<double>(v));
    }

    return true;
}

DcmSequenceOfItems* getSequence(DcmItem* it,
                                const DcmTagKey& key)
{
    if(!it) return nullptr;

    DcmSequenceOfItems* seq = nullptr;
    if(it->findAndGetSequence(key, seq).good())
        return seq;

    return nullptr;
}

}

