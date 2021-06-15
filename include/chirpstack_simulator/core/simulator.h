//
// Created by chungphb on 25/5/21.
//

#pragma once

#include <chirpstack_simulator/core/device.h>
#include <chirpstack_simulator/core/gateway.h>
#include <chirpstack_simulator/core/config.h>
#include <chirpstack_client/chirpstack_client.h>
#include <spdlog/spdlog.h>

namespace chirpstack_simulator {

using namespace chirpstack_cpp_client;

struct simulator {
public:
    void generate_config_file(const std::string& config_file);
    void set_config_file(const std::string& config_file);
    void init();
    void run();
    void stop();
    void test_downlink();

private:
    bool is_running();
    void setup_client();
    void setup_service_profile();
    void setup_gateways();
    void setup_device_profile();
    void setup_application();
    void setup_devices();
    void setup_device_keys();
    void tear_down_client();
    void tear_down_devices();
    void tear_down_application();
    void tear_down_device_profile();
    void tear_down_gateways();

private:
    std::string _config_file;
    config _config;
    std::vector<std::shared_ptr<device>> _dev_list;
    std::vector<std::shared_ptr<gateway>> _gw_list;
    std::unique_ptr<chirpstack_client> _client;
    bool _stopped = false;
    struct client_info {
        std::string _device_profile_name;
        std::string _application_name;
        api::ServiceProfile _service_profile;
        std::string _device_profile_id;
        int64_t _application_id;
    };
    client_info _client_info;
};

}