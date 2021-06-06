//
// Created by chungphb on 5/6/21.
//

#pragma once

#include <chirpstack_simulator/core/lora/payload.h>

namespace chirpstack_simulator {
namespace lora {

struct mac_payload : public payload {
    std::vector<byte> marshal_binary() override;
    void unmarshal_binary(const std::vector<byte>& data, bool uplink) override;
    fhdr _fhdr;
    std::unique_ptr<uint8_t> _f_port;
    std::vector<std::unique_ptr<payload>> _frm_payload;
};

}
}