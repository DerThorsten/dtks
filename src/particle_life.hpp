
#pragma once
#include <vector>
#include <random>
#include "image.hpp"

namespace dtks{
    
    struct Particle{
        TinyVector<float, 2> position;
        TinyVector<float, 2> velocity;
        TinyVector<float, 2> force = {0.0f, 0.0f};
        std::uint8_t type;
    };
    
    struct ParticleLifeParameters
    {
        std::uint8_t n_particle_types = 4;
        std::size_t n_particles_per_type = 10000;
        std::array<int, 2> shape = {1024, 1024} ;
        std::size_t max_range = 64;
        std::size_t seed = 42;
        std::vector<TinyVector<uint8_t, 3>> type_colors;
        dtks::Image2d<float> interaction_strength;

    };
    
    struct ParticleSimulation
    {
        ParticleSimulation(const ParticleLifeParameters& params);

        ParticleLifeParameters params_;
        float max_range_sq_;
        // for fast neighbor search, we divide the space into grid cells
        Image2d<std::vector<std::size_t>> particle_in_grid_cell_;
        std::vector<Particle> particles_;

        // rand generator
        std::mt19937 generator_;

        void step();


        // helper
        inline std::size_t grid_cell_index(const TinyVector<float, 2>& position)
        {
            int cell_x = static_cast<int>(position[0] / params_.max_range );
            int cell_y = static_cast<int>(position[1] / params_.max_range );
            return cell_y * particle_in_grid_cell_.shape()[0] + cell_x;
        }
        inline TinyVector<int, 2> grid_cell(const TinyVector<float, 2>& position)
        {
            int cell_x = static_cast<int>(position[0] / params_.max_range );
            int cell_y = static_cast<int>(position[1] / params_.max_range );
            return TinyVector<int, 2>{cell_x, cell_y};
        }

    };


}// namespace dtks