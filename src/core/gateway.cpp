//
// Created by chungphb on 25/5/21.
//

#include <chirpstack_simulator/core/gateway.h>
#include <chirpstack_simulator/util/helper.h>
#include <spdlog/spdlog.h>
#include <algorithm>

namespace chirpstack_simulator {

gateway::gateway(std::vector<byte> mac, const config& config) : _mac{std::move(mac)}, _config{config} {}

void gateway::push_data(int socket_fd, const sockaddr_in& server_addr, payload&& payload) const {
    auto pk_id = payload._f_cnt;
    auto&& gw_mac = to_hex_string(_mac);
    auto&& dev_addr = to_hex_string(payload._dev_addr);

    // Create Semtech UDP packet
    auto&& data = generate_data(payload.as_byte_array());
    byte_array packet(GW_MAC_LEN + 4, 0);
    packet[0] = 0x02;
    packet[1] = get_random_byte();
    packet[2] = get_random_byte();
    packet[3] = 0x00;
    for (size_t i = 0; i < GW_MAC_LEN; ++i) {
        packet[i + 4] = _mac[i];
    }
    packet += data.as_byte_array();

    // Send packet
    sendto(socket_fd, packet.c_str(), packet.length(), MSG_CONFIRM, (const sockaddr*)&server_addr, sizeof(server_addr));
    spdlog::info("Gateway {}: Send packet #{} from device {}", gw_mac, pk_id, dev_addr);

    // Receive ACK
    byte ack[ACK_MAX_LEN];
    auto ack_len = timeout_recvfrom(socket_fd, ack, ACK_MAX_LEN, server_addr, 10);
    if (ack_len < 0) {
        spdlog::info("Gateway {}: Failed to receive ACK for packet #{} from device {}", gw_mac, pk_id, dev_addr);
    } else {
        ack[ack_len] = '\0';
        if (ack_len == 4) {
            spdlog::info("Gateway {}: Receive valid ACK for packet #{} from device {}", gw_mac, pk_id, dev_addr);
        } else {
            spdlog::info("Gateway {}: Receive invalid ACK for packet #{} from device {}", gw_mac, pk_id, dev_addr);
        }
    }
}

rxpk gateway::generate_data(byte_array&& payload) const {
    rxpk data;
    data.add("time", get_current_timestamp());
    data.add("tmst", 3512348514);
    data.add("chan", 2);
    data.add("rfch", 0);
    data.add("freq", _config._freq / 1000000.);
    data.add("stat", 1);
    data.add("modu", "FSK");
    data.add("datr", 50000);
    data.add("rssi", -75);
    data.add("size", payload.size());
    data.add("data", base64_encode(payload));
    return data;
}

}
