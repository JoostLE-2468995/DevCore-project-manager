#ifndef DEVMAP_HPP
#define DEVMAP_HPP

#include "../dependencies/Canvas.hpp"
#include "../dependencies/Config.hpp"
#include "Main.hpp"
#include <string>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <set>
#include <ctime>
#include <algorithm>
#include <cstdlib>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;

namespace DevMap
{
    // Project structure holding project metadata.
    struct Project
    {
        std::string name;       // Virtual name for the manager.
        std::string folderName; // Actual folder name of the project.
        std::string lang;       // Language (also used as directory name).
        std::string createdBy;  // User who created the project.
        time_t createdAt;       // Creation time.
        size_t size;            // Project size in bytes.
        bool usesGit;           // Wether there is a .git folder in the projects
    };

    // Global inline variables to store the DevMap state.
    inline fs::path projectsPath;
    inline fs::path devmapFileName;
    inline nlohmann::json devmapData;
    inline std::vector<std::string> languages;
    inline std::set<std::string> users;
    inline std::vector<Project> projects;

    // Helper: Convert a time string ("HH:MM DD-MM-YYYY") to a time_t value.
    inline time_t parseTime(const std::string &timeStr)
    {
        std::tm tm = {};
        std::istringstream ss(timeStr);
        ss >> std::get_time(&tm, "%H:%M %d-%m-%Y");
        if (ss.fail())
        {
            return std::time(nullptr); // Fallback to current time.
        }
        return std::mktime(&tm);
    }

    // Helper: Convert a time_t value back to a formatted string.
    inline std::string timeToString(time_t t)
    {
        std::tm *tmPtr = std::localtime(&t);
        char buffer[32];
        std::strftime(buffer, sizeof(buffer), "%H:%M %d-%m-%Y", tmPtr);
        return std::string(buffer);
    }

    inline bool usesGit(const std::string &projectfolder)
    {
        fs::path gitPath = fs::path(projectfolder) / ".git";
        return fs::exists(gitPath) && fs::is_directory(gitPath);
    }

    inline size_t getFolderSize(const std::string &projectfolder)
    {
        size_t totalSize = 0;
        fs::path folderPath(projectfolder);
        if (fs::exists(folderPath) && fs::is_directory(folderPath))
        {
            for (const auto &entry : fs::recursive_directory_iterator(folderPath))
            {
                if (fs::is_regular_file(entry.status()))
                {
                    totalSize += fs::file_size(entry);
                }
            }
        }
        return totalSize;
    }

    inline void CreateProject(const Project &proj)
    {
        fs::path projPath = projectsPath / proj.lang / proj.folderName;
        try
        {
            if (!fs::exists(projPath))
            {
                fs::create_directories(projPath);
                Canvas::PrintInfo("Created project directory: " + projPath.string());
            }
            else
            {
                Canvas::PrintInfo("Project directory already exists: " + projPath.string());
            }
        }
        catch (const fs::filesystem_error &e)
        {
            Canvas::PrintError("Error creating project directory: " + std::string(e.what()));
        }
    }

