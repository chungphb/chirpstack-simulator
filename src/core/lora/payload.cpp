//
// Created by chungphb on 4/6/21.
//

#include <chirpstack_simulator/core/lora/payload.h>
#include <chirpstack_simulator/util/helper.h>
#include <sstream>
#include <iostream>
#include <iomanip>

namespace chirpstack_simulator {
namespace lora {

std::vector<byte> eui64::marshal_binary() {
    return {};
}

void eui64::unmarshal_binary(const std::vector<byte>& data) {

}

std::basic_string<byte> eui64::string() {
    return to_hex_string(_value.data(), _value.size());
}

std::vector<byte> dev_nonce::marshal_binary() {
    return {};
}

void dev_nonce::unmarshal_binary(const std::vector<byte>& data) {

}

std::vector<byte> join_nonce::marshal_binary() {
    return {};
}

void join_nonce::unmarshal_binary(const std::vector<byte>& data) {

}

std::vector<byte> dl_settings::marshal_binary() {
    return {};
}

void dl_settings::unmarshal_binary(const std::vector<byte>& data) {

}

std::vector<byte> data_payload::marshal_binary() {
    return {};
}

void data_payload::unmarshal_binary(const std::vector<byte>& data, bool uplink) {

}

std::vector<byte> join_request_payload::marshal_binary() {
    return {};
}

void join_request_payload::unmarshal_binary(const std::vector<byte>& data, bool uplink) {

}

std::vector<byte> cf_list::marshal_binary() {
    return {};
}

void cf_list::unmarshal_binary(const std::vector<byte>& data) {

}

std::vector<byte> cf_list_channel_payload::marshal_binary() {
    return {};
}

void cf_list_channel_payload::unmarshal_binary(const std::vector<byte>& data, bool uplink) {

}

std::vector<byte> cf_list_channel_mask_payload::marshal_binary() {
    return {};
}

void cf_list_channel_mask_payload::unmarshal_binary(const std::vector<byte>& data, bool uplink) {

}

std::vector<byte> join_accept_payload::marshal_binary() {
    return {};
}

void join_accept_payload::unmarshal_binary(const std::vector<byte>& data, bool uplink) {

}

std::vector<byte> rejoin_request_type_02_payload::marshal_binary() {
    return {};
}

void rejoin_request_type_02_payload::unmarshal_binary(const std::vector<byte>& data, bool uplink) {

}

std::vector<byte> rejoin_request_type_1_payload::marshal_binary() {
    return {};
}

void rejoin_request_type_1_payload::unmarshal_binary(const std::vector<byte>& data, bool uplink) {

}

}
}