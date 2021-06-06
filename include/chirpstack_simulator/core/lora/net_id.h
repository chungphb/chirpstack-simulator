//
// Created by chungphb on 4/6/21.
//

#pragma once

#include <chirpstack_simulator/util/data_types.h>

namespace chirpstack_simulator {
namespace lora {

struct net_id {
    std::vector<byte> marshal_binary();
    void unmarshal_binary(const std::vector<byte>& data);
    std::array<byte, 3> _value;
};

}
}
