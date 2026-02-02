#include "particle_life.hpp"
#include <sstream>
#include <iostream>
namespace dtks{


    float distance(float r, float a, float beta=0.3f) {
        if(r<beta){
            return  r/beta -1.0f;
        }
        else if (beta<r && r<1.0f){
            return a * ( 1.0f - std::abs(2*r -1 -  beta)/(1.0f - beta));
        }
        else{
            return 0.0f;
        }
    };
    
    ParticleSimulation::ParticleSimulation(const ParticleLifeParameters& params)
    :   params_(params),
        particle_in_grid_cell_({
            int(std::ceil(float(params.shape[0]) / params.max_range)),
            int(std::ceil(float(params.shape[1]) / params.max_range))
        }
        ), 
        particles_(params.n_particle_types * params.n_particles_per_type),
        generator_(params.seed)
    {

        //  pre-reserve space in each grid cell
        for(std::size_t i=0; i<particle_in_grid_cell_.size(); ++i)
        {
            particle_in_grid_cell_[i].reserve(100);
        }

        std::cout<<"shape of the grid: "<<particle_in_grid_cell_.shape()[0]<<" "<<particle_in_grid_cell_.shape()[1]<<"\n";
        max_range_sq_ = float(params.max_range * params.max_range);
        std::size_t particle_index = 0;
        for(std::size_t i=0; i<params.n_particle_types; ++i)
        {
            for(std::size_t j=0; j<params.n_particles_per_type; ++j)
            {
                auto & particle = particles_[i * params_.n_particles_per_type + j];
                std::uniform_real_distribution<float> dist_x(0.0f, float(params_.shape[0]));
                std::uniform_real_distribution<float> dist_y(0.0f, float(params_.shape[1]));
                std::uniform_real_distribution<float> dist_vel(-1.0f, 1.0f);
                particle.position = { dist_x(generator_), dist_y(generator_) };
                particle.velocity = { 0,0 };
                particle.type = static_cast<std::uint8_t>(i);
                auto gi = grid_cell_index(particle.position);
                auto gc = grid_cell(particle.position);
                if(gi >= particle_in_grid_cell_.size())
                {
                    std::cout<<"position: "<<particle.position[0]<<" "<<particle.position[1]<<"\n";
                    std::cout<<"grid cell: "<<gc[0]<<" "<<gc[1]<<"\n";

                    std::cout<<"Error: grid cell index out of bounds: "<<gi<<" size: "<<particle_in_grid_cell_.size()<<"\n";
                    throw std::runtime_error("Grid cell index out of bounds");
                }
                particle_in_grid_cell_[gi].push_back(particle_index);
                ++particle_index;
            }
        }

        // if no colors are provided, generate some random ones
        if(params_.type_colors.size() < params_.n_particle_types)
        {
            std::uniform_int_distribution<int> color_dist(0, 255);
            for(std::size_t i=params_.type_colors.size(); i<params_.n_particle_types; ++i)
            {
                params_.type_colors.push_back    (TinyVector<uint8_t, 3>{
                    static_cast<uint8_t>(color_dist(generator_)),
                    static_cast<uint8_t>(color_dist(generator_)),
                    static_cast<uint8_t>(color_dist(generator_))
                }); 
            }
        }
    }