    // Synchronize the DevMap JSON data with the filesystem.
    inline void syncDevMap()
    {
        users.clear();

        // 1. Populate languages vector from JSON and ensure directories exist.
        if (devmapData.contains("Languages") && devmapData["Languages"].is_array())
        {
            languages.clear();
            for (const auto &lang : devmapData["Languages"])
            {
                std::string language = lang.get<std::string>();
                languages.push_back(language);
                fs::path langPath = projectsPath / language;
                if (!fs::exists(langPath))
                {
                    fs::create_directories(langPath);
                    Canvas::PrintInfo("Created language directory: " + langPath.string());
                }
            }
        }

        // 2. Scan projectsPath for language directories not in JSON and update.
        for (const auto &entry : fs::directory_iterator(projectsPath))
        {
            if (entry.is_directory())
            {
                std::string langDir = entry.path().filename().string();
                if (std::find(languages.begin(), languages.end(), langDir) == languages.end())
                {
                    languages.push_back(langDir);
                    devmapData["Languages"].push_back(langDir);
                    Canvas::PrintInfo("Added new language from filesystem to DevMap: " + langDir);
                }
            }
        }

        // 3. Populate projects vector from JSON.
        projects.clear();
        if (devmapData.contains("Projects") && devmapData["Projects"].is_array())
        {
            for (const auto &projData : devmapData["Projects"])
            {
                Project proj;
                proj.name = projData.value("name", "");
                proj.folderName = projData.value("folderName", "");
                proj.lang = projData.value("lang", "");
                proj.createdBy = projData.value("created_by", "");
                std::string createdAtStr = projData.value("created_at", "");
                proj.createdAt = parseTime(createdAtStr);
                proj.size = projData.value("size", 0);
                proj.usesGit = projData.value("git", false);
                projects.push_back(proj);
            }
        }

        // 4. Ensure each project directory exists; if missing, create it.
        for (const auto &proj : projects)
        {
            fs::path projPath = projectsPath / proj.lang / proj.folderName;
            users.insert(proj.createdBy);
            if (!fs::exists(projPath))
            {
                CreateProject(proj);
            }
        }

        // 4.5. Update existing project data (size and Git status) from the filesystem.
        if (devmapData.contains("Projects") && devmapData["Projects"].is_array())
        {
            for (auto &projData : devmapData["Projects"])
            {
                std::string language = projData.value("lang", "");
                std::string folderName = projData.value("folderName", "");
                fs::path projPath = projectsPath / language / folderName;
                if (fs::exists(projPath) && fs::is_directory(projPath))
                {
                    std::string fullProjPath = projPath.string();
                    size_t currentSize = getFolderSize(fullProjPath);
                    bool currentUsesGit = usesGit(fullProjPath);
                    projData["size"] = currentSize;
                    projData["git"] = currentUsesGit;
                    // Also update the corresponding project in the projects vector.
                    for (auto &proj : projects)
                    {
                        if (proj.folderName == folderName && proj.lang == language)
                        {
                            proj.size = currentSize;
                            proj.usesGit = currentUsesGit;
                            break;
                        }
                    }
                }
            }
        }

        // 5. For every language directory, add any project directory not listed in the JSON.
        for (const auto &language : languages)
        {
            fs::path langPath = projectsPath / language;
            if (!fs::exists(langPath))
                continue;
            for (const auto &entry : fs::directory_iterator(langPath))
            {
                if (entry.is_directory())
                {
                    std::string folderName = entry.path().filename().string();
                    bool found = false;
                    for (const auto &proj : projects)
                    {
                        if (proj.folderName == folderName && proj.lang == language)
                        {
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                    {
                        // New project detected on filesystem; add it with default values.
                        Project newProj;
                        std::string projectPath = Main::HOME_PATH + std::string(projectsPath) + language + "/" + folderName;
                        newProj.name = folderName; // Default: use folder name as project name.
                        newProj.folderName = folderName;
                        newProj.lang = language;
                        newProj.createdBy = "unknown";
                        newProj.createdAt = std::time(nullptr);
                        newProj.size = getFolderSize(projectPath);
                        newProj.usesGit = usesGit(projectPath);
                        projects.push_back(newProj);

                        // Update the JSON data.
                        nlohmann::json newProjJson = {
                            {"name", newProj.name},
                            {"folderName", newProj.folderName},
                            {"lang", newProj.lang},
                            {"created_by", newProj.createdBy},
                            {"created_at", timeToString(newProj.createdAt)},
                            {"size", newProj.size},
                            {"git", newProj.usesGit}};
                        devmapData["Projects"].push_back(newProjJson);
                        Canvas::PrintInfo("Added new project from filesystem to DevMap: " + folderName + " in " + language);
                    }
                }
            }
        }

        // 6. Optionally update users vector from JSON.
        if (devmapData.contains("Users") && devmapData["Users"].is_array())
        {
            for (const auto user : devmapData["Users"])
                users.insert(user.get<std::string>());
            devmapData["Users"].clear();
            for (const auto &user : users)
                devmapData["Users"].push_back(user);
        }

        // 7. Write the updated JSON back to the file.
        std::ofstream outFile(devmapFileName);
        if (outFile.is_open())
        {
            outFile << devmapData.dump(4); // Pretty-print with indentations.
            outFile.close();
        }
        else
        {
            Canvas::PrintError("Unable to write to DevMap file: " + devmapFileName.string());
        }
    }

    // Load the DevMap from a JSON file.
    inline bool load(const std::string &filename, bool install = false)
    {

        if (install)
        {
            Canvas::PrintInfo("Checking for required directories");
            fs::path devmapPath = filename;
            fs::path devmapDir = devmapPath.parent_path();

            if (!fs::exists(devmapDir))
            {
                fs::create_directories(devmapDir);
            }

            // Clone and overwrite the DevMap file with the default template.
            Canvas::PrintInfo("Cloning the DevCore repository to retrieve the default DevMap.");
            std::string cloneCommand = "git clone " + Config::github + " /tmp/devcore_repo";
            if (std::system(cloneCommand.c_str()) != 0)
            {
                Canvas::PrintErrorExit("Failed to clone repository from " + Canvas::LinkText(Config::github));
            }

            fs::path sourceConfig = "/tmp/devcore_repo/devmap.json";
            if (!fs::exists(sourceConfig))
            {
                Canvas::PrintErrorExit("Default DevMap file not found in the cloned repository.");
            }
            Canvas::PrintInfo("Copying the new DevMap to '" + Canvas::LinkText(filename, Canvas::Color::CYAN) + "'");
            fs::copy_file(sourceConfig, devmapPath, fs::copy_options::overwrite_existing);
            Canvas::PrintInfo("Removing the temporary cloned repository.");
            fs::remove_all("/tmp/devcore_repo");
            Canvas::PrintSuccess(Canvas::BoldText("Done installing the default DevMap.") +
                                 Canvas::ColorToAnsi(Canvas::Color::GREEN) +
                                 "\n    You can list and manage projects in your devmap by running several commands (see `devcore --help` for more info). \n    You can edit the devmap manually at '" +
                                 Canvas::LinkText(filename, Canvas::Color::GREEN) + "', however, this is not recommended!");
        }

        // Save the filename and get projectsPath from configuration.
        devmapFileName = filename;
        projectsPath = Main::HOME_PATH + Config::get("projects_path");

        std::ifstream file(filename);
        if (!file.is_open())
        {
            return false;
        }

        try
        {
            file >> devmapData;
        }
        catch (const std::exception &e)
        {
            Canvas::PrintError("Failed to parse the DevMap file: " + std::string(e.what()));
            return false;
        }

        // At this point the JSON has been read.
        // The expected JSON structure is:
        // {
        //     "Projects": [
        //         {
        //             "name": "DevCore Project Manager",
        //             "folderName": "DevCore-project-manager",
        //             "lang": "C++",
        //             "created_by": "Huplo",
        //             "created_at": "23:04 17-03-2025",
        //             "size": 25042,
        //             "git": true,
        //         },
        //         ...
        //     ],
        //     "Languages": ["Java", "C++"],
        //     "Users": ["Huplo"]
        // }

        // Synchronize the JSON data with the filesystem.
        syncDevMap();

        return true;
    }

    // Setup function to create a default DevMap if one does not exist.
    inline int setup(const std::string &filename)
    {
        Canvas::ClearConsole();
        Canvas::PrintTitle("DevCore | Setup Zone");
        Canvas::PrintWarning("It seems like you do not yet have a DevMap file. You require the correct structure and we recommend you download the default template. Would you like to install the default (empty) DevMap? \n    If not, check out '" + Canvas::LinkText(filename, Canvas::Color::YELLOW) +
                             "' to configure one manually, although this is not recommended!");
        if (Canvas::GetBoolInput("    "))
        {
            load(filename, true);
            return 0;
        }
        return 0;
    }

    // Validate that the DevMap has been loaded.
    inline void validate()
    {
        if (devmapData.empty())
        {
            setup(devmapFileName);
            exit(0);
        }
    }

    // Return a string representation of the current DevMap configuration.
    inline std::string GetStringRepresentation()
    {
        validate();
        return devmapData.dump(4);
    }

    inline void ListProjects(bool extra = false)
    {
        std::vector<std::string> header;
        std::vector<std::vector<std::string>> rows;

        if (!extra)
        {
            header = {"Created By", "Name", "Language"};
            for (const auto &proj : projects)
            {
                rows.push_back({proj.createdBy,
                                proj.name,
                                proj.lang});
            }
        }
        else
        {
            header = {"Created By", "Name", "Folder", "Language", "Created At", "Size", "Git"};
            for (const auto &proj : projects)
            {
                rows.push_back({proj.createdBy,
                                proj.name,
                                proj.folderName,
                                proj.lang,
                                timeToString(proj.createdAt),
                                std::to_string(proj.size),
                                proj.usesGit ? "Yes" : "No"});
            }
        }
        // Display the table with the default color.
        Canvas::PrintTable(" Projects ", header, rows, Canvas::Color::CYAN);
    }

    inline void ListUsers()
    {
        std::vector<std::string> header;
        std::vector<std::vector<std::string>> rows;

        header = {"Users"};
        for (const auto &user : users)
        {
            rows.push_back({user});
        }
        // Display the table with the default color.
        Canvas::PrintTable("", header, rows, Canvas::Color::CYAN);
    }

    inline void ListLanguages()
    {
        std::vector<std::string> header;
        std::vector<std::vector<std::string>> rows;

        header = {"Languages   "};
        for (const auto &lang : languages)
        {
            rows.push_back({lang});
        }
        // Display the table with the default color.
        Canvas::PrintTable("", header, rows, Canvas::Color::CYAN);
    }

    inline void CreateLang(std::string &lang)
    {
        // Check if the language is already in the vector.
        if (std::find(languages.begin(), languages.end(), lang) == languages.end())
        {
            // Add to the languages vector.
            languages.push_back(lang);

            // Create the language folder if it does not exist.
            fs::path langPath = projectsPath / lang;
            if (!fs::exists(langPath))
            {
                fs::create_directories(langPath);
                Canvas::PrintInfo("Created language directory: " + langPath.string());
            }

            // Ensure the JSON "Languages" array exists.
            if (!devmapData.contains("Languages") || !devmapData["Languages"].is_array())
            {
                devmapData["Languages"] = nlohmann::json::array();
            }

            // Add the language to the JSON data.
            devmapData["Languages"].push_back(lang);
            Canvas::PrintInfo("Added language to DevMap: " + lang);

            // Write the updated JSON back to the file.
            std::ofstream outFile(devmapFileName);
            if (outFile.is_open())
            {
                outFile << devmapData.dump(4); // Pretty-print with indentations.
                outFile.close();
                Canvas::PrintInfo("DevMap updated successfully.");
            }
            else
            {
                Canvas::PrintError("Unable to write updated DevMap to: " + devmapFileName.string());
            }
        }
        else
        {
            Canvas::PrintInfo("Language already exists: " + lang);
        }
    }

    inline void CreateProjectWizard()
    {
        // Clear the console and print a vibrant title.
        Canvas::ClearConsole();
        Canvas::PrintTitle(u8"DevCore | Project Creation Wizard 🚀", Canvas::Color::MAGENTA);

        // 1. Ask for the project language.
        std::string projectLang = Canvas::GetStringInput(u8"👉 Please enter the project language: ", "", Canvas::Color::CYAN);
        // Verify if the language exists; if not, offer to create it.
        if (std::find(languages.begin(), languages.end(), projectLang) == languages.end())
        {
            bool createLang = Canvas::GetBoolInput(u8"⚠️ Language '" + projectLang + "' not found. Create it? ", "", Canvas::Color::YELLOW);
            if (createLang)
            {
                CreateLang(projectLang);
                Canvas::PrintSuccess(u8"Language '" + projectLang + "' created successfully!");
            }
            else
            {
                Canvas::PrintInfo(u8"❌ Project creation cancelled. Please choose an existing language next time.");
                return;
            }
        }

        // 2. Ask for the project name.
        std::string projectName = Canvas::GetStringInput(u8"📝 Enter your project name (spaces allowed): ", "", Canvas::Color::CYAN);

        // 3. Determine the project folder name.
        bool useNamingConvention = Canvas::GetBoolInput(u8"🔠 Use GitHub naming conventions for folder name? ", "", Canvas::Color::CYAN);
        std::string projectFolderName;
        if (useNamingConvention)
        {
            projectFolderName = projectName;
            // Convert to lowercase.
            std::transform(projectFolderName.begin(), projectFolderName.end(), projectFolderName.begin(), ::tolower);
            // Replace spaces with hyphens.
            std::replace(projectFolderName.begin(), projectFolderName.end(), ' ', '-');
            // Remove any characters other than alphanumeric or hyphen.
            projectFolderName.erase(std::remove_if(projectFolderName.begin(), projectFolderName.end(),
                [](char c) { return !(std::isalnum(c) || c == '-'); }), projectFolderName.end());
            Canvas::PrintInfo(u8"📁 Using folder name: " + projectFolderName);
        }
        else
        {
            projectFolderName = Canvas::GetStringInput(u8"📁 Enter a custom project folder name: ", "", Canvas::Color::CYAN);
        }

        // 4. Ask if the project should be initialized as a Git repository.
        bool initGit = Canvas::GetBoolInput(u8"🐙 Initialize as a Git repository? ", "", Canvas::Color::CYAN);

        // 5. Ask if the user wants to use a project template.
        bool useTemplate = Canvas::GetBoolInput(u8"🎨 Would you like to apply a project template? ", "", Canvas::Color::CYAN);
        std::string selectedTemplate = "";
        if (useTemplate)
        {
            // Template directory: HOME_PATH/TEMPLATE_PATH/projectLang.
            fs::path templateDir = fs::path(Main::HOME_PATH) / Main::TEMPLATE_PATH / projectLang;
            if (!fs::exists(templateDir) || !fs::is_directory(templateDir))
            {
                Canvas::PrintInfo(u8"📂 No templates available for '" + projectLang + "'. Skipping template.");
                useTemplate = false;
            }
            else
            {
                // List available templates.
                std::vector<std::string> templates;
                for (const auto &entry : fs::directory_iterator(templateDir))
                {
                    if (entry.is_directory())
                        templates.push_back(entry.path().filename().string());
                }
                if (templates.empty())
                {
                    Canvas::PrintInfo(u8"📂 No templates found in " + templateDir.string() + ". Skipping template.");
                    useTemplate = false;
                }
                else
                {
                    Canvas::PrintInfo(u8"✨ Available templates:");
                    for (size_t i = 0; i < templates.size(); i++)
                    {
                        Canvas::PrintInfo(u8"  " + std::to_string(i + 1) + u8". " + templates[i]);
                    }
                    std::string templateChoice = Canvas::GetStringInput(u8"🔢 Enter template number (or press Enter to skip): ", "", Canvas::Color::CYAN);
                    if (!templateChoice.empty())
                    {
                        int choice = std::stoi(templateChoice);
                        if (choice > 0 && choice <= static_cast<int>(templates.size()))
                        {
                            selectedTemplate = templates[choice - 1];
                            Canvas::PrintInfo(u8"🎉 Template '" + selectedTemplate + "' selected.");
                        }
                        else
                        {
                            Canvas::PrintInfo(u8"❌ Invalid choice. Skipping template.");
                            useTemplate = false;
                        }
                    }
                    else
                    {
                        useTemplate = false;
                    }
                }
            }
        }

        // 6. Prepare the new project data.
        Project newProj;
        newProj.name = projectName;
        newProj.folderName = projectFolderName;
        newProj.lang = projectLang;
        newProj.createdBy = "current_user";  // Replace with actual user info if available.
        newProj.createdAt = std::time(nullptr);
        newProj.size = 0;  // Will be updated if a template is applied.
        newProj.usesGit = initGit;

        // 7. Create the project directory.
        CreateProject(newProj);
        Canvas::PrintSuccess(u8"🚀 Project directory created successfully!");

        // 8. If a template was selected, copy its contents into the new project folder.
        if (useTemplate && !selectedTemplate.empty())
        {
            fs::path templatePath = fs::path(Main::HOME_PATH) / Main::TEMPLATE_PATH / projectLang / selectedTemplate;
            fs::path projectPath = projectsPath / projectLang / projectFolderName;
            try
            {
                fs::copy(templatePath, projectPath, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
                Canvas::PrintSuccess(u8"✨ Template '" + selectedTemplate + "' applied to project.");
            }
            catch (const std::exception &e)
            {
                Canvas::PrintError(u8"❌ Error copying template: " + std::string(e.what()));
            }
            // Update project size after copying template contents.
            newProj.size = getFolderSize(projectPath.string());
        }

        // 9. Initialize Git repository if requested.
        if (initGit)
        {
            fs::path projectPath = projectsPath / projectLang / projectFolderName;
            std::string initCommand = "cd " + projectPath.string() + " && git init";
            if (std::system(initCommand.c_str()) == 0)
            {
                Canvas::PrintSuccess(u8"🐙 Git repository initialized in " + projectPath.string());
            }
            else
            {
                Canvas::PrintError(u8"❌ Failed to initialize Git repository in " + projectPath.string());
            }
        }

        // 10. Update the DevMap JSON with the new project entry.
        nlohmann::json projJson = {
            {"name", newProj.name},
            {"folderName", newProj.folderName},
            {"lang", newProj.lang},
            {"created_by", newProj.createdBy},
            {"created_at", timeToString(newProj.createdAt)},
            {"size", newProj.size},
            {"git", newProj.usesGit}
        };
        devmapData["Projects"].push_back(projJson);
        Canvas::PrintSuccess(u8"✅ Project '" + newProj.name + "' created successfully!");
    }


} // namespace DevMap

#endif // DEVMAP_HPP
