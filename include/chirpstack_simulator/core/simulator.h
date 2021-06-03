//
// Created by chungphb on 25/5/21.
//

#pragma once

#include <chirpstack_simulator/core/device.h>
#include <chirpstack_simulator/core/gateway.h>
#include <chirpstack_simulator/util/config.h>
#include <chirpstack_client/chirpstack_client.h>

namespace chirpstack_simulator {

using namespace chirpstack_cpp_client;

struct simulator {
public:
    void init();
    void start();
    void stop();

private:
    struct client_info {
        std::string _device_profile_name;
        std::string _application_name;
        api::ServiceProfile _service_profile;
        std::string _device_profile_id;
        int64_t _application_id;
    };

    void setup_client();
    void setup_service_profile(client_info& info);
    void setup_gateway(client_info& info);
    void setup_device_profile(client_info& info);
    void setup_application(client_info& info);
    void setup_device(client_info& info);
    void setup_device_keys(client_info& info);

private:
    int _socket_fd;
    sockaddr_in _server_addr;
    config _config;
    std::vector<device> _dev_list;
    std::vector<gateway> _gw_list;
    std::unique_ptr<chirpstack_client> _client;
};

}