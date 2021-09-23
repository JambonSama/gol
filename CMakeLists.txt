cmake_minimum_required(VERSION 3.15)
project(gol LANGUAGES CXX)

find_package(freetype CONFIG REQUIRED)
find_package(SFML COMPONENTS system window graphics CONFIG REQUIRED)
add_executable(gol main.cpp)

target_link_libraries(gol PRIVATE freetype sfml-system sfml-graphics sfml-window)
target_compile_features(gol PRIVATE cxx_std_20)