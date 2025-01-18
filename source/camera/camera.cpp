#include "camera.hpp"

#include <algorithm>
#include <functional>
#include <cstdio>
#include <iomanip>
#include <sstream>

#include "../ray/ray.hpp"
#include "../integrator/path-tracer/path-tracer.hpp"
#include "../integrator/photon-mapper/photon-mapper.hpp"
#include "../sampling/sampling.hpp"
#include "../sampling/sampler.hpp"
#include "../common/util.hpp"
#include "../common/constexpr-math.hpp"
#include "../common/constants.hpp"

#include "../edge_detection/edge_detection.hpp" // added

Camera::Camera(const nlohmann::json &j, const Option &option)
{
    if (option.photon_map)
    {
        integrator = std::make_shared<PhotonMapper>(j);
    }
    else
    {
        integrator = std::make_shared<PathTracer>(j);
    }

    const nlohmann::json &c = j.at("cameras").at(option.camera_idx);

    image = Image(c.at("image"));
    if (c.find("film") != c.end())
        film = Film(image.width, image.height, c.at("film"));
    else
        film = Film(image.width, image.height);

    eye = c.at("eye");
    focal_length = c.at("focal_length").get<double>() / 1000.0;
    sensor_width = c.at("sensor_width").get<double>() / 1000.0;
    spp1 = c.at("spp1"); // modified
    spp2 = c.at("spp2"); // modified
    edge_threshold = c.at("edge_threshold");
    edge_threshold /= 255.0; // added
    approx_prob = c.at("approx_prob"); // added
    savename = c.at("savename");
    aperture_radius = (focal_length / getOptional(c, "f_stop", -1.0)) / 2.0;
    focus_distance = getOptional(c, "focus_distance", -1.0);

    if (c.find("look_at") != c.end())
    {
        glm::dvec3 look_at = c.at("look_at");
        lookAt(look_at);
        if (focus_distance < 0.0)
        {
            focus_distance = glm::distance(eye, look_at);
        }
    }
    else
    {
        forward = glm::normalize(c.at("forward").get<glm::dvec3>());
        up = glm::normalize(c.at("up").get<glm::dvec3>());
        left = glm::normalize(glm::cross(up, forward));
    }

    thin_lens = aperture_radius > 0.0 && focus_distance > 0.0;
}

void Camera::init_counter(){
    cnt_regular = 0;
    cnt_approx = 0;
    cnt_all = 0;
}

void Camera::samplePixel_first(size_t x, size_t y)
{
    double pixel_size = sensor_width / image.width;
    glm::dvec2 half_dim = glm::dvec2(image.width, image.height) * 0.5;

    Sampler::initiate(static_cast<uint32_t>(y * image.width + x));

    for(int i = 0; i < spp1; i++)
    {
        Sampler::setIndex(i);

        auto u = Sampler::get<Dim::PIXEL, 2>();
        glm::dvec2 px(x + u[0], y + u[1]);
        glm::dvec2 local = pixel_size * (half_dim - px);
        glm::dvec3 direction = glm::normalize(forward * focal_length + left * local.x + up * local.y);

        // Pinhole camera ray
        Ray ray(eye, direction, integrator->scene.ior);

        if (thin_lens)
        {
            // Thin lens camera ray for depth of field
            auto u = Sampler::get<Dim::LENS, 2>();
            glm::dvec2 aperture_sample = Sampling::uniformDisk(u[0], u[1]) * aperture_radius;
            glm::dvec3 focus_point = ray(focus_distance / glm::dot(ray.direction, forward));
            glm::dvec3 start = eye + left * aperture_sample.x + up * aperture_sample.y;
            ray = Ray(start, glm::normalize(focus_point - start), integrator->scene.ior);
        }
        cnt_regular++;
        film.deposit(px, integrator->sampleRay(ray));
        cnt_all++;
        if(cnt_all % 2048 == 0){
            std::printf("RISCV Sim: %d samples finished\n", cnt_all);
        }
    }
}

