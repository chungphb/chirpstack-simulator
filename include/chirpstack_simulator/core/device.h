//
// Created by chungphb on 25/5/21.
//

#pragma once

#include <chirpstack_simulator/core/gateway.h>

namespace chirpstack_simulator {

struct device {
public:
    device(std::vector<byte> addr, const config& config);
    void send_payload(const gateway& gateway, int socket_fd, const sockaddr_in& server_addr);
    std::string addr() const;
    std::string eui() const;

private:
    payload generate_payload(message_type m_type);

private:
    std::vector<byte> _addr;
    std::vector<byte> _eui;
    int _frame_cnt = 0;
    const config& _config;
};

}
