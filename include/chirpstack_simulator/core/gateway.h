//
// Created by chungphb on 24/5/21.
//

#pragma once

#include <chirpstack_simulator/core/lora/phy_payload.h>
#include <gw/gw.grpc.pb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <map>

namespace chirpstack_simulator {

struct simulator;

struct gateway {
public:
    ~gateway();
    void run();
    void stop() const;
    void send_uplink_frame(gw::UplinkFrame payload);
    friend struct simulator;

private:
    std::vector<byte> generate_push_data_packet(const gw::UplinkFrame& payload);

private:
    int _socket_fd;
    sockaddr_in _server;
    lora::eui64 _gateway_id;
    gw::UplinkRXInfo _uplink_rx_info;
};

struct rxpk {
public:
    template <typename field_value_t>
    void add(std::string field_name, field_value_t field_value) {
        _fields.emplace_back(std::move(field_name), std::to_string(field_value));
    }
    void add(std::string field_name, const std::string& field_value);
    void add(std::string field_name, const char* field_value);
    std::string string() const;

private:
    std::vector<std::pair<std::string, std::string>> _fields;
};

}

