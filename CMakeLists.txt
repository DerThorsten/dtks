

cmake_minimum_required(VERSION 3.15...3.27)
project(dtks) # Replace 'my_project' with the name of your project

if (CMAKE_VERSION VERSION_LESS 3.18)
  set(DEV_MODULE Development)
else()
  set(DEV_MODULE Development.Module)
endif()


# Use modern C++
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# if we are **NOT** on emscripten, we can use raylib

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    set(USE_RAYLIB ON)
else()
    set(USE_RAYLIB OFF)
endif()


if(USE_RAYLIB)

  include(FetchContent)
  set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(
      raylib
      GIT_REPOSITORY https://github.com/raysan5/raylib.git
      GIT_TAG 5.0   # or a newer tag if available
  )
  FetchContent_MakeAvailable(raylib)
endif()



find_package(Python 3.9 COMPONENTS Interpreter ${DEV_MODULE} REQUIRED)


# Detect the installed nanobind package and import it into CMake
execute_process(
  COMMAND "${Python_EXECUTABLE}" -m nanobind --cmake_dir
  OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE nanobind_ROOT)
find_package(nanobind CONFIG REQUIRED)





nanobind_add_module(dtks_ext 
  src/dtks_ext.cpp
  src/ants.cpp
)

if(USE_RAYLIB)
  target_link_libraries(dtks_ext PRIVATE raylib)
endif()


add_custom_command(TARGET dtks_ext POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:dtks_ext> ${CMAKE_SOURCE_DIR}/src/dtks
)