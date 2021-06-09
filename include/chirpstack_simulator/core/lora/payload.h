//
// Created by chungphb on 4/6/21.
//

#pragma once

#include <chirpstack_simulator/core/lora/fhdr.h>
#include <chirpstack_simulator/core/lora/net_id.h>
#include <memory>

namespace chirpstack_simulator {
namespace lora {

enum struct join_type : uint8_t {
    join_request_type = 0xff,
    rejoin_request_type_0 = 0x00,
    rejoin_request_type_1 = 0x01,
    rejoin_request_type_2 = 0x02
};

struct eui64 {
    std::vector<byte> marshal_binary();
    void unmarshal_binary(const std::vector<byte>& data);
    std::basic_string<byte> string();
    bool operator<(const eui64& rhs) const;
    std::array<byte, 8> _value;
};

struct dev_nonce {
    std::vector<byte> marshal_binary();
    void unmarshal_binary(const std::vector<byte>& data);
    uint16_t _value;
};

struct join_nonce {
    std::vector<byte> marshal_binary();
    void unmarshal_binary(const std::vector<byte>& data);
    uint32_t _value;
};

struct dl_settings {
    std::vector<byte> marshal_binary();
    void unmarshal_binary(const std::vector<byte>& data);
    bool _opt_neg;
    uint8_t _rx2_data_rate;
    uint8_t _rx1_dr_offset;
};

struct payload {
    virtual std::vector<byte> marshal_binary() = 0;
    virtual void unmarshal_binary(const std::vector<byte>& data, bool uplink) = 0;
};

struct data_payload : public payload {
    std::vector<byte> marshal_binary() override;
    void unmarshal_binary(const std::vector<byte>& data, bool uplink) override;
    std::vector<byte> _data;
};

struct join_request_payload : public payload {
    std::vector<byte> marshal_binary() override;
    void unmarshal_binary(const std::vector<byte>& data, bool uplink) override;
    eui64 _join_eui;
    eui64 _dev_eui;
    dev_nonce _dev_nonce;
};

enum struct cf_list_type : uint8_t {
    cf_list_channel = 0,
    cf_list_channel_mask = 1
};

struct cf_list {
    std::vector<byte> marshal_binary();
    void unmarshal_binary(const std::vector<byte>& data);
    std::unique_ptr<payload> _payload;
    cf_list_type _type;
};

struct cf_list_channel_payload : public payload {
    std::vector<byte> marshal_binary() override;
    void unmarshal_binary(const std::vector<byte>& data, bool uplink) override;
    std::array<uint32_t, 5> _channels;
};

struct ch_mask {
    std::vector<byte> marshal_binary();
    void unmarshal_binary(const std::vector<byte>& data);
    operator bool() const;
    std::array<bool, 16> _value;
};

struct cf_list_channel_mask_payload : public payload {
    std::vector<byte> marshal_binary() override;
    void unmarshal_binary(const std::vector<byte>& data, bool uplink) override;
    std::vector<ch_mask> _channel_masks;
};

struct join_accept_payload : public payload {
    std::vector<byte> marshal_binary() override;
    void unmarshal_binary(const std::vector<byte>& data, bool uplink) override;
    join_nonce _join_nonce;
    net_id _home_net_id;
    dev_addr _dev_addr;
    dl_settings _dl_settings;
    uint8_t _rx_delay;
    std::unique_ptr<cf_list> _cf_list;
};

struct rejoin_request_type_02_payload : public payload {
    std::vector<byte> marshal_binary() override;
    void unmarshal_binary(const std::vector<byte>& data, bool uplink) override;
    join_type _rejoin_type;
    net_id _net_id;
    eui64 _dev_eui;
    uint16_t _rj_count_0;
};

struct rejoin_request_type_1_payload : public payload {
    std::vector<byte> marshal_binary() override;
    void unmarshal_binary(const std::vector<byte>& data, bool uplink) override;
    join_type _rejoin_type;
    eui64 _join_eui;
    eui64 _dev_eui;
    uint16_t _rj_count_1;
};

}
}