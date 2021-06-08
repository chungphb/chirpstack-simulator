//
// Created by chungphb on 25/5/21.
//

#include <chirpstack_simulator/core/device.h>
#include <chirpstack_simulator/core/lora/phy_payload.h>
#include <chirpstack_simulator/core/lora/mac_payload.h>
#include <chirpstack_simulator/util/helper.h>
#include <spdlog/spdlog.h>
#include <thread>
#include <chrono>

namespace chirpstack_simulator {

using namespace std::chrono_literals;

void device::run() {
    _uplink_loop = std::async(std::launch::async, &device::uplink_loop, this);
    _downlink_loop = std::async(std::launch::async, &device::downlink_loop, this);
}

void device::stop() {
    _stopped = true;
    _uplink_loop.get();
    _downlink_loop.get();
}

void device::uplink_loop() {
    std::this_thread::sleep_for(std::chrono::seconds(_otaa_delay));
    while (!_stopped) {
        switch (_dev_state) {
            case device_state::otaa: {
                send_join_request();
                std::this_thread::sleep_for(6s);
                break;
            }
            case device_state::activated: {
                send_data();
                if (_uplink_cnt > 0 && _uplink_cnt <= _f_cnt_up) {
                    std::this_thread::sleep_for(1s);
                    stop();
                    return;
                }
                std::this_thread::sleep_for(std::chrono::seconds(_uplink_interval));
            }
        }
    }
}

void device::send_join_request() {
    lora::phy_payload phy_payload{};

    // Set MHDR
    phy_payload._mhdr._m_type = lora::m_type::join_request;
    phy_payload._mhdr._major = lora::major::lorawan_r1;

    // Set MAC payload
    lora::join_request_payload payload;
    payload._dev_eui = _dev_eui;
    payload._join_eui = _join_eui;
    payload._dev_nonce = get_dev_nonce();
    phy_payload._mac_payload = std::make_shared<lora::join_request_payload>(std::move(payload));

    // Set MIC
    lora::phy_payload::uplink_join_info info{};
    info._key = _app_key;
    phy_payload.set_uplink_join_mic(info);

    // Send payload
    send_uplink(std::move(phy_payload));
}

void device::send_data() {
    lora::phy_payload phy_payload{};

    // Set MHDR
    phy_payload._mhdr._m_type = _confirmed ? lora::m_type::confirmed_data_up : lora::m_type::unconfirmed_data_up;
    phy_payload._mhdr._major = lora::major::lorawan_r1;

    // Set MAC payload
    lora::mac_payload payload;
    payload._fhdr._dev_addr = _dev_addr;
    payload._fhdr._f_cnt = _f_cnt_up;
    payload._fhdr._f_ctrl._adr = false;
    payload._f_port = std::make_unique<uint8_t>(_f_port);
    lora::data_payload data;
    data._data = _payload;
    payload._frm_payload.push_back(std::make_unique<lora::data_payload>(std::move(data)));
    phy_payload._mac_payload = std::make_shared<lora::mac_payload>(std::move(payload));

    // Encrypt
    phy_payload.encrypt_frm_payload(_app_s_key);

    // Set MIC
    lora::phy_payload::uplink_data_info info{};
    info._m_ver = lora::mac_version::lorawan_1_0;
    info._conf_f_cnt = 0;
    info._tx_dr = 0;
    info._tx_ch = 0;
    info._f_nwk_s_int_key = _nwk_s_key;
    info._s_nwk_s_int_key = _nwk_s_key;
    phy_payload.set_uplink_data_mic(info);

    ++_f_cnt_up;

    // Send payload
    send_uplink(std::move(phy_payload));
}

void device::send_uplink(lora::phy_payload phy_payload) {
    // Prepare uplink frame
    auto payload = phy_payload.marshal_binary();
    gw::UplinkFrame frame;
    frame.set_phy_payload(payload.data(), payload.size());
    gw::UplinkTXInfo* tx_info = frame.mutable_tx_info();
    *tx_info = _uplink_tx_info;

    // Send uplink frame
    for (const auto& gateway : _gateways) {
        gateway->send_uplink_frame(frame);
    }
}

void device::downlink_loop() {
    while (!_stopped) {
        spdlog::debug("Receive downlink");
        std::this_thread::sleep_for(6s);
    }
}

lora::dev_nonce device::get_dev_nonce() {
    if (_rand_dev_nonce) {
        constexpr size_t N = sizeof(lora::dev_nonce::_value) / sizeof(byte);
        std::array<byte, N> buf{};
        for (int i = 0; i < N; ++i) {
            buf[i] = get_random_byte();
        }
        memcpy(&_dev_nonce._value, buf.data(), buf.size());
    } else {
        ++_dev_nonce._value;
    }
    return _dev_nonce;
}

}