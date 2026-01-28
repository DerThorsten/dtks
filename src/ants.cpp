#include <array>
#include <vector>
#include <format>
#include <random>
#include <iostream>
#include <math.h>
#include <utility>
#include "ants.hpp"

namespace ant{

    float to_radians(float degrees)
    {
        return degrees * M_PI / 180.0f;
    }



    AntSimulation::AntSimulation(Parameters params) : 
        params_(params),
        ants_(params_.n_ants),
        pheromone_map_(params_.shape, {0.0, 0.0}),
        temp_pheromone_map_(params_.shape, {0.0, 0.0}),
        is_land_(params_.shape, 1),
        food_map_(params_.shape, 0),
        nest_map_(params_.shape, 0),
        nest_positions_(),
        generator_(params_.seed),
        direction_change_dist_(-0.1f, 0.1f)
    {

    }


    const Parameters& AntSimulation::parameters() const { return params_; }

    void AntSimulation::step()
    {
        // Simulation step logic goes here
        for(auto & ant : ants_)
        {
            
            // update pos
            update_ant_pos(ant);

            // drop pheromone at last position
            pheromone_map_[ant.grid_position][int(ant.carrying_food)] += params_.pheromone_deposit_amount * ant.pheromone_drop_multiplier;
        }
        nest_and_food_emit();

        if(params_.sigma_diffusion > 0.0001f)
        {
            // diffuse pheromones
            gaussianSeparableWrap(
                pheromone_map_,
                temp_pheromone_map_,
                pheromone_map_,
                1,
                params_.sigma_diffusion
            );
        }
        //evaporate pheromones
        //pheromone_map_ *= (1.0f - params_.pheromone_evaporation_rate);
        auto ptr = reinterpret_cast<double*>(pheromone_map_.data());
        for(std::size_t i = 0; i < pheromone_map_.size() * 2; ++i)
        {
            ptr[i] *= (1.0f - params_.pheromone_evaporation_rate);
            if(ptr[i] < params_.pheromone_truncation_threshold){
                ptr[i] = 0.0f;
            }                    
        }

    }

    void AntSimulation::nest_and_food_emit()
    {
        for(auto nest_pos : nest_positions_)
        {
            pheromone_map_[nest_pos][0] = params_.nest_pheromone_deposit_amount;
        }
        for(auto i=0; i<food_map_.size(); ++i)
        {
            if(food_map_[i] > 0)
            {
                pheromone_map_[i][1] = params_.nest_pheromone_deposit_amount;
            }
        }
    }

