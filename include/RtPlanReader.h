#pragma once

#include <string>
#include <vector>

struct RtControlPointSummary
{
    int index = -1;
    double gantryAngleDeg = 0.0;
    double collimatorAngleDeg = 0.0;
    double couchAngleDeg = 0.0;
};

struct RtBeamSummary
{
    int beamNumber = -1;
    std::string beamName;
    std::string beamType;       // e.g., STATIC, DYNAMIC
    std::string radiationType;  // e.g., PHOTON, ELECTRON
    std::string treatmentDeliveryType;  // e.g., TREATMENT, SETUP

    int numControlPoints = 0;
    std::vector<RtControlPointSummary> ControlPoints;   // optional sampling
};

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
    
    std::vector<RtBeamSummary> beams;

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

