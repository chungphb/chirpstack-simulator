//
// Created by chungphb on 5/6/21.
//

#include <chirpstack_simulator/core/lora/mac_payload.h>

namespace chirpstack_simulator {
namespace lora {

std::vector<byte> mac_payload::marshal_binary() {
    std::vector<byte> res;
    std::vector<byte> bytes;

    // Marshal frame header
    bytes = _fhdr.marshal_binary();
    res.insert(res.end(), bytes.begin(), bytes.end());

    // Marshal frame port
    if (!_f_port) {
        if (!_frm_payload.empty()) {
            throw std::runtime_error("lora: frm_payload is not empty");
        }
        return res;
    }
    if (!_fhdr._f_opts.empty() && *_f_port == 0) {
        throw std::runtime_error("lora: f_opts are set");
    }
    res.push_back(static_cast<byte>(*_f_port));

    // Marshal payloads
    bytes = marshal_payload();
    res.insert(res.end(), bytes.begin(), bytes.end());

    // Return
    return res;
}

void mac_payload::unmarshal_binary(const std::vector<byte>& data, bool uplink) {
    if (data.size() < 7) {
        throw std::runtime_error("lora: failed to unmarshal mac payload");
    }
    std::vector<byte> bytes;

    // Unmarshal frame control to continue checking size
    bytes = {data.begin() + 4, data.begin() + 5};
    _fhdr._f_ctrl.unmarshal_binary(bytes);

    if (data.size() < 7 + _fhdr._f_ctrl._f_opts_len) {
        throw std::runtime_error("lora: failed to unmarshal mac payload");
    }

    // Unmarshal frame header
    bytes = {data.begin(), data.begin() + 7 + _fhdr._f_ctrl._f_opts_len};
    _fhdr.unmarshal_binary(bytes);

    // Unmarshal frame port
    if (data.size() > 7 + _fhdr._f_ctrl._f_opts_len) {
        _f_port = std::make_unique<uint8_t>(data[7 + _fhdr._f_ctrl._f_opts_len]);
    }

    // Unmarshal frame payloads
    if (data.size() > 7 + _fhdr._f_ctrl._f_opts_len + 1) {
        if (_f_port && *_f_port == 0 && _fhdr._f_ctrl._f_opts_len > 0) {
            throw std::runtime_error("lora: f_opts are set");
        }
        bytes = {data.begin() + 7 + _fhdr._f_ctrl._f_opts_len + 1, data.end()};
        data_payload payload;
        payload._data = std::move(bytes);
        _frm_payload.push_back(std::make_shared<data_payload>(std::move(payload)));
    }
}

std::vector<byte> mac_payload::marshal_payload() {
    std::vector<byte> res;
    std::vector<byte> bytes;
    for (const auto& payload : _frm_payload) {
        bytes = payload->marshal_binary(); // TODO: Support MAC command
        res.insert(res.end(), bytes.begin(), bytes.end());
    }
    return res;
}

}
}