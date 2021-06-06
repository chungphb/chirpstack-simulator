//
// Created by chungphb on 4/6/21.
//

#include <chirpstack_simulator/core/lora/fhdr.h>

namespace chirpstack_simulator {
namespace lora {

std::vector<byte> dev_addr::marshal_binary() {
    return {};
}

void dev_addr::unmarshal_binary(const std::vector<byte>& data) {

}

std::vector<byte> f_ctrl::marshal_binary() {
    return {};
}

void f_ctrl::unmarshal_binary(const std::vector<byte>& data) {

}

std::vector<byte> fhdr::marshal_binary() {
    return {};
}

void fhdr::unmarshal_binary(const std::vector<byte>& data) {

}

}
}