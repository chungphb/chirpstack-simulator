//
// Created by chungphb on 4/6/21.
//

#include <chirpstack_simulator/core/lora/net_id.h>

namespace chirpstack_simulator {
namespace lora {

std::vector<byte> net_id::marshal_binary() {
    std::vector<byte> res{_value.rbegin(), _value.rend()};
    return res;
}

void net_id::unmarshal_binary(const std::vector<byte>& data) {
    if (data.size() != _value.size()) {
        throw std::runtime_error("lora: failed to unmarshal net id");
    }
    for (int i = 0; i < _value.size(); ++i) {
        _value[_value.size() - (i + 1)] = data[i];
    }
}

}
}