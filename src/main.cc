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

    /*
    std::cout << "File: " << path.filename() << "\n"
              << "  SOPClass: " << (sopName ? sopName : "unknown") << "\n"
              << "  PatientID: " << patientOpt->id << "\n"
              << "  PatientName: " << patientOpt->name << "\n"
              << "--------------------------------------------------\n";
              */
    if(sopClass == UID_RTPlanStorage)
    {
        RtPlanSummary plan;
        std::string err;
        if(!RtPlanReader::ReadPlan(path.string(), plan, err))
        {
            std::cerr << "Failed RTPLAN parse: " << err << "\n";
        }
    }
}

#include <filesystem>
#include <iostream>
#include <vector>
#include <algorithm>

namespace fs = std::filesystem;

static bool isDicomFileByExtension(const fs::path& p)
{
    if(!p.has_extension()) return false;
    std::string ext = p.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c){ return (char)std::tolower(c); });
    return ext == ".dcm";
}

static std::vector<fs::path> collectDicomFiles(const fs::path& input)
{
    std::vector<fs::path> files;

    if(!fs::exists(input))
        return files;

    if(fs::is_regular_file(input))
    {
        if(isDicomFileByExtension(input))
            files.push_back(input);
        return files;
    }

    if(fs::is_directory(input))
    {
        for(const auto& entry : fs::directory_iterator(input))
        {
            if(entry.is_regular_file() && isDicomFileByExtension(entry.path()))
                files.push_back(entry.path());
        }
        return files;
    }

    return files; // e.g. symlink, socket, etc.
}

int main(int argc, char** argv)
{
    if(argc < 2)
    {
        std::cerr << "Usage: dicom_reader <dicom_folder_or_file>\n";
        return 1;
    }

    fs::path input(argv[1]);
    auto dicomFiles = collectDicomFiles(input);

    if(dicomFiles.empty())
    {
        std::cerr << "No .dcm files found at: " << input << "\n";
        return 2;
    }

    std::optional<PatientInfo> referencePatient;

    for(const auto& f : dicomFiles)
        readDicomFile(f, referencePatient);

    return 0;
}

