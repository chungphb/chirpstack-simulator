//
// Created by chungphb on 25/5/21.
//

#include <chirpstack_simulator/core/device.h>
#include <chirpstack_simulator/util/helper.h>

namespace chirpstack_simulator {

device::device(std::vector<byte> addr, const config& config) : _addr{std::move(addr)}, _config{config} {
    for (int i = 0; i < 8; ++i) {
        _eui.push_back(get_random_byte(0, 127));
    }
}

void device::send_payload(const gateway& gateway, int socket_fd, const sockaddr_in& server_addr) {
    auto&& payload = generate_payload(message_type::unconfirmed_data_up);
    gateway.push_data(socket_fd, server_addr, std::move(payload));
}

std::string device::addr() const {
    return to_hex_string(_addr);
}

std::string device::eui() const {
    return to_hex_string(_eui);
}

payload device::generate_payload(message_type m_type) {
    payload new_payload;
    switch (m_type) {
        case message_type::unconfirmed_data_up: {
            new_payload._f_port = _config._f_port;
            new_payload._f_cnt = _frame_cnt++;
            new_payload._dev_addr = _addr;
            new_payload._m_type = m_type;
            new_payload._data = _config._payload;
            break;
        }
        default: {
            break;
        }
    }
    return new_payload;
}

}