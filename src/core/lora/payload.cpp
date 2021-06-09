//
// Created by chungphb on 4/6/21.
//

#include <chirpstack_simulator/core/lora/payload.h>
#include <chirpstack_simulator/util/helper.h>
#include <algorithm>

namespace chirpstack_simulator {
namespace lora {

std::vector<byte> eui64::marshal_binary() {
    std::vector<byte> res{_value.rbegin(), _value.rend()};
    return res;
}

void eui64::unmarshal_binary(const std::vector<byte>& data) {
    if (data.size() != _value.size()) {
        throw std::runtime_error("lora: failed to unmarshal eui64");
    }
    for (int i = 0; i < _value.size(); ++i) {
        _value[_value.size() - (i + 1)] = data[i];
    }
}

std::basic_string<byte> eui64::string() {
    return to_hex_string(_value.data(), _value.size());
}

bool eui64::operator<(const eui64& rhs) const {
    for (int i = 0; i < _value.size(); ++i) {
        if (_value[i] < rhs._value[i]) {
            return true;
        }
    }
    return false;
}

std::vector<byte> dev_nonce::marshal_binary() {
    std::vector<byte> res;
    std::array<byte, sizeof(_value)> bytes{};
    std::memcpy(bytes.data(), &_value, sizeof(_value));
    res.insert(res.end(), bytes.begin(), bytes.end());
    return res;
}

void dev_nonce::unmarshal_binary(const std::vector<byte>& data) {
    if (data.size() != sizeof(_value)) {
        throw std::runtime_error("lora: failed to unmarshal device nonce");
    }
    std::memcpy(&_value, data.data(), sizeof(_value));
}

std::vector<byte> join_nonce::marshal_binary() {
    if (_value >= (1 << 24)) {
        throw std::runtime_error("lora: max value of join nonce is 2^24 - 1");
    }
    std::vector<byte> res;
    std::array<byte, sizeof(_value)> bytes{};
    std::memcpy(bytes.data(), &_value, sizeof(_value));
    res.insert(res.end(), bytes.begin(), bytes.begin() + 3);
    return res;
}

void join_nonce::unmarshal_binary(const std::vector<byte>& data) {
    if (data.size() != 3) {
        throw std::runtime_error("lora: failed to unmarshal join nonce");
    }
    std::vector<byte> bytes;
    bytes = data;
    bytes.push_back(0x00);
    std::memcpy(&_value, bytes.data(), sizeof(_value));
}

std::vector<byte> dl_settings::marshal_binary() {
    if (_rx2_data_rate > 15) {
        throw std::runtime_error("lora: max value of rx2_data_rate is 15");
    }
    if (_rx1_dr_offset > 7) {
        throw std::runtime_error("lora: max value of rx1_dr_offset is 7");
    }
    byte res = static_cast<byte>(_rx2_data_rate);
    res |= static_cast<byte>(_rx1_dr_offset << 4);
    if (_opt_neg) {
        res |= static_cast<byte>(0x80);
    }
    return {res};
}

void dl_settings::unmarshal_binary(const std::vector<byte>& data) {
    if (data.size() != 1) {
        throw std::runtime_error("lora: failed to unmarshal downlink settings");
    }
    _opt_neg = (data[0] & 0x80) != 0;
    _rx2_data_rate = data[0] & 0x0f;
    _rx1_dr_offset = (data[0] & 0x70) >> 4;
}

std::vector<byte> data_payload::marshal_binary() {
    return _data;
}

void data_payload::unmarshal_binary(const std::vector<byte>& data, bool uplink) {
    _data = data;
}

std::vector<byte> join_request_payload::marshal_binary() {
    std::vector<byte> res;
    std::vector<byte> bytes;
    res.reserve(18);

    // Marshal join EUI
    bytes = _join_eui.marshal_binary();
    res.insert(res.end(), bytes.begin(), bytes.end());

    // Marshal device EUI
    bytes = _dev_eui.marshal_binary();
    res.insert(res.end(), bytes.begin(), bytes.end());

    // Marshal device nonce
    bytes = _dev_nonce.marshal_binary();
    res.insert(res.end(), bytes.begin(), bytes.end());

    // Return
    return res;
}

void join_request_payload::unmarshal_binary(const std::vector<byte>& data, bool uplink) {
    if (data.size() != 18) {
        throw std::runtime_error("lora: failed to unmarshal join request payload");
    }
    std::vector<byte> bytes;

    // Unmarshal join EUI
    bytes = {data.begin(), data.begin() + 8};
    _join_eui.unmarshal_binary(bytes);

    // Unmarshal device EUI
    bytes = {data.begin() + 8, data.begin() + 16};
    _dev_eui.unmarshal_binary(bytes);

    // Unmarshal device nonce
    bytes = {data.begin() + 16, data.end()};
    _dev_nonce.unmarshal_binary(bytes);
}

std::vector<byte> cf_list::marshal_binary() {
    std::vector<byte> res;
    std::vector<byte> bytes;
    res.reserve(16);

    // Marshal payload
    bytes = _payload->marshal_binary();
    res.insert(res.end(), bytes.begin(), bytes.end());

    // Marshal type
    res.push_back(static_cast<byte>(_type));

    // Return
    return res;
}

void cf_list::unmarshal_binary(const std::vector<byte>& data) {
    if (data.size() != 16) {
        throw std::runtime_error("lora: failed to unmarshal channel list");
    }
    std::vector<byte> bytes;

    // Unmarshal type
    _type = static_cast<cf_list_type>(data[15]);

    // Unmarshal payload
    bytes = {data.begin(), data.begin() + 15};
    switch (_type) {
        case cf_list_type::cf_list_channel: {
            _payload = std::make_unique<cf_list_channel_payload>();
            break;
        }
        case cf_list_type::cf_list_channel_mask: {
            _payload = std::make_unique<cf_list_channel_mask_payload>();
            break;
        }
    }
    _payload->unmarshal_binary(bytes, false);
}

std::vector<byte> cf_list_channel_payload::marshal_binary() {
    std::vector<byte> res;
    uint32_t freq;
    std::array<byte, sizeof(freq)> bytes{};
    for (const auto& channel : _channels) {
        if (channel % 100) {
            throw std::runtime_error("lora: invalid frequency");
        }
        freq = channel / 100;
        if (freq >= (1 << 24)) {
            throw std::runtime_error("lora: invalid frequency");
        }
        std::memcpy(bytes.data(), &freq, sizeof(freq));
        res.insert(res.end(), bytes.begin(), bytes.begin() + 3);
    }
    return res;
}

void cf_list_channel_payload::unmarshal_binary(const std::vector<byte>& data, bool uplink) {
    if (data.size() > 15 || data.size() % 3) {
        throw std::runtime_error("lora: failed to unmarshal cf list channel payload");
    }
    std::vector<byte> bytes;
    uint32_t freq;
    for (int i = 0; i < data.size() / 3; ++i) {
        bytes = {data.begin() + i * 3, data.begin() + i * 3 + 3};
        bytes.push_back(0x00);
        std::memcpy(&freq, bytes.data(), sizeof(freq));
        _channels[i] = freq * 100;
    }
}

std::vector<byte> ch_mask::marshal_binary() {
    std::vector<byte> res;
    uint16_t mask = 0;
    std::array<byte, sizeof(mask)> bytes{};
    for (int i = 0; i < _value.size(); ++i) {
        if (_value[i]) {
            mask |= static_cast<uint16_t>(1 << i);
        }
    }
    std::memcpy(bytes.data(), &mask, sizeof(mask));
    res.insert(res.end(), bytes.begin(), bytes.end());
    return res;
}

void ch_mask::unmarshal_binary(const std::vector<byte>& data) {
    if (data.size() != 2) {
        throw std::runtime_error("lora: failed to unmarshal channel mask");
    }
    uint16_t mask = 0;
    std::memcpy(&mask, data.data(), sizeof(mask));
    for (int i = 0; i < _value.size(); ++i) {
        _value[i] = (mask & static_cast<uint16_t>(1 << i)) != 0;
    }
}

ch_mask::operator bool() const {
    return std::any_of(_value.begin(), _value.end(), [](const auto& bit) {
        return bit;
    });
}

std::vector<byte> cf_list_channel_mask_payload::marshal_binary() {
    if (_channel_masks.size() > 6) {
        throw std::runtime_error("lora: max number of channel mask is 6");
    }
    std::vector<byte> res;
    std::vector<byte> bytes;
    for (auto& channel_mask : _channel_masks) {
        bytes = channel_mask.marshal_binary();
        res.insert(res.end(), bytes.begin(), bytes.end());
    }
    return res;
}

void cf_list_channel_mask_payload::unmarshal_binary(const std::vector<byte>& data, bool uplink) {
    if (data.size() > 15) {
        throw std::runtime_error("lora: failed to unmarshal channel mask list");
    }
    std::vector<ch_mask> pending;
    std::vector<byte> bytes;
    for (int i = 0; i < (data.size() - 1) / 2; ++i) {
        bytes = {data.begin() + i * 2, data.begin() + i * 2 + 2};
        ch_mask channel_mask{};
        channel_mask.unmarshal_binary(bytes);
        pending.push_back(channel_mask);
        if (channel_mask) {
            _channel_masks.insert(_channel_masks.end(), pending.begin(), pending.end());
            pending.clear();
        }
    }
}

std::vector<byte> join_accept_payload::marshal_binary() {
    if (_rx_delay > 15) {
        throw std::runtime_error("lora: max value of rx_delay is 15");
    }
    std::vector<byte> res;
    std::vector<byte> bytes;
    res.reserve(12);

    // Marshal join nonce
    bytes = _join_nonce.marshal_binary();
    res.insert(res.end(), bytes.begin(), bytes.end());

    // Marshal home net ID
    bytes = _home_net_id.marshal_binary();
    res.insert(res.end(), bytes.begin(), bytes.end());

    // Marshal device address
    bytes = _dev_addr.marshal_binary();
    res.insert(res.end(), bytes.begin(), bytes.end());

    // Marshal downlink settings
    bytes = _dl_settings.marshal_binary();
    res.insert(res.end(), bytes.begin(), bytes.end());

    // Marshal RX delay
    res.push_back(static_cast<byte>(_rx_delay));

    // Marshal channel list
    if (_cf_list) {
        bytes = _cf_list->marshal_binary();
        res.insert(res.end(), bytes.begin(), bytes.end());
    }

    // Return
    return res;
}

void join_accept_payload::unmarshal_binary(const std::vector<byte>& data, bool uplink) {
    if (data.size() != 12 && data.size() != 28) {
        throw std::runtime_error("lora: failed to unmarshal join accept payload");
    }
    std::vector<byte> bytes;

    // Unmarshal join nonce
    bytes = {data.begin(), data.begin() + 3};
    _join_nonce.unmarshal_binary(bytes);

    // Unmarshal home net ID
    bytes = {data.begin() + 3, data.begin() + 6};
    _home_net_id.unmarshal_binary(bytes);

    // Unmarshal device address
    bytes = {data.begin() + 6, data.begin() + 10};
    _dev_addr.unmarshal_binary(bytes);

    // Unmarshal downlink settings
    bytes = {data.begin() + 10, data.begin() + 11};
    _dl_settings.unmarshal_binary(bytes);

    // Unmarshal RX delay
    _rx_delay = static_cast<uint8_t>(data[11]);

    // Unmarshal channel list
    if (data.size() == 28) {
        bytes = {data.begin() + 12, data.end()};
        _cf_list = std::make_unique<cf_list>();
        _cf_list->unmarshal_binary(bytes);
    }
}

std::vector<byte> rejoin_request_type_02_payload::marshal_binary() {
    if (_rejoin_type != join_type::rejoin_request_type_0 && _rejoin_type != join_type::rejoin_request_type_2) {
        throw std::runtime_error("lora: rejoin_type must be 0 or 2");
    }
    std::vector<byte> res;
    std::vector<byte> bytes;
    res.reserve(14);

    // Marshal join type
    res.push_back(static_cast<byte>(_rejoin_type));

    // Marshal net ID
    bytes = _net_id.marshal_binary();
    res.insert(res.end(), bytes.begin(), bytes.end());

    // Marshal device EUI
    bytes = _dev_eui.marshal_binary();
    res.insert(res.end(), bytes.begin(), bytes.end());

    // Marshal rejoin count
    std::array<byte, sizeof(_rj_count_0)> rj_count_bytes{};
    std::memcpy(rj_count_bytes.data(), &_rj_count_0, sizeof(_rj_count_0));
    res.insert(res.end(), rj_count_bytes.begin(), rj_count_bytes.end());

    // Return
    return res;
}

void rejoin_request_type_02_payload::unmarshal_binary(const std::vector<byte>& data, bool uplink) {
    if (data.size() != 14) {
        throw std::runtime_error("lora: failed to unmarshal rejoin request payload");
    }
    std::vector<byte> bytes;

    // Unmarshal join type
    _rejoin_type = static_cast<join_type>(data[0]);

    // Unmarshal net ID
    bytes = {data.begin() + 1, data.begin() + 4};
    _net_id.unmarshal_binary(bytes);

    // Unmarshal device EUI
    bytes = {data.begin() + 4, data.begin() + 12};
    _dev_eui.unmarshal_binary(bytes);

    // Unmarshal rejoin count
    bytes = {data.begin() + 12, data.end()};
    std::memcpy(&_rj_count_0, bytes.data(), sizeof(_rj_count_0));
}

std::vector<byte> rejoin_request_type_1_payload::marshal_binary() {
    if (_rejoin_type != join_type::rejoin_request_type_1) {
        throw std::runtime_error("lora: rejoin_type must be 1");
    }
    std::vector<byte> res;
    std::vector<byte> bytes;
    res.reserve(19);

    // Marshal join type
    res.push_back(static_cast<byte>(_rejoin_type));

    // Marshal join EUI
    bytes = _join_eui.marshal_binary();
    res.insert(res.end(), bytes.begin(), bytes.end());

    // Marshal device EUI
    bytes = _dev_eui.marshal_binary();
    res.insert(res.end(), bytes.begin(), bytes.end());

    // Marshal rejoin count
    std::array<byte, sizeof(_rj_count_1)> rj_count_bytes{};
    std::memcpy(rj_count_bytes.data(), &_rj_count_1, sizeof(_rj_count_1));
    res.insert(res.end(), rj_count_bytes.begin(), rj_count_bytes.end());

    // Return
    return res;
}

void rejoin_request_type_1_payload::unmarshal_binary(const std::vector<byte>& data, bool uplink) {
    if (data.size() != 19) {
        throw std::runtime_error("lora: failed to unmarshal rejoin request payload");
    }
    std::vector<byte> bytes;

    // Unmarshal join type
    _rejoin_type = static_cast<join_type>(data[0]);

    // Unmarshal join EUI
    bytes = {data.begin() + 1, data.begin() + 9};
    _join_eui.unmarshal_binary(bytes);

    // Unmarshal device EUI
    bytes = {data.begin() + 9, data.begin() + 17};
    _dev_eui.unmarshal_binary(bytes);

    // Unmarshal rejoin count
    bytes = {data.begin() + 17, data.end()};
    std::memcpy(&_rj_count_1, bytes.data(), sizeof(_rj_count_1));
}

}
}