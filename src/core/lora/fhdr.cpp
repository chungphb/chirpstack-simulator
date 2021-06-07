//
// Created by chungphb on 4/6/21.
//

#include <chirpstack_simulator/core/lora/fhdr.h>
#include <chirpstack_simulator/core/lora/payload.h>

namespace chirpstack_simulator {
namespace lora {

std::vector<byte> dev_addr::marshal_binary() {
    std::vector<byte> res{_value.rbegin(), _value.rend()};
    return res;
}

void dev_addr::unmarshal_binary(const std::vector<byte>& data) {
    if (data.size() != _value.size()) {
        throw std::runtime_error("lora: failed to unmarshal device address");
    }
    for (int i = 0; i < _value.size(); ++i) {
        _value[_value.size() - (i + 1)] = data[i];
    }
}

std::vector<byte> f_ctrl::marshal_binary() {
    if (_f_opts_len > 15) {
        throw std::runtime_error("lora: max value of f_opts_len is 15");
    }
    byte res = 0;
    if (_adr) {
        res |= static_cast<byte>(0x80);
    }
    if (_adr_ack_req) {
        res |= static_cast<byte>(0x40);
    }
    if (_ack) {
        res |= static_cast<byte>(0x20);
    }
    if (_class_b || _f_pending) {
        res |= static_cast<byte>(0x10);
    }
    res |= static_cast<byte>(_f_opts_len & 0x0f);
    return {res};
}

void f_ctrl::unmarshal_binary(const std::vector<byte>& data) {
    if (data.size() != 1) {
        throw std::runtime_error("lora: failed to unmarshal frame control");
    }
    _adr = (data[0] & 0x80) != 0;
    _adr_ack_req = (data[0] & 0x40) != 0;
    _ack = (data[0] & 0x20) != 0;
    _class_b = (data[0] & 0x10) != 0;
    _f_pending = (data[0] & 0x10) != 0;
    _f_opts_len = data[0] & 0x0f;
}

std::vector<byte> fhdr::marshal_binary() {
    std::vector<byte> res;
    std::vector<byte> bytes;

    // Marshal frame options
    std::vector<byte> f_opts_bytes;
    for (const auto& mac : _f_opts) {
        bytes = mac->marshal_binary();
        f_opts_bytes.insert(f_opts_bytes.end(), bytes.begin(), bytes.end());
    }
    _f_ctrl._f_opts_len = static_cast<uint8_t>(f_opts_bytes.size());
    if (_f_ctrl._f_opts_len > 15) {
        throw std::runtime_error("lora: max value of f_opts_len is 15");
    }
    res.reserve(7 + _f_ctrl._f_opts_len);

    // Marshal device address
    bytes = _dev_addr.marshal_binary();
    res.insert(res.end(), bytes.begin(), bytes.end());

    // Marshal frame control
    bytes = _f_ctrl.marshal_binary();
    res.insert(res.end(), bytes.begin(), bytes.end());

    // Marshal frame count
    std::array<byte, sizeof(_f_cnt)> f_cnt_bytes{};
    std::memcpy(f_cnt_bytes.data(), &_f_cnt, sizeof(_f_cnt));
    res.insert(res.end(), f_cnt_bytes.begin(), f_cnt_bytes.begin() + 2);

    // Append frame options and return
    res.insert(res.end(), f_opts_bytes.begin(), f_opts_bytes.end());
    return res;
}

void fhdr::unmarshal_binary(const std::vector<byte>& data) {
    if (data.size() < 7) {
        throw std::runtime_error("lora: failed to unmarshal frame header");
    }
    std::vector<byte> bytes;

    // Unmarshal device address
    bytes = {data.begin(), data.begin() + 4};
    _dev_addr.unmarshal_binary(bytes);

    // Unmarshal frame control
    bytes = {data.begin() + 4, data.begin() + 5};
    _f_ctrl.unmarshal_binary(bytes);

    // Unmarshal frame count
    bytes = {data.begin() + 5, data.begin() + 7};
    bytes.push_back(0x00);
    bytes.push_back(0x00);
    std::array<byte, sizeof(_f_cnt)> f_cnt_bytes{};
    for (int i = 0; i < bytes.size(); ++i) {
        f_cnt_bytes[f_cnt_bytes.size() - (i + 1)] = bytes[i];
    }
    std::memcpy(&_f_cnt, f_cnt_bytes.data(), sizeof(_f_cnt));

    // Unmarshal frame options
    if (data.size() > 7) {
        bytes = {data.begin() + 7, data.end()};
        data_payload payload;
        payload._data = std::move(bytes);
        _f_opts.push_back(std::make_unique<data_payload>(std::move(payload)));
    }
}

}
}