//
// Created by chungphb on 26/5/21.
//

#pragma once

#include <chirpstack_simulator/util/data_types.h>

namespace chirpstack_simulator {

constexpr size_t DEV_ADDR_LEN = 4;
constexpr size_t GW_MAC_LEN = 8;
constexpr size_t ACK_MAX_LEN = 1024;

constexpr char DEFAULT_NS_HOST[] = "localhost";
constexpr size_t DEFAULT_NS_PORT = 1700;
constexpr char DEFAULT_AS_HOST[] = "localhost";
constexpr size_t DEFAULT_AS_PORT = 8080;

struct config {
    server_address _network_server{DEFAULT_NS_HOST, DEFAULT_NS_PORT};
    server_address _application_server{DEFAULT_AS_HOST, DEFAULT_AS_PORT};
    std::string _jwt_token;
    std::string _service_profile_id;
    int _dev_count = 1000;
    int _f_port = 10;
    std::string _payload = "TEST_PACKET_1234";
    int _freq = 868100000;
    int _s_factor = 12;
    int _gw_min_count = 3;
    int _gw_max_count = 5;
};

}