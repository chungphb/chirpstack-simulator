//
// Created by chungphb on 4/6/21.
//

#include <chirpstack_simulator/core/lora/phy_payload.h>

namespace chirpstack_simulator {
namespace lora {

std::vector<byte> aes128key::marshal_binary() {
    return {};
}

void aes128key::unmarshal_binary(const std::vector<byte>& data) {

}

std::vector<byte> mhdr::marshal_binary() {
    return {};
}

void mhdr::unmarshal_binary(const std::vector<byte>& data) {

}

void phy_payload::set_uplink_data_mic(const uplink_data_info& info) {

}

bool phy_payload::validate_uplink_data_mic(const uplink_data_info& info) {
    return true;
}

bool phy_payload::validate_uplink_data_micf(aes128key f_nwk_s_int_key) {
    return true;
}

mic phy_payload::calculate_uplink_data_mic(const uplink_data_info& info) {
    return mic{};
}

void phy_payload::set_downlink_data_mic(const downlink_data_info& info) {

}

bool phy_payload::validate_downlink_data_mic(const downlink_data_info& info) {
    return true;
}

mic phy_payload::calculate_downlink_data_mic(const downlink_data_info& info) {
    return mic{};
}

void phy_payload::set_uplink_join_mic(const uplink_join_info& info) {

}

bool phy_payload::validate_uplink_join_mic(const uplink_join_info& info) {
    return true;
}

mic phy_payload::calculate_uplink_join_mic(const uplink_join_info& info) {
    return mic{};
}

void phy_payload::set_downlink_join_mic(const downlink_join_info& info) {

}

bool phy_payload::validate_downlink_join_mic(const downlink_join_info& info) {
    return true;
}

mic phy_payload::calculate_downlink_join_mic(const downlink_join_info& info) {
    return mic{};
}

void phy_payload::encrypt_f_opts(aes128key nwk_s_enc_key) {

}

void phy_payload::decrypt_f_opts(aes128key nwk_s_enc_key) {

}

void phy_payload::decode_f_opts_to_mac_commands() {

}

std::vector<byte> phy_payload::encrypt_f_opts(const f_opts_info& info) {
    return {};
}

void phy_payload::encrypt_frm_payload(aes128key key) {

}

void phy_payload::decrypt_frm_payload(aes128key key) {

}

void phy_payload::decode_frm_payload_to_mac_commands() {

}

std::vector<byte> phy_payload::encrypt_frm_payload(const frm_payload_info& info) {
    return {};
}

std::vector<byte> phy_payload::marshal_binary() {
    std::vector<byte> res;
    std::string sample = "test_packet_1234";
    std::copy(sample.begin(), sample.end(), std::back_inserter(res));
    return res;
}

void phy_payload::unmarshal_binary(const std::vector<byte>& data) {

}

bool phy_payload::is_uplink() {
    return true;
}

}
}