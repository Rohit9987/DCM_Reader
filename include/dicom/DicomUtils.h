#pragma once

#include <dcmtk/dcmdata/dctk.h>
#include <string>
#include <array>
#include <vector>

namespace dicom
{

// String
bool getString(DcmItem* it,
               const DcmTagKey& key,
               std::string& out,
               unsigned long pos = 0);

// Integer
bool getInt(DcmItem* it,
            const DcmTagKey& key,
            int& out,
            unsigned long pos = 0);

// Double
bool getDouble(DcmItem* it,
               const DcmTagKey& key,
               double& out,
               unsigned long pos = 0);

// Multi-value helpers
bool getDouble2(DcmItem* it,
                const DcmTagKey& key,
                std::array<double,2>& out2);

bool getDouble3(DcmItem* it,
                const DcmTagKey& key,
                std::array<double,3>& out3);

// Vector (for MLC etc.)
bool getDoubleVector(DcmItem* it,
                     const DcmTagKey& key,
                     std::vector<double>& out);

// Sequence helper
DcmSequenceOfItems* getSequence(DcmItem* it,
                                const DcmTagKey& key);

}

