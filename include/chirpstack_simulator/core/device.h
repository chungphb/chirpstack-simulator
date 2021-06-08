//
// Created by chungphb on 25/5/21.
//

#pragma once

#include <chirpstack_simulator/core/gateway.h>
#include <chirpstack_simulator/core/lora/phy_payload.h>
#include <gw/gw.grpc.pb.h>
#include <future>

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
    friend struct simulator;

private:
    void uplink_loop();
    void send_join_request();
    void send_data();
    void send_uplink(lora::phy_payload phy_payload);
    void downlink_loop();
    lora::dev_nonce get_dev_nonce();

private:
    lora::eui64 _dev_eui;
    lora::eui64 _join_eui;
    lora::aes128key _app_key;
    size_t _uplink_interval = 60;
    uint32_t _uplink_cnt = 0;
    bool _confirmed = false;
    std::vector<byte> _payload;
    uint8_t _f_port;
    lora::dev_addr _dev_addr;
    lora::dev_nonce _dev_nonce;
    uint32_t _f_cnt_up = 0;
    lora::aes128key _app_s_key;
    lora::aes128key _nwk_s_key;
    device_state _dev_state = device_state::otaa;
    std::vector<gw::DownlinkFrame> _downlink_frames;
    std::vector<std::shared_ptr<gateway>> _gateways;
    bool _rand_dev_nonce = false;
    gw::UplinkTXInfo _uplink_tx_info;
    size_t _otaa_delay = 60;
    std::future<void> _uplink_loop;
    std::future<void> _downlink_loop;
    bool _stopped = false;
};

}