    // step function
    void ParticleSimulation::step()
    {

        auto wrap = [](float coord, int max_coord)
        {
            while(coord < 0.0f)  coord += float(max_coord);
            while(coord >= float(max_coord))  coord -= float(max_coord);
            return coord;
        };

        // update positions
        for(std::size_t particle_index=0; particle_index<particles_.size(); ++particle_index)
        {
            auto & particle = particles_[particle_index];
        
            std::size_t n_neighbors = 0;



            const auto grid_coord = this->grid_cell(particle.position);

            for(auto yy=-1; yy<=1; ++yy)
            {
                for(auto xx=-1; xx<=1; ++xx)
                {

                    auto neighbor_cell_x = wrap(grid_coord[0] + xx, particle_in_grid_cell_.shape()[0]);
                    auto neighbor_cell_y = wrap(grid_coord[1] + yy, particle_in_grid_cell_.shape()[1]);
                    auto & neighbor_cell = particle_in_grid_cell_[neighbor_cell_y * particle_in_grid_cell_.shape()[0] + neighbor_cell_x];

                    for(auto neighbor_index : neighbor_cell)
                    {
                        if(neighbor_index > particle_index)
                        {
                            continue;
                        }
                        

                        auto & neighbor_particle = particles_[neighbor_index];
                        if(&neighbor_particle != &particle)
                        {
                            TinyVector<float, 2> diff = neighbor_particle.position - particle.position;
                            // apply periodic boundary conditions
                            for(std::size_t d=0; d<2; ++d)
                            {
                                if(diff[d] > float(params_.shape[d]) / 2.0f)
                                    diff[d] -= float(params_.shape[d]);
                                else if(diff[d] < -float(params_.shape[d]) / 2.0f)
                                    diff[d] += float(params_.shape[d]);
                            }
                            const float dist_sq = diff[0]*diff[0] + diff[1]*diff[1];
                            if(dist_sq < max_range_sq_)
                            {
                                n_neighbors++;
                                const float dist = std::sqrt(dist_sq) + 1e-6f;
                                float interaction_strength_ab = params_.interaction_strength(particle.type,neighbor_particle.type);
                                float interaction_strength_ba = params_.interaction_strength(neighbor_particle.type, particle.type);
                                const auto unit_diff_times_range =  params_.max_range  * (diff / dist); 
                                
                                const float rdist = dist / float(params_.max_range);
                               
                                float force_magnitude_ab = distance(rdist, interaction_strength_ab);
                                particle.force[0] += force_magnitude_ab * unit_diff_times_range[0];
                                particle.force[1] += force_magnitude_ab * unit_diff_times_range[1];

                                float force_magnitude_ba = distance(rdist, interaction_strength_ba);
                                neighbor_particle.force[0] -= force_magnitude_ba * unit_diff_times_range[0];
                                neighbor_particle.force[1] -= force_magnitude_ba * unit_diff_times_range[1];

                            }
                        }
                    }
                }
            }
        }
        // clear grid cells
        for(std::size_t i=0; i<particle_in_grid_cell_.size(); ++i)
        {
            particle_in_grid_cell_[i].clear();
        }
        // apply forces and update positions
        for(auto & particle : particles_)
        {   
            auto dt = 0.02f;
            auto friction = 0.5f;
            particle.velocity[0]  = particle.velocity[0] * friction + particle.force[0] * dt;
            particle.velocity[1]  = particle.velocity[1] * friction + particle.force[1] * dt;

            particle.position[0] += particle.velocity[0] * dt;
            particle.position[1] += particle.velocity[1] * dt;


            //reset force
            particle.force = {0.0f, 0.0f};

            // apply periodic boundary conditions
            for(std::size_t d=0; d<2; ++d)
            {
                particle.position[d] = wrap(particle.position[d], params_.shape[d]);    
            }
            // assert position is within bounds
            for(std::size_t d=0; d<2; ++d)
            {
                if(particle.position[d] < 0.0f || particle.position[d] >= float(params_.shape[d]))
                {
                    std::cout<<"particle position out of bounds: "<<particle.position[0]<<" "<<particle.position[1]<<" shape: "<<params_.shape[0]<<" "<<params_.shape[1]<<"\n";
                    throw std::runtime_error("Particle position out of bounds after wrapping"); 
                }
            }
        }


        // re-insert into grid cells
        for(std::size_t i=0; i<particles_.size(); ++i)
        {
            const auto grid_coord = this->grid_cell(particles_[i].position);
            //std::cout<<"grid coord: "<<grid_coord[0]<<" "<<grid_coord[1]<<" shape: "<<particle_in_grid_cell_.shape()[0]<<" "<<particle_in_grid_cell_.shape()[1]<<"\n";
            particle_in_grid_cell_[grid_coord[1] * particle_in_grid_cell_.shape()[0] + grid_coord[0]].push_back(i);
        }
    }



}// namespace dtks