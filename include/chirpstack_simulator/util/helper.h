//
// Created by chungphb on 25/5/21.
//

#pragma once

#include <chirpstack_simulator/util/data_types.h>
#include <netinet/in.h>

namespace chirpstack_simulator {

// Get random values
size_t get_random(size_t min, size_t max);
byte get_random_byte();

// Get time related values
std::string get_current_timestamp();
size_t get_time_since_epoch();

// Encode to and decode from base64 format
std::string base64_encode(const std::string& in);
std::string base64_decode(const std::string& in);

// Convert to string
std::string to_string(const server_address& addr);
std::string to_hex_string(const std::vector<byte>& vec);
std::string to_hex_string(const byte* str, size_t len);

// Extract information
std::string get_host(const std::string& host_name);
uint16_t get_port(const std::string& host_name);

// Receive UDP packet with timeout
ssize_t timeout_recvfrom(int fd, char* buf, ssize_t buf_len, const sockaddr_in& addr, int t_sec);

}