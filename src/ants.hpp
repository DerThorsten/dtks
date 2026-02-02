#pragma once

#include "conf.hpp"
#include <array>
#include <vector>
#include <format>
#include <random>
#include <iostream>
#include <math.h>
// pair
#include <utility>
#include "image.hpp"

namespace dtks{

    float to_radians(float degrees);

    struct Parameters
    {
        std::array<int, 2> shape = {1000, 1000};
        std::size_t n_ants = 1000;
        float pheromone_deposit_amount = 1.0f;
        float nest_pheromone_deposit_amount = 100.0f;
        float pheromone_evaporation_rate = 0.003f;
        float beta_uniformity = 0.0001f;
        float beta_straight = 0.00f;
        float sigma_diffusion = 0.4f;


        float only_wall_turn_angle = to_radians(45.0f);

        std::size_t sense_distance = 15; // pixels
        float sense_angle = to_radians(30.0f);
        float turn_angle = to_radians(20.0f);
        float random_wiggle =  M_PI / 30;
        float wall_repellent_strength = 100.5f;
        float pheromone_truncation_threshold = 0.0001f;
        long seed = 42;
        bool infinite_food = true;
    };





    struct Ant
    {
        std::array<float, 2> position = {0.0f, 0.0f};
        std::array<int,  2>  grid_position = {0, 0};
        std::size_t          grid_index = 0;
        float direction = 0.0f;
        bool carrying_food = false;
        std::size_t age = 0;
        std::size_t time_since_home = 0;
        std::size_t time_since_food = 0;
        std::size_t last_turn_direction = 1; // 1 left, 2 right
        float pheromone_drop_multiplier = 1.0f;

    };



    class AntSimulation
    {
        public:
        friend struct Ant;

        AntSimulation(Parameters params);


        const Parameters& parameters() const;

        void step();
        void nest_and_food_emit();
        void update_ant_pos(Ant & ant);

        template<typename T>
        inline void wrap(std::array<T, 2> & position)
        {
            if(position[0] < 0) position[0] += params_.shape[0];
            if(position[0] >= params_.shape[0]) position[0] -= params_.shape[0];
            if(position[1] < 0) position[1] += params_.shape[1];
            if(position[1] >= params_.shape[1]) position[1] -= params_.shape[1];
        }

        std::array<int, 2> round_and_wrap(const std::array<float, 2> & position);

        void draw(uint8_t * display_image);

        void ready();

        Image2d<uint8_t> & food_map();
        Image2d<uint8_t> & nest_map();
        Image2d<uint8_t> & is_land();


        inline std::size_t food_collected() const { return food_collected_; }
        inline std::size_t food_at_nest() const { return food_at_nest_; }

        private:

        
            Parameters params_;
            std::vector<Ant> ants_;

            MultiChannelImage2d<double, 2> pheromone_map_;  // 0: home, 1: food
            MultiChannelImage2d<double, 2> temp_pheromone_map_;
            Image2d<uint8_t> food_map_;
            Image2d<uint8_t> nest_map_;
            Image2d<uint8_t> is_land_;

            std::vector<std::array<int, 2>> nest_positions_;

            // how much food did we collect?
            std::size_t food_collected_ = 0;
            std::size_t food_at_nest_ = 0;


            // distribution for random direction changes
            std::mt19937 generator_;
            std::uniform_real_distribution<float> direction_change_dist_;


    };

}