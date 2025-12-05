cmake_minimum_required(VERSION 3.21)
project(JobSystem VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE PROJECT_SOURCES "Source/JobSystem/*.cpp" "Source/JobSystem/*.h")
add_executable(JobSystem ${PROJECT_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE "Source/JobSystem")

if(WIN32)
    set(PLATFORM_NAME "Win64")
elseif(UNIX)
    set(PLATFORM_NAME "Linux")
endif()

set(BASE_OUTPUT_DIR "${CMAKE_BINARY_DIR}/Binaries/${PLATFORM_NAME}")
set(INTERMEDIATE_DIR "${CMAKE_BINARY_DIR}/Intermediate/${PLATFORM_NAME}")

    set_target_properties(JobSystem PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${BASE_OUTPUT_DIR}/$<CONFIG>"
        LIBRARY_OUTPUT_DIRECTORY "${BASE_OUTPUT_DIR}/$<CONFIG>"
        ARCHIVE_OUTPUT_DIRECTORY "${BASE_OUTPUT_DIR}/$<CONFIG>"
        OBJECT_OUTPUT_DIRECTORY "${INTERMEDIATE_DIR}/$<CONFIG>/JobSystem"
    )