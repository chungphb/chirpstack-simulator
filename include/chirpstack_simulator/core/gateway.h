//
// Created by chungphb on 24/5/21.
//

#pragma once

#include <chirpstack_simulator/core/lora/phy_payload.h>
#include <chirpstack_simulator/util/channel.h>
#include <gw/gw.grpc.pb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <future>
#include <map>

namespace chirpstack_simulator {

struct simulator;

struct gateway {
public:
    void run();
    void stop();
    void add_device(lora::eui64 dev_eui, std::shared_ptr<channel<gw::DownlinkFrame>> channel);
    void send_uplink_frame(gw::UplinkFrame frame);
    void keep_alive();
    void handle_downlink_frame();
    void send_downlink_tx_ack(gw::DownlinkTXAck ack);
    friend struct simulator;

private:
    std::vector<byte> generate_push_data_packet(const gw::UplinkFrame& frame);
    std::vector<byte> generate_pull_data_packet();
    std::vector<byte> generate_tx_ack_packet(const gw::DownlinkTXAck& ack);
    bool is_push_ack(const byte* resp, size_t resp_len, const std::vector<byte>& packet);
    bool is_pull_ack(const byte* resp, size_t resp_len);
    bool is_pull_resp(const byte* resp, size_t resp_len);

private:
    int _push_socket_fd = -1;
    int _pull_socket_fd = -1;
    sockaddr_in _server;
    lora::eui64 _gateway_id;
    std::map<lora::eui64, std::shared_ptr<channel<gw::DownlinkFrame>>> _devices;
    gw::UplinkRXInfo _uplink_rx_info;
    std::future<void> _keep_alive;
    std::future<void> _handle_downlink_frame;
    bool _connected = false;
    bool _stopped = false;
    std::mutex _push_mutex;
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

