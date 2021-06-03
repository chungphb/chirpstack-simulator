//
// Created by chungphb on 25/5/21.
//

#pragma once

#include <chirpstack_simulator/core/device.h>
#include <chirpstack_simulator/core/gateway.h>
#include <chirpstack_simulator/util/config.h>
#include <chirpstack_client/chirpstack_client.h>

using namespace chirpstack_cpp_client;

namespace chirpstack_simulator {

struct simulator {
public:
    void init();
    void start();
    void stop();

private:
    int _socket_fd;
    sockaddr_in _server_addr;
    config _config;
    std::vector<device> _dev_list;
    std::vector<gateway> _gw_list;
};

}