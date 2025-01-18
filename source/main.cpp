#include <cstdio>
#include <fstream>

#include "camera/camera.hpp"

#include "common/option.hpp"
#include "common/util.hpp"

int main(int argc, char* argv[])
{
    Scene::path = "/home/sakakibara/photon_mapping_output_image/scenes";
    std::printf("Scene directory:\n");
    std::printf("%s\n", Scene::path.c_str());

    std::vector<Option> options;
    try
    {
        options = availible(Scene::path);
    }
    catch (const std::exception& ex)
    {
        std::printf("%s\n", ex.what());
        return -1;
    }

    if (options.empty())
    {
        std::printf("No scenes found.\n");
        return -1;
    }

    Option scene_option = getOption(options);

    std::ifstream scene_file(scene_option.path);
    nlohmann::json j;
    scene_file >> j;
    scene_file.close();

    std::unique_ptr<Camera> camera = std::make_unique<Camera>(j, scene_option);

    camera->capture();

    return 0;
}