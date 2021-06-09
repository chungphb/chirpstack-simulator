cmake_minimum_required(VERSION 3.12)
project(chirpstack_simulator)

set(CMAKE_CXX_STANDARD 14)

include_directories(modules/tinytoml/include)
include_directories(modules/date/include)
include_directories(modules/spdlog/include)
include_directories(modules/chirpstack-client/include)
include_directories(modules/cryptopp)
include_directories(modules/json11)
include_directories(include)

set(chirpstack_simulator_hdrs
        include/chirpstack_simulator/core/simulator.h
        include/chirpstack_simulator/core/device.h
        include/chirpstack_simulator/core/gateway.h
        include/chirpstack_simulator/core/config.h
        include/chirpstack_simulator/core/lora/phy_payload.h
        include/chirpstack_simulator/core/lora/mac_payload.h
        include/chirpstack_simulator/core/lora/payload.h
        include/chirpstack_simulator/core/lora/fhdr.h
        include/chirpstack_simulator/core/lora/net_id.h
        include/chirpstack_simulator/util/helper.h
        include/chirpstack_simulator/util/channel.h
        include/chirpstack_simulator/util/data_types.h)
set(chirpstack_simulator_srcs
        src/core/simulator.cpp
        src/core/device.cpp
        src/core/gateway.cpp
        src/core/config.cpp
        src/core/lora/phy_payload.cpp
        src/core/lora/mac_payload.cpp
        src/core/lora/payload.cpp
        src/core/lora/fhdr.cpp
        src/core/lora/net_id.cpp
        src/util/helper.cpp)
add_library(chirpstack_simulator
        ${chirpstack_simulator_hdrs}
        ${chirpstack_simulator_srcs})
target_link_libraries(chirpstack_simulator
        PUBLIC date::date
        PUBLIC chirpstack_client
        PUBLIC cryptopp-shared
        PUBLIC json11)

add_executable(main
        apps/main.cpp)
target_link_libraries(main
        PRIVATE chirpstack_simulator)

add_subdirectory(modules)