#pragma once

#include <array>
#include <optional>
#include <string>
#include <vector>
#include <iostream>

#include <dcmtk/dcmdata/dctk.h>
#include <dcmtk/dcmdata/dcdeftag.h>

#include "ControlPoint.h"

struct Beam
{
    // Identity
    int beamNumber = -1;
    std::optional<std::string> beamName;

    // Classification
    std::optional<std::string> beamType;               // STATIC / DYNAMIC
    std::optional<std::string> radiationType;          // PHOTON / ELECTRON
    std::optional<std::string> treatmentDeliveryType;  // TREATMENT / SETUP

    // Machine / geometry meta
    std::optional<std::string> treatmentMachineName;   // (300A,00B2)
    std::optional<std::string> primaryDosimeterUnit;   // (300A,00B3) MU
    std::optional<double> sourceAxisDistanceMm;        // (300A,00B4)

    // Control point metadata
    std::optional<double> finalCumulativeMetersetWeight; // (300A,010E)
    int numberOfControlPoints = 0;                       // (300A,0110)

    int leafPairs = 60;                                  // pass in from machine model
    std::vector<ControlPoint> controlPoints;

    // Populated from FractionGroupSequence/ReferencedBeamSequence (later)
    double beamMetersetMU;                // (300A,0086)
    double beamDoseGy;                    // (300A,0084)
    std::array<double,3> beamDoseSpecPointMm; // (300A,0082)

    Beam() = default;
    explicit Beam(DcmItem* beamItem);

    void print(std::ostream& os = std::cout) const;

    // TODO: store MU, Beam dose and dose spec point
    void storeFractionSequence(DcmItem* item); 
    
};

