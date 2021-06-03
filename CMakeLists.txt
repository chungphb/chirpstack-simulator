cmake_minimum_required(VERSION 3.12)
project(chirpstack_simulator)

set(CMAKE_CXX_STANDARD 14)

include_directories(modules/tinytoml/include)
include_directories(modules/date/include)
include_directories(modules/spdlog/include)
include_directories(modules/chirpstack-cpp-client/include)
include_directories(include)

set(CHIRPSTACK_SIMULATOR_HEADERS
    include/chirpstack_simulator/core/simulator.h
    include/chirpstack_simulator/core/device.h
    include/chirpstack_simulator/core/gateway.h
    include/chirpstack_simulator/core/rxpk.h
    include/chirpstack_simulator/core/payload.h
    include/chirpstack_simulator/util/helper.h
    include/chirpstack_simulator/util/config.h
    include/chirpstack_simulator/util/helper.h
)
set(CHIRPSTACK_SIMULATOR_SOURCES
    src/core/simulator.cpp
    src/core/device.cpp
    src/core/gateway.cpp
    src/core/rxpk.cpp
    src/core/payload.cpp
    src/util/helper.cpp
)
add_library(chirpstack_simulator ${CHIRPSTACK_SIMULATOR_HEADERS} ${CHIRPSTACK_SIMULATOR_SOURCES})
target_link_libraries(chirpstack_simulator PUBLIC date::date chirpstack_client)

add_executable(main apps/main.cpp)
target_link_libraries(main PRIVATE chirpstack_simulator)

add_subdirectory(modules)