//
// Created by chungphb on 4/6/21.
//

#include <chirpstack_simulator/core/lora/phy_payload.h>
#include <chirpstack_simulator/core/lora/mac_payload.h>

namespace chirpstack_simulator {
namespace lora {

std::vector<byte> aes128key::marshal_binary() {
    std::vector<byte> res{_value.rbegin(), _value.rend()};
    return res;
}

bool mic::operator==(const mic& rhs) const {
    for (int i = 0; i < _value.size(); ++i) {
        if (_value[i] != rhs._value[i]) {
            return false;
        }
    }
    return true;
}

bool mic::operator!=(const mic& rhs) const {
    return !operator==(rhs);
}

void aes128key::unmarshal_binary(const std::vector<byte>& data) {
    if (data.size() != _value.size()) {
        throw std::runtime_error("lora: failed to unmarshal eui64");
    }
    for (int i = 0; i < _value.size(); ++i) {
        _value[_value.size() - (i + 1)] = data[i];
    }
}

std::vector<byte> mhdr::marshal_binary() {
    byte res = (static_cast<byte>(_m_type) << 5) | (static_cast<byte>(_major) & 0x03);
    return {res};
}

void mhdr::unmarshal_binary(const std::vector<byte>& data) {
    if (data.size() != 1) {
        throw std::runtime_error("lora: failed to unmarshal mac header");
    }
    _m_type = static_cast<m_type>(data[0] >> 5);
    _major = static_cast<major>(data[0] & 0x03);
}

void phy_payload::set_uplink_data_mic(const uplink_data_info& info) {
    _mic = calculate_uplink_data_mic(info);
}

bool phy_payload::validate_uplink_data_mic(const uplink_data_info& info) {
    auto mic = calculate_uplink_data_mic(info);
    return _mic == mic;
}

bool phy_payload::validate_uplink_data_micf(aes128key f_nwk_s_int_key) {
    // Calculate mic
    uplink_data_info info{};
    info._m_ver = mac_version::lorawan_1_1;
    info._conf_f_cnt = 0;
    info._tx_ch = 0;
    info._tx_dr = 0;
    info._f_nwk_s_int_key = f_nwk_s_int_key;
    info._s_nwk_s_int_key = f_nwk_s_int_key;
    auto mic = calculate_uplink_data_mic(info);

    // Validate micf
    for (int i = 2; i < _mic._value.size(); ++i) {
        if (_mic._value[i] != mic._value[i]) {
            return false;
        }
    }
    return true;
}

mic phy_payload::calculate_uplink_data_mic(const uplink_data_info& info) {
    return mic{};
}

void phy_payload::set_downlink_data_mic(const downlink_data_info& info) {
    _mic = calculate_downlink_data_mic(info);
}

bool phy_payload::validate_downlink_data_mic(const downlink_data_info& info) {
    auto mic = calculate_downlink_data_mic(info);
    return _mic == mic;
}

mic phy_payload::calculate_downlink_data_mic(const downlink_data_info& info) {
    return mic{};
}

void phy_payload::set_uplink_join_mic(const uplink_join_info& info) {
    _mic = calculate_uplink_join_mic(info);
}

bool phy_payload::validate_uplink_join_mic(const uplink_join_info& info) {
    auto mic = calculate_uplink_join_mic(info);
    return _mic == mic;
}

mic phy_payload::calculate_uplink_join_mic(const uplink_join_info& info) {
    return mic{};
}

void phy_payload::set_downlink_join_mic(const downlink_join_info& info) {
    _mic = calculate_downlink_join_mic(info);
}

bool phy_payload::validate_downlink_join_mic(const downlink_join_info& info) {
    auto mic = calculate_downlink_join_mic(info);
    return _mic == mic;
}

mic phy_payload::calculate_downlink_join_mic(const downlink_join_info& info) {
    return mic{};
}

void phy_payload::encrypt_join_accept_payload(aes128key key) {

}

void phy_payload::decrypt_join_accept_payload(aes128key key) {

}

void phy_payload::encrypt_f_opts(aes128key nwk_s_enc_key) {

}

void phy_payload::decrypt_f_opts(aes128key nwk_s_enc_key) {

}

void phy_payload::decode_f_opts_to_mac_commands() {

}

std::vector<byte> phy_payload::encrypt_f_opts(const f_opts_info& info) {
    return {};
}

void phy_payload::encrypt_frm_payload(aes128key key) {

}

void phy_payload::decrypt_frm_payload(aes128key key) {

}

void phy_payload::decode_frm_payload_to_mac_commands() {

}

std::vector<byte> phy_payload::encrypt_frm_payload(const frm_payload_info& info) {
    return {};
}

std::vector<byte> phy_payload::marshal_binary() {
    if (_mac_payload == nullptr) {
        throw std::runtime_error("lora: mac_payload should not be null");
    }
    std::vector<byte> res;
    std::vector<byte> bytes;

    // Marshal MAC header
    bytes = _mhdr.marshal_binary();
    res.insert(res.end(), bytes.begin(), bytes.end());

    // Marshal MAC payload
    bytes = _mac_payload->marshal_binary();
    res.insert(res.end(), bytes.begin(), bytes.end());

    // Marshal MIC
    res.insert(res.end(), _mic._value.begin(), _mic._value.end());

    // Return
    return res;
}

void phy_payload::unmarshal_binary(const std::vector<byte>& data) {
    if (data.size() < 5) {
        throw std::runtime_error("lora: failed to unmarshal physical payload");
    }
    std::vector<byte> bytes;

    // Unmarshal MAC header
    bytes = {data.begin(), data.begin() + 1};
    _mhdr.unmarshal_binary(bytes);

    // Unmarshal MAC payload
    bytes = {data.begin() + 1, data.begin() + data.size() - 4};
    switch (_mhdr._m_type) {
        case m_type::join_request: {
            _mac_payload = std::make_unique<join_request_payload>();
            break;
        }
        case m_type::join_accept: {
            _mac_payload = std::make_unique<data_payload>();
            break;
        }
        case m_type::rejoin_request: {
            switch (data[1]) {
                case 0:
                case 2: {
                    _mac_payload = std::make_unique<rejoin_request_type_02_payload>();
                    break;
                }
                case 1: {
                    _mac_payload = std::make_unique<rejoin_request_type_1_payload>();
                    break;
                }
                default: {
                    throw std::runtime_error("lora: invalid rejoin type");
                }
            }
            break;
        }
        case m_type::proprietary: {
            _mac_payload = std::make_unique<data_payload>();
            break;
        }
        default: {
            _mac_payload = std::make_unique<mac_payload>();
            break;
        }
    }
    _mac_payload->unmarshal_binary(bytes, is_uplink());

    // Unmarshal MIC
    for (int i = 0; i < 4; ++i) {
        _mic._value[i] = data[data.size() - 4 + i];
    }

}

bool phy_payload::is_uplink() {
    switch (_mhdr._m_type) {
        case m_type::join_request:
        case m_type::unconfirmed_data_up:
        case m_type::confirmed_data_up:
        case m_type::rejoin_request: {
            return true;
        }
        default: {
            return false;
        }
    }
}

}
}