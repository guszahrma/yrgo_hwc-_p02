cmake_minimum_required(VERSION 3.16)

# Test target.
set(TARGET testsuite)

# C++ compiler. Must be set before project() so it takes effect during
# compiler detection instead of whatever compiler is first on PATH
# (e.g. ESP-IDF's esp-clang when built from an ESP-IDF shell).
project(${TARGET} CXX C)

set(CMAKE_CXX_COMPILER /usr/bin/g++)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# Place all build output in the root build directory.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Main firmware directory.
set(MAIN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../main)

# Firmware header directory.
set(FW_INC ${MAIN_DIR}/include)

# Firmware source directory.
set(FW_SRC ${MAIN_DIR}/source)

# Test files.
set(TST_FILES
    source/testsuite.cpp
    source/gpio/gpio_mock.c
)

# Source files.
set(SRC_FILES
    ${FW_SRC}/driver/gpio/esp32s3.cpp
)

# All source files.
set(ALL_FILES ${TST_FILES} ${SRC_FILES})

# Build the test suite.
add_executable(${TARGET} ${ALL_FILES})

target_include_directories(${TARGET} PRIVATE
    include
    ${FW_INC}
)

target_compile_options(${TARGET} PRIVATE -Wall -Werror)
target_compile_definitions(${TARGET} PUBLIC TESTSUITE)
target_compile_definitions(${TARGET} PUBLIC TRACE_ENABLED)


add_subdirectory(googletest)
target_link_libraries(${TARGET} PRIVATE gtest gtest_main)

# Run the test suite.
add_custom_target(run ALL
    COMMAND $<TARGET_FILE:${TARGET}>
    DEPENDS ${TARGET}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Clean the test suite (available by default as `cmake --build . --target clean`).
