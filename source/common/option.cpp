#include "option.hpp"

#include <fstream>
#include <sstream>
#include <cstdio>
#include <iostream>
#include <limits>

#include <glm/vec3.hpp>
#include <nlohmann/json.hpp>

#include "util.hpp"

std::vector<std::string> json_list;
void make_json_list(){
    json_list.push_back("/home/sakakibara/photon_mapping_output_image/scenes/hexagon_room.json");
    json_list.push_back("/home/sakakibara/photon_mapping_output_image/scenes/water_caustics.json");
    json_list.push_back("/home/sakakibara/photon_mapping_output_image/scenes/diamond.json");
}

std::vector<Option> availible(std::string path)
{
    make_json_list();
    std::vector<Option> options;
    for (const auto& file_path : json_list)
    {
        std::ifstream scene_file(file_path);
        nlohmann::json j;
        scene_file >> j;

        bool photon_map = j.find("photon_map") != j.end();

        int i = 0;
        for (const auto& c : j.at("cameras"))
        {
            glm::dvec3 eye = c.at("eye");
            double f = c.at("focal_length");
            double s = c.at("sensor_width");
            std::stringstream ss;
            int eye_x = eye.x; int eye_y = eye.y; int eye_z = eye.z;
            ss << "Eye: " << std::fixed << "(" << eye_x << " " << eye_y << " " << eye_z << "), ";
            ss << "Focal length: " << int(f) << "mm (" << int(s) << "mm)";
            options.emplace_back(file_path, ss.str(), i, photon_map);
            i++;
        }
        scene_file.close();
    }
    return options;
}

Option getOption(std::vector<Option>& options)
{
    size_t max_opt = 13, max_fil = 0, max_cam = 0;
    for (const auto& o : options)
    {
        std::string file = o.path;
        file.erase(file.find("."), file.length());

        if (file.size() > max_fil)
            max_fil = file.size();

        if (o.camera.size() > max_cam)
            max_cam = o.camera.size();
    }
    max_fil++;
    max_cam++;

    std::printf(" %s\n", std::string(max_opt + max_fil + max_cam + 5, '_').c_str());

    auto printLine = [](std::vector<std::pair<std::string, size_t>> line) {
        std::printf("| ");
        for (const auto& l : line)
        {
            std::printf("%s%s", l.first.c_str(), std::string(l.second - l.first.size(), ' ').c_str());
            std::printf("| ");
        }
        std::printf("\n");
    };

    printLine({ { "Option", max_opt }, { "File", max_fil }, { "Camera", max_cam } });

    std::string sep("|" + std::string(max_opt + 1, '_') + '|' + std::string(max_fil + 1, '_') + '|' + std::string(max_cam + 1, '_') + '|');
    std::printf("%s\n", sep.c_str());

    for (int i = 0; i < options.size(); i++)
    {
        std::string file = options[i].path;
        file.erase(file.find("."), file.length());

        printLine({ {std::to_string(i), max_opt},{file, max_fil},{options[i].camera, max_cam} });
        std::printf("%s\n", sep.c_str());
    }

    int option;
    std::printf("\nSelect option: ");
    while (1)
    {
        std::scanf("%d", &option);
        if (option < 0 || option >= options.size())
            std::printf("Invalid option, try again: ");
        else
            break;
    }

    if (options[option].photon_map)
    {
        char a;
        std::printf("\nUse photon mapping? (y/n) ");
        while (std::scanf("%*c%c", &a))
        {
            if (a == 'y' || a == 'Y' || a == 'n' || a == 'N') break;
            std::printf("Answer with the letters y or n: ");
        }
        if (a == 'n' || a == 'N')
        {
            options[option].photon_map = false;
        }
    }

    return options[option];
}