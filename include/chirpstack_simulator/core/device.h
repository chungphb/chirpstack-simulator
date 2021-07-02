//
// Created by chungphb on 25/5/21.
//

#pragma once

#include <chirpstack_simulator/core/gateway.h>
#include <chirpstack_simulator/core/lora/phy_payload.h>
#include <gw/gw.grpc.pb.h>

namespace chirpstack_simulator {

struct simulator;

enum struct device_state : int {
    otaa = 0,
    activated = 1
};

struct device {
public:
    void run();
    void stop();
    void add_gateway(std::shared_ptr<gateway> gateway);
    friend struct simulator;

private:
    void uplink_loop();
    void send_join_request();
    void send_data();
    void send_uplink(lora::phy_payload phy_payload);
    void downlink_loop();
    void handle_join_accept(lora::phy_payload phy_payload);
    void handle_data(lora::phy_payload phy_payload);
    lora::dev_nonce get_dev_nonce();

private:
    lora::eui64 _dev_eui;
    lora::eui64 _join_eui;
    lora::aes128key _app_key;
    size_t _uplink_interval = 10;
    uint32_t _uplink_cnt = 0;
    bool _confirmed = false;
    std::vector<byte> _payload;
    uint8_t _f_port = 10;
    lora::dev_addr _dev_addr;
    lora::dev_nonce _dev_nonce;
    uint32_t _f_cnt_up = 0;
    uint32_t _f_cnt_down = 0;
    lora::aes128key _app_s_key;
    lora::aes128key _nwk_s_key;
    device_state _dev_state = device_state::otaa;
    std::shared_ptr<channel<gw::DownlinkFrame>> _downlink_frames;
    std::vector<std::shared_ptr<gateway>> _gateways;
    bool _rand_dev_nonce = false;
    gw::UplinkTXInfo _uplink_tx_info;
    size_t _otaa_delay = 5;
    std::future<void> _uplink_loop;
    std::future<void> _downlink_loop;
    bool _stopped = false;
    struct downlink_data_info {
        bool _confirmed;
        bool _ack;
        uint32_t _f_cnt_down;
        uint8_t _f_port;
        std::vector<byte> _data;
    };
    std::function<void(downlink_data_info)> _downlink_data_handler;
};

struct key_info {
    bool _opt_neg;
    lora::aes128key _nwk_key;
    lora::net_id _net_id;
    lora::eui64 _join_eui;
    lora::join_nonce _join_nonce;
    lora::dev_nonce _dev_nonce;
};

lora::aes128key get_app_s_key(key_info& info);
lora::aes128key get_nwk_s_key(key_info& info);
lora::aes128key get_s_key(key_info& info, byte type);

}
