//
// Created by chungphb on 25/5/21.
//

#include <chirpstack_simulator/util/helper.h>
#include <chirpstack_simulator/util/config.h>
#include <date/date.h>
#include <random>
#include <sstream>
#include <iomanip>
#include <chrono>

namespace chirpstack_simulator {

size_t get_random(size_t min, size_t max) {
    std::random_device rd;
    std::mt19937 mt{rd()};
    std::uniform_int_distribution<size_t> dist{min, max};
    return dist(mt);
}

byte get_random_byte() {
    size_t res = get_random(0, 127);
    return static_cast<byte>(res);
}

std::string get_current_timestamp() {
    auto now = std::chrono::system_clock::now();
    return date::format("%FT%TZ", date::floor<std::chrono::microseconds>(now));
}

size_t get_time_since_epoch() {
    auto now = std::chrono::system_clock::now();
    return now.time_since_epoch().count();
}

const char* base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string base64_encode(const std::string& in) {
    std::string out;
    unsigned int val = 0;
    int valb = -6;
    for (unsigned char c : in) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(base64_chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) {
        out.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    while (out.size() % 4) {
        out.push_back('=');
    }
    return out;
}

std::string base64_decode(const std::string& in) {
    std::string out;
    std::vector<int> tmp(256, -1);
    for (int i = 0; i < 64; ++i) {
        tmp[base64_chars[i]] = i;
    }
    unsigned int val = 0;
    int valb = -8;
    for (unsigned char c : in) {
        if (tmp[c] == -1) {
            break;
        }
        val = (val << 6) + tmp[c];
        valb += 6;
        if (valb >= 0) {
            out.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

std::string to_string(const server_address& addr) {
    return addr._host + ":" + std::to_string(addr._port);
}

std::string to_hex_string(const std::vector<byte>& vec) {
    std::stringstream ss;
    for (const auto& ele : vec) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)ele;
    }
    return ss.str();
}

std::string to_hex_string(const byte* str, size_t len) {
    std::stringstream ss;
    for (size_t i = 0; i < len; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << str[i];
    }
    return ss.str();
}

std::string get_host(const std::string& host_name) {
    auto colon_pos = host_name.find(':');
    if (colon_pos == std::string::npos) {
        throw std::invalid_argument("Invalid host name");
    }
    return host_name.substr(0, colon_pos);
}

uint16_t get_port(const std::string& host_name) {
    auto colon_pos = host_name.find(':');
    if (colon_pos == std::string::npos) {
        throw std::invalid_argument("Invalid host name");
    }
    return std::stoi(host_name.substr(colon_pos + 1));
}

ssize_t timeout_recvfrom(int fd, char* buf, ssize_t buf_len, const sockaddr_in& addr, int t_sec) {
    fd_set fds;
    timeval timeout{};
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    timeout.tv_sec = t_sec;
    auto res = select(fd + 1, &fds, nullptr, nullptr, &timeout);
    if (res == -1 || res == 0) {
        return -1;
    } else {
        size_t addr_len = 0;
        return recvfrom(fd, buf, buf_len, MSG_WAITALL, (sockaddr*)&addr, (socklen_t*)&addr_len);
    }
}

}