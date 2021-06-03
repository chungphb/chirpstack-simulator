//
// Created by chungphb on 24/5/21.
//

#pragma once

#include <chirpstack_simulator/util/data_types.h>
#include <string>
#include <vector>

namespace chirpstack_simulator {

struct device;
struct gateway;

struct payload {
public:
    byte_array as_byte_array() const;
    friend device;
    friend gateway;

private:
    int _f_port;
    int _f_cnt;
    std::vector<byte> _dev_addr;
    message_type _m_type;
    byte_array _data;
};

}