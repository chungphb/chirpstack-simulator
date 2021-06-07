//
// Created by chungphb on 4/6/21.
//

#pragma once

#include <chirpstack_simulator/util/data_types.h>
#include <memory>

namespace chirpstack_simulator {
namespace lora {

struct payload;

struct dev_addr {
    std::vector<byte> marshal_binary();
    void unmarshal_binary(const std::vector<byte>& data);
    std::array<byte, 4> _value;
};

struct f_ctrl {
    std::vector<byte> marshal_binary();
    void unmarshal_binary(const std::vector<byte>& data);
    bool _adr;
    bool _adr_ack_req;
    bool _ack;
    bool _f_pending;
    bool _class_b;
    uint8_t _f_opts_len;
};

struct fhdr {
    std::vector<byte> marshal_binary();
    void unmarshal_binary(const std::vector<byte>& data);
    dev_addr _dev_addr;
    f_ctrl _f_ctrl;
    uint32_t _f_cnt;
    std::vector<std::unique_ptr<payload>> _f_opts;
};

}
}