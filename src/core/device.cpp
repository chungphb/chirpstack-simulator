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
#include <aes.h>
#include <modes.h>
#include <filters.h>

namespace chirpstack_simulator {

using namespace std::chrono_literals;

void device::run() {
    _uplink_loop = std::async(std::launch::async, &device::uplink_loop, this);
    _downlink_loop = std::async(std::launch::async, &device::downlink_loop, this);
}

void device::stop() {
    _stopped = true;
    _downlink_frames->close();
    _uplink_loop.get();
    _downlink_loop.get();
}

void device::add_gateway(std::shared_ptr<gateway> gateway) {
    gateway->add_device(_dev_eui, _downlink_frames);
    _gateways.push_back(std::move(gateway));
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
    spdlog::debug("DEV {}: Send Join request", _dev_eui.string());
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

    // Send payload
    spdlog::debug("DEV {}: Send packet #{}", _dev_eui.string(), _f_cnt_up);
    ++_f_cnt_up;
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
    gw::DownlinkFrame frame;
    while (!_stopped) {
        if (_downlink_frames->is_closed()) {
            break;
        }
        if (_downlink_frames->get(frame)) {
            auto payload = base64_decode(frame.phy_payload());
            lora::phy_payload phy_payload{};
            phy_payload.unmarshal_binary({payload.begin(), payload.end()});
            switch (phy_payload._mhdr._m_type) {
                case lora::m_type::join_accept: {
                    handle_join_accept(std::move(phy_payload));
                    break;
                }
                case lora::m_type::unconfirmed_data_down:
                case lora::m_type::confirmed_data_down: {
                    handle_data(std::move(phy_payload));
                    break;
                }
                default: {
                    throw std::runtime_error("Something went wrong");
                }
            }
        }
        std::this_thread::sleep_for(50ms);
    }
}

void device::handle_join_accept(lora::phy_payload phy_payload) {
    spdlog::debug("DEV {}: Handle Join accept", _dev_eui.string());

    // Decrypt Join Accept payload
    phy_payload.decrypt_join_accept_payload(_app_key);

    // Validate MIC
    lora::phy_payload::downlink_join_info join_info{};
    join_info._join_req_type = lora::join_type::join_request_type;
    join_info._join_eui = _join_eui;
    join_info._dev_nonce = _dev_nonce;
    join_info._key = _app_key;
    auto res = phy_payload.validate_downlink_join_mic(join_info);
    if (!res) {
        throw std::runtime_error("Invalid MIC");
    }

    // Validate payload type
    auto payload = std::dynamic_pointer_cast<lora::join_accept_payload>(phy_payload._mac_payload);
    if (!payload) {
        throw std::runtime_error("Wrong type of payload");
    }

    // Update device settings
    key_info key_info{};
    key_info._opt_neg = payload->_dl_settings._opt_neg;
    key_info._nwk_key = _app_key;
    key_info._net_id = payload->_home_net_id;
    key_info._join_eui = _join_eui;
    key_info._join_nonce = payload->_join_nonce;
    key_info._dev_nonce = _dev_nonce;
    _app_s_key = get_app_s_key(key_info);
    _nwk_s_key = get_nwk_s_key(key_info);
    _dev_addr = payload->_dev_addr;

    // Update device state
    _dev_state = device_state::activated;
}

void device::handle_data(lora::phy_payload phy_payload) {
    spdlog::debug("DEV {}: Handle packet #{}", _dev_eui.string(), _f_cnt_down);
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

lora::aes128key get_app_s_key(key_info& info) {
    return get_s_key(info, 0x02);
}

lora::aes128key get_nwk_s_key(key_info& info) {
    return get_s_key(info, 0x01);
}

lora::aes128key get_s_key(key_info& info, byte type) {
    lora::aes128key key{};
    std::vector<byte> res(16, 0x00);
    std::vector<byte> bytes;

    // Marshal type
    res.push_back(type);

    // Marshal join nonce
    bytes = info._join_nonce.marshal_binary();
    res.insert(res.end(), bytes.begin(), bytes.end());

    if (info._opt_neg) {
        // Marshal join EUI
        bytes = info._join_eui.marshal_binary();
    } else {
        // Marshal net ID
        bytes = info._net_id.marshal_binary();
    }
    res.insert(res.end(), bytes.begin(), bytes.end());

    // Marshal device nonce
    bytes = info._dev_nonce.marshal_binary();
    res.insert(res.end(), bytes.begin(), bytes.end());

    // Encrypt
    std::string plain(res.data(), res.size());
    std::string cipher;
    CryptoPP::ECB_Mode<CryptoPP::AES>::Encryption encryption;
    encryption.SetKey((const CryptoPP::byte*)info._nwk_key._value.data(), CryptoPP::AES::DEFAULT_KEYLENGTH);
    CryptoPP::StreamTransformationFilter encryptor{encryption, new CryptoPP::StringSink(cipher)};
    encryptor.Put((const CryptoPP::byte*)res.data(), res.size());

    // Return
    std::copy_n(cipher.begin(), 16, key._value.begin());
    return key;
}

}