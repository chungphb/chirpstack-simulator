//
// Created by chungphb on 4/6/21.
//

#pragma once

#include <chirpstack_simulator/core/lora/payload.h>
#include <memory>

namespace chirpstack_simulator {
namespace lora {

enum struct m_type : byte {
    join_request = 0,
    join_accept = 1,
    unconfirmed_data_up = 2,
    unconfirmed_data_down = 3,
    confirmed_data_up = 4,
    confirmed_data_down = 5,
    rejoin_request = 6,
    proprietary = 7
};

enum struct major : byte {
    lorawan_r1 = 0
};

enum struct mac_version : byte {
    lorawan_1_0 = 0,
    lorawan_1_1 = 1
};

struct aes128key {
    std::vector<byte> marshal_binary();
    void unmarshal_binary(const std::vector<byte>& data);
    std::basic_string<byte> string();
    std::array<byte, 16> _value;
};

struct mic {
    bool operator==(const mic& rhs) const;
    bool operator!=(const mic& rhs) const;
    std::array<byte, 4> _value;
};

struct mhdr {
    std::vector<byte> marshal_binary();
    void unmarshal_binary(const std::vector<byte>& data);
    m_type _m_type;
    major _major;
};

struct phy_payload {
    struct uplink_data_info {
        mac_version _m_ver;
        uint32_t _conf_f_cnt;
        uint8_t _tx_dr;
        uint8_t _tx_ch;
        aes128key _f_nwk_s_int_key;
        aes128key _s_nwk_s_int_key;
    };

    struct downlink_data_info {
        mac_version _m_ver;
        uint32_t _conf_f_cnt;
        aes128key _s_nwk_s_int_key;
    };

    struct uplink_join_info {
        aes128key _key;
    };

    struct downlink_join_info {
        join_type _join_req_type;
        eui64 _join_eui;
        dev_nonce _dev_nonce;
        aes128key _key;
    };

    struct frm_payload_info {
        aes128key _key;
        bool _is_uplink;
        dev_addr _dev_addr;
        uint32_t _f_cnt;
        std::vector<byte> _data;
    };

    // For uplink data frame
    void set_uplink_data_mic(uplink_data_info& info);
    bool validate_uplink_data_mic(uplink_data_info& info);
    bool validate_uplink_data_micf(aes128key f_nwk_s_int_key);
    mic calculate_uplink_data_mic(uplink_data_info& info);

    // For downlink data frame
    void set_downlink_data_mic(downlink_data_info& info);
    bool validate_downlink_data_mic(downlink_data_info& info);
    mic calculate_downlink_data_mic(downlink_data_info& info);

    // For uplink join request
    void set_uplink_join_mic(uplink_join_info& info);
    bool validate_uplink_join_mic(uplink_join_info& info);
    mic calculate_uplink_join_mic(uplink_join_info& info);

    // For downlink join request
    void set_downlink_join_mic(downlink_join_info& info);
    bool validate_downlink_join_mic(downlink_join_info& info);
    mic calculate_downlink_join_mic(downlink_join_info& info);

    // For join accept payload
    void encrypt_join_accept_payload(aes128key key);
    void decrypt_join_accept_payload(aes128key key);

    // For frame payload
    void encrypt_frm_payload(aes128key key);
    void decrypt_frm_payload(aes128key key);
    std::vector<byte> encrypt_frm_payload(frm_payload_info& info);

    // For physical payload
    std::vector<byte> marshal_binary();
    void unmarshal_binary(const std::vector<byte>& data);
    bool is_uplink();

    mhdr _mhdr;
    std::shared_ptr<payload> _mac_payload;
    mic _mic;
};

}
}