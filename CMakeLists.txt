project(MS-ADPCM)
cmake_minimum_required(VERSION 3.0)

file(GLOB SRC *.cpp *.h)

add_executable(ms_adpcm ${SRC})