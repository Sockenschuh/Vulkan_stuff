cmake_minimum_required(VERSION 3.10)

# Project name
project(vulkan_tutorial)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

set(SOURCE_FILES src/main.cpp src/simple_logger.cpp src/vulkan_base/vulkan_device.cpp src/vulkan_base/vulkan_swapchain.cpp src/vulkan_base/vulkan_renderpass.cpp src/vulkan_base/vulkan_pipeline.cpp src/vulkan_base/vulkan_utils.cpp src/vulkan_base/vulkan_computePipeline.cpp src/Buildingblocks/Buildingblocks.cpp src/Buildingblocks/InterpretationsCluster.cpp src/Buildingblocks/SensorGruppe.cpp src/Buildingblocks/Spezies.cpp src/Buildingblocks/Simulation_base.cpp)

# Find SDL2
add_subdirectory(libs/SDL/SDL)

# Find Vulkan
find_package(Vulkan REQUIRED)

if (UNIX)
add_custom_target(build_shaders ALL
    COMMAND "${PROJECT_SOURCE_DIR}/shaders/compile.sh"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/shaders"
)
endif(UNIX)

if (WIN32)
add_custom_target(build_shaders ALL
    COMMAND "${PROJECT_SOURCE_DIR}/shaders/compile.bat"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/shaders"
)
endif(WIN32)

# vulkan_tutorial executable
add_executable(vulkan_tutorial ${SOURCE_FILES})
add_dependencies(vulkan_tutorial build_shaders)
target_include_directories(vulkan_tutorial PUBLIC libs/SDL/include)
target_link_libraries(vulkan_tutorial PUBLIC SDL3-static)
target_include_directories(vulkan_tutorial PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(vulkan_tutorial PUBLIC ${Vulkan_LIBRARIES})



