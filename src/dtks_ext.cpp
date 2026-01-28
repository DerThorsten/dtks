#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/stl/array.h>

#include "conf.hpp"

namespace nb = nanobind;
using namespace nb::literals;


#ifdef USE_RAYLIB
#include "raylib.h"
#endif

#include <array>
#include <vector>
#include <format>
#include <random>
#include <iostream>
#include <math.h>
// pair
#include <utility>
#include "image.hpp"
#include "ants.hpp"



namespace nb = nanobind;
using namespace nb::literals;



using ImgUInt8 = nb::ndarray<uint8_t, nb::shape<-1, -1>, nb::device::cpu>;

using RgbaImgUInt8 =  nb::ndarray<uint8_t, nb::shape<-1, -1, 4>, nb::device::cpu>;

void export_ant_simulation(nb::module_& m)
{


    nb::class_<ant::AntSimulation>(m, "AntSimulation")
        .def(nb::init<ant::Parameters>())
        .def("step", &ant::AntSimulation::step)
        .def("ready", &ant::AntSimulation::ready)
        .def("parameters", &ant::AntSimulation::parameters,  nb::rv_policy::reference)

        .def("set_food_map", [](ant::AntSimulation & self, ImgUInt8 array){
            // check that shape matches
            if(array.shape(0) != self.parameters().shape[0] || array.shape(1) != self.parameters().shape[1])
            {
                throw std::runtime_error("Shape of food_map does not match simulation shape");
            }
            // copy data
            for(int y = 0; y < self.parameters().shape[1]; ++y)
            {
                for(int x = 0; x < self.parameters().shape[0]; ++x)
                {
                    self.food_map()(x, y) = array(x, y);
                }
            }
        })

        .def("set_nest_map", [](ant::AntSimulation & self, ImgUInt8 array){
            // check that shape matches
            if(array.shape(0) != self.parameters().shape[0] || array.shape(1) != self.parameters().shape[1])
            {
                throw std::runtime_error("Shape of nest_map does not match simulation shape");
            }
            // copy data
            for(int y = 0; y < self.parameters().shape[1]; ++y)
            {
                for(int x = 0; x < self.parameters().shape[0]; ++x)
                {
                    self.nest_map()(x, y) = array(x, y);
                }
            }
        })

        .def("set_is_land", [](ant::AntSimulation & self, ImgUInt8 array){
            // check that shape matches
            if(array.shape(0) != self.parameters().shape[0] || array.shape(1) != self.parameters().shape[1])
            {
                throw std::runtime_error("Shape of is_land does not match simulation shape");
            }
            // copy data
            for(int y = 0; y < self.parameters().shape[1]; ++y)
            {
                for(int x = 0; x < self.parameters().shape[0]; ++x)
                {
                    self.is_land()(x, y)  = array(x, y);
                }
            }
        })

        .def("draw",[](ant::AntSimulation & self, RgbaImgUInt8 & img){
            // get the ptr
            auto ptr = reinterpret_cast<uint8_t*>(img.data());

            self.draw(ptr);

        })
        #ifdef USE_RAYLIB
        .def("run_with_raylib", [](ant::AntSimulation & self, std::size_t n_steps_per_draw){
            
            ant::MultiChannelImage2d<uint8_t, 4> display_image(self.parameters().shape, {0,0,0,0});

            InitWindow(self.parameters().shape[0], self.parameters().shape[1], "Ant Simulation");
            SetTargetFPS(60);

            // Create an empty image that matches the format
            Image img = {
                .data = reinterpret_cast<void*>(display_image.data()),
                .width = self.parameters().shape[0],
                .height = self.parameters().shape[1],
                .mipmaps = 1,
                .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
            };
            // Upload once
            Texture2D texture = {0};
            texture = LoadTextureFromImage(img);


            bool simulation_started = false;

            

            while (!WindowShouldClose())
            {
                if (IsKeyPressed(KEY_SPACE)) {
                    simulation_started = true;
                }

                if (simulation_started) {
                    for(auto i=0; i<n_steps_per_draw ; ++i)
                    {
                        self.step();  
                    }
                }
                self.draw(reinterpret_cast<uint8_t*>(display_image.data()));
                
                // Update texture with new image data
                
                BeginDrawing();
                UpdateTexture(texture, reinterpret_cast<const void*>(display_image.data()));
                
                // clear background
                ClearBackground(RAYWHITE);


                // draw texture to screen
                DrawTexture(texture, 0, 0, WHITE);

                DrawText(std::format("Food collected: {}", self.food_collected()).c_str(), 10, 60, 20, DARKGRAY);
                DrawText(std::format("Food at nest: {}", self.food_at_nest()).c_str(), 10, 80, 20, DARKGRAY);


                // draw fps
                DrawFPS(10, 10);


                EndDrawing();



            }

            CloseWindow();
        }, nb::arg("n_steps_per_draw") = 1
        )
        #endif
    ;

    nb::class_<ant::Parameters>(m, "Parameters")
        .def(nb::init<>())
        .def_rw("shape", &ant::Parameters::shape)
        .def_rw("n_ants", &ant::Parameters::n_ants)
        .def_rw("pheromone_deposit_amount", &ant::Parameters::pheromone_deposit_amount)
        .def_rw("nest_pheromone_deposit_amount", &ant::Parameters::nest_pheromone_deposit_amount)
        .def_rw("pheromone_evaporation_rate", &ant::Parameters::pheromone_evaporation_rate)
        .def_rw("sense_distance", &ant::Parameters::sense_distance)
        .def_rw("sense_angle", &ant::Parameters::sense_angle)
        .def_rw("turn_angle", &ant::Parameters::turn_angle)
        .def_rw("wall_repellent_strength", &ant::Parameters::wall_repellent_strength);
};







NB_MODULE(dtks_ext, m) {
    m.doc() = "This is the kitchen sink";
    export_ant_simulation(m);
}