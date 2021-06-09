//
// Created by chungphb on 25/5/21.
//

#pragma once

#include <string>
#include <vector>
#include <array>
#include <cstring>
#include <climits>

namespace chirpstack_simulator {

using byte = char;
constexpr byte BYTE_MIN = CHAR_MIN;
constexpr byte BYTE_MAX = CHAR_MAX;
using byte_array = std::basic_string<byte>;

struct server_address {
    std::string _host;
    uint16_t _port;
};

}