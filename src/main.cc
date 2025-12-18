#include <dcmtk/dcmdata/dctk.h>
#include <dcmtk/dcmdata/dcuid.h>

#include <filesystem>
#include <iostream>
#include <optional>
#include <string>

#include "RtPlanReader.h"

namespace fs = std::filesystem;

struct PatientInfo
{
    std::string name;
    std::string id;
};

static std::optional<PatientInfo> extractPatientInfo(DcmDataset* ds)
{
    OFString name, id;

    if (ds->findAndGetOFString(DCM_PatientName, name).bad())
        return std::nullopt;

    if (ds->findAndGetOFString(DCM_PatientID, id).bad())
        return std::nullopt;

    return PatientInfo{ name.c_str(), id.c_str() };
}

static void readDicomFile(
    const fs::path& path,
    std::optional<PatientInfo>& referencePatient)
{
    DcmFileFormat ff;
    OFCondition st = ff.loadFile(path.string().c_str());
    if (!st.good()) {
        std::cerr << "Failed to read: " << path << " (" << st.text() << ")\n";
        return;
    }

    DcmDataset* ds = ff.getDataset();

    auto patientOpt = extractPatientInfo(ds);
    if (!patientOpt) {
        std::cerr << "WARNING: Missing patient info in " << path.filename() << "\n";
        return;
    }

    if (!referencePatient) {
        referencePatient = patientOpt;
        std::cout << "Reference patient set from: "
                  << path.filename() << "\n";
    } else {
        if (patientOpt->id != referencePatient->id ||
            patientOpt->name != referencePatient->name) {

            std::cerr << "WARNING: Patient mismatch detected!\n"
                      << "  File: " << path.filename() << "\n"
                      << "  Expected ID:   " << referencePatient->id << "\n"
                      << "  Found ID:      " << patientOpt->id << "\n"
                      << "  Expected Name: " << referencePatient->name << "\n"
                      << "  Found Name:    " << patientOpt->name << "\n";
        }
    }

    // Print basic identifiers (same as before)
    OFString sopClass;
    ds->findAndGetOFString(DCM_SOPClassUID, sopClass);

    const char* sopName = dcmFindNameOfUID(sopClass.c_str());

    std::cout << "File: " << path.filename() << "\n"
              << "  SOPClass: " << (sopName ? sopName : "unknown") << "\n"
              << "  PatientID: " << patientOpt->id << "\n"
              << "  PatientName: " << patientOpt->name << "\n"
              << "--------------------------------------------------\n";
    if(sopClass == UID_RTPlanStorage)
    {
        std::cout << "Plan storage!\n";
        RtPlanSummary plan;
        std::string err;
        if(RtPlanReader::ReadPlan(path.string(), plan, err))
        {

            std::cout << "RTPLAN: " << plan.rtPlanLabel << " | beams=" << plan.beams.size() << "\n";
            std::cout << "Name: " << plan.rtPlanName << " | numFractionsPlanned = " << plan.numFractionsPlanned << '\n';
        } else {
            std::cerr << "Failed RTPLAN parse: " << err << "\n";
        }
        std::cout <<"-----------------------------------------------\n";
    }
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::cerr << "Usage: dicom_reader <dicom_folder>\n";
        return 1;
    }

    fs::path folder(argv[1]);
    if (!fs::exists(folder) || !fs::is_directory(folder)) {
        std::cerr << "Not a valid directory: " << folder << "\n";
        return 2;
    }

    std::optional<PatientInfo> referencePatient;

    for (const auto& entry : fs::directory_iterator(folder)) {
        if (!entry.is_regular_file())
            continue;

        if (entry.path().extension() == ".dcm") {
            readDicomFile(entry.path(), referencePatient);
        }
    }

    return 0;
}

