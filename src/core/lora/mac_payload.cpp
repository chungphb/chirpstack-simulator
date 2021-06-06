//
// Created by chungphb on 5/6/21.
//

#include <chirpstack_simulator/core/lora/mac_payload.h>

namespace chirpstack_simulator {
namespace lora {

std::vector<byte> mac_payload::marshal_binary() {
    return {};
}

void mac_payload::unmarshal_binary(const std::vector<byte>& data, bool uplink) {

}

}
}