//
// Created by chungphb on 6/6/21.
//

#pragma once

#include <chirpstack_simulator/util/data_types.h>
#include <spdlog/spdlog.h>

namespace chirpstack_simulator {

constexpr char default_ns_host[] = "localhost";
constexpr size_t default_ns_port = 1700;
constexpr char default_as_host[] = "localhost";
constexpr size_t default_as_port = 8080;

struct simulator;

struct config {
public:
    void init(const std::string& config_file);
    friend struct simulator;

private:
    server_address _network_server{default_ns_host, default_ns_port};
    server_address _application_server{default_as_host, default_as_port};
    std::string _jwt_token;
    std::string _service_profile_id;
    int _activation_time = 60;
    int _dev_count = 1000;
    int _uplink_interval = 60;
    int _f_port = 10;
    std::string _payload = "TEST_PACKET_1234";
    int _freq = 868100000;
    int _bandwidth = 500;
    int _s_factor = 12;
    int _gw_min_count = 3;
    int _gw_max_count = 5;
};

}