    void AntSimulation::update_ant_pos(Ant & ant)
    {   
        // randomly change direction a bit

        // ant.direction += direction_change_dist_(generator_);


        
        constexpr std::size_t n_directions = 3;

        TinyVector<float, n_directions>   angles = {0.0f, -params_.sense_angle, params_.sense_angle};
        TinyVector<float, n_directions>   pheromones;
        TinyVector<uint8_t, n_directions> land_at_distance;
        TinyVector<uint8_t, n_directions> land_nh;
        TinyVector<float, n_directions>    probabilities;

        // with food, look for home,
        // without food, look for food
        bool any_target = false;
        int target_index = 0;
        TinyVector<uint8_t, n_directions> is_target(0);

        std::size_t is_land_nh_count = 0;
        std::size_t is_land_at_distance_count = 0;
        for(std::size_t i = 0; i < n_directions; ++i)
        {
            float sense_angle = ant.direction + angles[i];
            float sense_x = ant.position[0] + params_.sense_distance * cos(sense_angle);
            float sense_y = ant.position[1] + params_.sense_distance * sin(sense_angle);
            float nh_x = ant.position[0] + cos(sense_angle);
            float nh_y = ant.position[1] + sin(sense_angle);
            auto nh_xy = round_and_wrap({nh_x, nh_y});
            auto sense_xy = round_and_wrap({sense_x, sense_y});
            auto pheromone = pheromone_map_(sense_xy[0], sense_xy[1])[int(!ant.carrying_food)];
            auto is_land = is_land_(sense_xy[0], sense_xy[1]);
            auto is_land_nh = is_land_(nh_xy[0], nh_xy[1]);
            is_land_at_distance_count += is_land;
            is_land_nh_count += is_land_nh;
            if(ant.carrying_food)
            {
                // looking for home
                if(nest_map_(sense_xy[0], sense_xy[1]) > 0)
                {
                    is_target[i] = 1;
                    any_target = true;
                    target_index = i;
                }
            }
            else
            {
                // looking for food
                if(food_map_(sense_xy[0], sense_xy[1]) > 0)
                {
                    is_target[i] = 1;
                    any_target = true;
                    target_index = i;
                }
            }
            
            land_nh[i] = is_land_nh;
            pheromones[i] = pheromone;
            land_at_distance[i] = is_land;
        }
        //std::cout<<"is_land_nh_count: "<<is_land_nh_count<<" is_land_at_distance_count: "<<is_land_at_distance_count<<"\n";
        ant.pheromone_drop_multiplier = float( is_land_at_distance_count) / float(n_directions);
        ant.pheromone_drop_multiplier *= float(is_land_nh_count) / float(n_directions);

        if (is_land_nh_count == 0)
        {
            // all directions blocked right in front of us, so just turn around
            ant.pheromone_drop_multiplier = 0.0f;
            if(ant.last_turn_direction == 1)
                ant.direction -= params_.only_wall_turn_angle;
            else
                ant.direction += params_.only_wall_turn_angle;
        }
        else{
                
            if(any_target)
            {
                // go towards target
                if(target_index == 0)
                {
                    // go forward, do nothing
                }
                else if(target_index == 1)
                {
                    // turn left
                    ant.last_turn_direction = 1;
                    ant.direction -= params_.turn_angle;
                }
                else if(target_index == 2)
                {
                    // turn right
                    ant.last_turn_direction = 2;
                    ant.direction += params_.turn_angle;
                }
            }
            else
            {

                // bias to make choice more uniform. Also if all pheromones are zero, this will yield an complete uniform choice
                pheromones += params_.beta_uniformity;
                pheromones[0] += params_.beta_straight; // bias to go straight

                // convert to probabilities
            
                
                
                for(std::size_t i = 0; i < n_directions; ++i)
                {
                    probabilities[i] = pheromones[i];
                    //probabilities[i] = pheromones[i] /(1.0f +  std::sqrt(repellents[i]));
                    probabilities[i] = probabilities[i] < 0.0f ? 0.0f : probabilities[i];
                    float turn_penalty = 0.5f;  // try 0.2–0.5
                    if(i != 0)
                    {
                        probabilities[i] *= (1.0f - turn_penalty);
                    }
                }

                probabilities *= land_nh; // zero out non-land directions right in front of us    

                // random value based on probabilities (with C++ standard library)
                std::discrete_distribution<int> direction_dist(
                    probabilities.begin(),
                    probabilities.end()
                );

                int choice = direction_dist(generator_);
                if(choice == 0)
                {
                    // go forward, do nothing
                }
                else if(choice == 1)
                {
                    // turn left
                    ant.last_turn_direction = 1;
                    ant.direction -= params_.turn_angle;
                }
                else if(choice == 2)
                {
                    // turn right
                    ant.last_turn_direction = 2;
                    ant.direction += params_.turn_angle;
                }
            }
            // step forward
            float step_size = 1.0f;
            ant.position[0] += step_size * cos(ant.direction);
            ant.position[1] += step_size * sin(ant.direction);
            this->wrap(ant.position); 
            ant.grid_position = round_and_wrap(ant.position);



            // update age
            ant.age += 1;
            

            
            if(ant.carrying_food)
            {
                // check for nest
                if(nest_map_(ant.grid_position[0], ant.grid_position[1]) > 0)
                {
                    ant.carrying_food = false;  
                    ant.direction += M_PI; // turn around
                    this->food_at_nest_ += 1;
                }
                else
                {
                    ant.time_since_home += 1;
                }
            }
            else
            {
                // check for food
                if(food_map_(ant.grid_position[0], ant.grid_position[1]) > 0)
                {
                    ant.carrying_food = true;  
                    ant.direction += M_PI; // turn around
                    this->food_collected_ += 1;
                }
                else
                {
                    ant.time_since_food += 1;
                }
            }
        }

    }