void Camera::samplePixel_second(size_t x, size_t y, int i1, int i2)
{
    double pixel_size = sensor_width / image.width;
    glm::dvec2 half_dim = glm::dvec2(image.width, image.height) * 0.5;

    Sampler::initiate(static_cast<uint32_t>(y * image.width + x));

    for(int i = i1; i < i2; i++)
    {
        Sampler::setIndex(i);

        auto u = Sampler::get<Dim::PIXEL, 2>();
        glm::dvec2 px(x + u[0], y + u[1]);
        glm::dvec2 local = pixel_size * (half_dim - px);
        glm::dvec3 direction = glm::normalize(forward * focal_length + left * local.x + up * local.y);

        // Pinhole camera ray
        Ray ray(eye, direction, integrator->scene.ior);

        if (thin_lens)
        {
            // Thin lens camera ray for depth of field
            auto u = Sampler::get<Dim::LENS, 2>();
            glm::dvec2 aperture_sample = Sampling::uniformDisk(u[0], u[1]) * aperture_radius;
            glm::dvec3 focus_point = ray(focus_distance / glm::dot(ray.direction, forward));
            glm::dvec3 start = eye + left * aperture_sample.x + up * aperture_sample.y;
            ray = Ray(start, glm::normalize(focus_point - start), integrator->scene.ior);
        }

        if(no_edge[y * image.width + x]){
            if(rand01() >= approx_prob){
                cnt_regular++;
                film.deposit(px, integrator->sampleRay(ray));
                cnt_all++;
            }
            else{
                cnt_approx++;
                film.deposit(px, average_window[y * image.width + x]);
                cnt_all++;
            }
        }
        else{
            cnt_regular++;
            film.deposit(px, integrator->sampleRay(ray));
            cnt_all++;
        }
        if(cnt_all % 2048 == 0){
            std::printf("RISCV Sim: %d samples finished\n", cnt_all);
        }
    }
}

void Camera::init_for_approx(){
    width = image.width; height = image.height;
    no_edge.assign(width * height, true);
    fig.assign(width * height, glm::dvec3(0.0));
    for (size_t y = 0; y < height; y++) {
        for (size_t x = 0; x < width; x++) {
            fig[y * width + x] = film.scan(x, y);
        }
    }
    average_window.assign(width * height, glm::dvec3(0.0));
}

void Camera::sampleImage()
{
    //step.0
    init_counter();

    // step.1
    for (size_t y = 0; y < image.height; y++)
    {
        for (size_t x = 0; x < image.width; x++)
        {
            samplePixel_first(x, y);
        }
    }

    // step2
    init_for_approx();
    edge_detection(edge_threshold);
    calc_average_window();

    // step3
    for (size_t y = 0; y < image.height; y++)
    {
        for (size_t x = 0; x < image.width; x++)
        {
            samplePixel_second(x, y, spp1, spp1 + spp2);
        }
    }

    // step4
    for (int y = 0; y < image.height; y++)
    {
        for (int x = 0; x < image.width; x++)
        {
            image(x, y) = film.scan(x, y);
        }
    }
}

void Camera::lookAt(const glm::dvec3& p)
{
    forward = glm::normalize(p - eye);
    left = glm::cross({0.0, 1.0, 0.0}, forward);
    left = glm::length(left) < C::EPSILON ? glm::dvec3(-1.0, 0.0, 0.0) : glm::normalize(left);
    up = glm::normalize(glm::cross(forward, left));
}

void Camera::capture()
{
    std::printf("\n");
    std::printf("----------------------------| MAIN RENDERING PASS |----------------------------\n");
    std::printf("\n");
    std::printf("Samples per pixel: %lu + %lu\n\n", spp1, spp2);
    sampleImage();
    saveImage();
    std::printf("Regular routine: %d  Approx routine: %d\n\n", cnt_regular, cnt_approx);
}