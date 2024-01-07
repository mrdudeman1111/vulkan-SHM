CMAKE_MINIMUM_REQUIRED(VERSION 3.18.1)
project("External Image example")

set(CMAKE_BUILD_TYPE Debug)

file(GLOB SOURCES
    ${CMAKE_SOURCE_DIR}/src/*.cpp)

find_package(glfw3 CONFIG REQUIRED)
find_package(Sail CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/include ${glfw3_INCLUDE_DIRS} ${glm_INCLUDE_DIRS} ${Sail_INCLUDE_DIRS})

add_executable(ExtImage ${SOURCES})

target_link_libraries(ExtImage vulkan sail::sail ${glm_LIBRARIES} ${glfw3_LIBRARIES})

