#pragma once

#include <string>
#include <optional>
#include <vector>
#include <array>

#include "Beam.h"

struct Plan
{
    // Provenance
    std::string filePath;

    // Patient
    std::string patientName;
    std::string patientId;

    // UIDs
    std::string studyInstanceUid;
    std::string seriesInstanceUid;
    std::string sopInstanceUid;
    std::string frameOfReferenceUid;   // important for linking CT/RTSTRUCT/RTDOSE

    // Plan identity
    std::string rtPlanLabel;
    std::string rtPlanName;
    std::optional<std::string> rtPlanDescription;
    std::optional<std::string> rtPlanGeometry;          // PATIENT etc.
    std::optional<std::string> approvalStatus;          // UNAPPROVED/APPROVED

    // Date/time (optional)
    std::optional<std::string> rtPlanDate;
    std::optional<std::string> rtPlanTime;

    // Patient setup (optional)
    std::optional<std::string> patientPosition;         // HFS, etc.

    // Linking to structure set
    std::optional<std::string> referencedStructSetSOPInstanceUid;

    // Fractionation summary (usually from FractionGroupSequence[0])
    int fractionGroupNumber = -1;
    int numFractionsPlanned = -1;

    // Convenience / sanity
    std::optional<std::array<double,3>> primaryIsocenterMm;

    // Delivery content
    std::vector<Beam> beams;

    // Convenience totals
    std::optional<double> totalPlannedMetersetMU; // sum of beamMetersetMU
};

