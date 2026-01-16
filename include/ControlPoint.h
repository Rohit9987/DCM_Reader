#pragma once

#include <array>
#include <optional>
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>

#include <dcmtk/dcmdata/dctk.h>

struct ControlPoint
{
    // Identity/Weighting
    int cpIndex = -1;
    double cumulativeMetersetWeight = 0.0;

    // Geometry
    double gantryAngleDeg;
    std::optional<std::string> gantryRotationDirection;         // NONE/CW/CC

    double collimatorAngleDeg;                   // BeamLimitingDeviceAngle
    std::optional<std::string> collimatorRotationDirection;     // Maybe not needed

    double couchAngleDeg;
    std::optional<std::string> couchRotationDirection;          // Maybe not needed

    std::optional<std::array<double,3>> isocenterMm;
    std::optional<double> ssdMm;

    //optional metadata
    double nominalEnergyMV;
    double doseRate;

    // Aperture state
    std::optional<std::array<double,2>> jawX;                   // ASYMX [x1, x2]
    std::optional<std::array<double,2>> jawY;                   // ASYMY[y1, y2]

    int leafPairs = 0;                                          // Typically 60
    std::vector<double> mlcA;
    std::vector<double> mlcB;

    ControlPoint(DcmItem* cpItem);

    bool hasMLC() const { return leafPairs > 0 && (int)mlcA.size()==leafPairs && (int)mlcB.size()==leafPairs; }

    void print(std::ostream& os = std::cout) const;
};

