#pragma once

#include <string>
#include <vector>

struct RtPlanSummary
{
    std::string filePath;

    std::string patientName;
    std::string patientId;

    std::string studyInstanceUid;
    std::string seriesInstanceUid;
    std::string sopInstanceUid;

    std::string rtPlanLabel;
    std::string rtPlanName;

    int numFractionsPlanned = -1;
    
    std::vector<float> isocenter;           // TODO: lets assume single isocenter and proceed for now.

};

struct DoseReferencePrescription {
    int doseReferenceNumber = -1;         // (300A,0012)
    std::string doseReferenceType;         // (300A,0020) e.g. TARGET
    std::string doseReferenceStructureType;// (300A,0014) POINT/VOLUME/COORDINATES
    int referencedRoiNumber = -1;          // (300A,0022) if present
    double targetPrescriptionDoseGy = -1;  // (300A,0026) if present

    bool hasPointCoordinates = false;
    double refPointXYZ_mm[3] = {0,0,0};    // (300A,0018) if present
};


class RtPlanReader
{
public:
    // Throws no exceptions; return false + sets errorMessage on failure
    static bool ReadPlan(const std::string& rtplanPath,
                         RtPlanSummary& outPlan,
                         std::string& errorMessage);

private:
    static bool IsRtPlanSopClass(const char* sopClassUid);

    // Utility getters (kept private to avoid leaking DCMTK types in header)
    static bool GetStr(void* dataset, const unsigned short group, const unsigned short element, std::string& out);
    static bool GetInt(void* dataset, const unsigned short group, const unsigned short element, int& out);
    static bool GetFloat64(void* dataset, const unsigned short group, const unsigned short element, double& out);
};