    std::array<int, 2> AntSimulation::round_and_wrap(const std::array<float, 2> & position)
    {
        std::array<int, 2> pos_rounded = {
            static_cast<int>(std::lround(position[0])),
            static_cast<int>(std::lround(position[1]))
        };
        this->wrap(pos_rounded);
        return pos_rounded;
    }


    void AntSimulation::draw(uint8_t * display_image)
    {


        // auto min_max = channel_min_max(pheromone_map_);
        
        // display_image_ *= 0; // clear image
        auto size = params_.shape[0] * params_.shape[1];
        for(auto i=0; i<size; ++i)
        {
            auto pixel = display_image + i * 4;
            pixel[0] = 0;
            pixel[1] = 0;
            pixel[2] = 0;
            pixel[3] = 255;

            // pheromone visualization
            const auto max_val = 10.0f;

            // normalized pheromone values
            const auto & home_pheromone = pheromone_map_[i][0];
            const auto & food_pheromone = pheromone_map_[i][1];
            float truncated_home = home_pheromone > max_val ? max_val : home_pheromone;
            float truncated_food = food_pheromone > max_val ? max_val : food_pheromone;

            pixel[0] = static_cast<uint8_t>(truncated_food * 255.0/max_val);   
            pixel[1] = static_cast<uint8_t>(truncated_home * 255.0/max_val); 
            pixel[2] = 0;
            pixel[3] = 255;  

            if(nest_map_[i] > 0)
            {
                pixel[0] = 255;
                pixel[1] = 255;
                pixel[2] = 255;
                pixel[3] = 255;  
            }

            if(food_map_[i] > 0)
            {
                pixel[0] = 255;
                pixel[1] = 0;
                pixel[2] = 0;
                pixel[3] = 255;  
            }

            if(is_land_[i] == 0)
            {
                pixel[0] = 50;
                pixel[1] = 50;
                pixel[2] = 50;
                pixel[3] = 255;  
            }
        }





        // draw ants
        for(const auto & ant : ants_)
        {
            auto xy = ant.grid_position;
            auto pixel = display_image + (xy[1] * params_.shape[0] + xy[0]) * 4;
            if(ant.carrying_food)
            {
                // red if carrying food
                pixel[0] = 0;
                pixel[1] = 0;
                pixel[2] = 255;
            }
            else{
                // green if not carrying food
                pixel[0] = 0;
                pixel[1] = 0;
                pixel[2] = 255;
                pixel[3] = 255;   // Alpha
            }
        }
    }

    void AntSimulation::ready()
    {
        // Prepare simulation (e.g., initialize ants)
        // each and needs to be placed at the nest location

        for(int y = 0; y < params_.shape[1]; ++y)
        {
            for(int x = 0; x < params_.shape[0]; ++x)
            {
                if(nest_map_(x, y) > 0)
                {
                    nest_positions_.push_back({x, y});
                }
            }   
        }

        std::default_random_engine generator;
        std::uniform_int_distribution<std::size_t> distribution(0, nest_positions_.size() - 1);  

        // float direction between 0 and 2pi
        std::uniform_real_distribution<float> angle_distribution(0.0f, 2.0 * M_PI);

        for(auto & ant : ants_)
        {
            auto & pos = nest_positions_[distribution(generator)];
            ant.position = {static_cast<float>(pos[0]), static_cast<float>(pos[1])};
            ant.direction = angle_distribution(generator);
            ant.carrying_food = false;
            ant.age = 0;
        }            
    }

    Image2d<uint8_t> & AntSimulation::food_map()  { return food_map_; }
    Image2d<uint8_t> & AntSimulation::nest_map()  { return nest_map_; }
    Image2d<uint8_t> & AntSimulation::is_land()  { return is_land_; }

      

}