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
    int _duration = 60;
    int _activation_time = 5;
    int _dev_count = 100;
    int _uplink_interval = 10;
    int _f_port = 10;
    std::string _uplink_payload = "uplink_packet_1234";
    int _freq = 868100000;
    int _bandwidth = 125;
    int _s_factor = 12;
    int _gw_min_count = 3;
    int _gw_max_count = 5;
    bool _enable_downlink_test = false;
    int _downlink_interval = 15;
    std::string _downlink_payload = "downlink_packet_1234";
};

}