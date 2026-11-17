# Ztest for P02's nvs::Zephyr on native_sim (run with Twister, see docs/contributing.md).

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(p02_test_nvs)

# P02's shared code: the driver under test lives in main/ (shared with the ESP-IDF build).
set(P02_MAIN ${CMAKE_CURRENT_SOURCE_DIR}/../../../../main)
target_include_directories(app PRIVATE ${P02_MAIN}/include)

target_sources(app PRIVATE
    src/main.cpp
    ${P02_MAIN}/source/driver/nvs/zephyr.cpp
)
