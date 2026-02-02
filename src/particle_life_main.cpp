
#include "particle_life.hpp"
#include <random>

// raylib
#include "raylib.h"

int main()
{
    auto param = dtks::ParticleLifeParameters();
    param.n_particle_types = 10;
    param.n_particles_per_type = 2000;   

    std::mt19937 generator(param.seed);

    dtks::Image2d<float> interaction_strength({param.n_particle_types, param.n_particle_types}, 0.0f);

    std::uniform_real_distribution<float> interaction_dist(-1,1);
    for(std::size_t i=0; i<param.n_particle_types; ++i)
    {
        for(std::size_t j=0; j<param.n_particle_types; ++j)
        {
        
            interaction_strength(i, j) = interaction_dist(generator);
            
        }
    }
        param.interaction_strength = interaction_strength;

    dtks::ParticleSimulation sim(param);


    // raylib init
    InitWindow(param.shape[0], param.shape[1], "raylib + CMake + C++");
    SetTargetFPS(60);
    while (!WindowShouldClose())
    {
        // Simulation step
        sim.step();

        // Drawing
        BeginDrawing();
        ClearBackground(BLACK);

        // draw particles
        for(const auto & particle : sim.particles_)
        {
            DrawRectangle(
                particle.position[0] - 1.5f,
                particle.position[1] - 1.5f,
                3.0f,
                3.0f,
                Color{
                    sim.params_.type_colors[particle.type][0],
                    sim.params_.type_colors[particle.type][1],
                    sim.params_.type_colors[particle.type][2],
                    255
                }
            );
        }

        // draw fps
        DrawFPS(10, 10);

        EndDrawing();
    }

}
