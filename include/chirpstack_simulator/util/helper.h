//
// Created by chungphb on 25/5/21.
//

#pragma once

#include <chirpstack_simulator/util/data_types.h>
#include <netinet/in.h>
#include <random>

namespace chirpstack_simulator {

// Get random values
template <typename T = size_t>
T get_random_number(T min, T max) {
    std::random_device rd;
    std::mt19937 mt{rd()};
    std::uniform_int_distribution<T> dist{min, max};
    return dist(mt);
}

byte get_random_byte(byte min = BYTE_MIN, byte max = BYTE_MAX);
std::string get_random_eui64();
std::string get_random_aes128key();
std::string get_random_uuid_v4();

// Get time related values
std::string get_current_timestamp();
size_t get_time_since_epoch();

// Encode to and decode from base64 format
std::string base64_encode(const std::string& in);
std::string base64_decode(const std::string& in);

// Convert to string
std::string to_string(const server_address& addr);
std::string to_hex_string(const byte* str, size_t len);

// Extract information
std::string get_host(const std::string& host_name);
uint16_t get_port(const std::string& host_name);

// Receive UDP packet with timeout
ssize_t timeout_recvfrom(int fd, char* buf, ssize_t buf_len, const sockaddr_in& addr, int t_sec);

}