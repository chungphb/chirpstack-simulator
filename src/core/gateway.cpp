//
// Created by chungphb on 25/5/21.
//

#include <chirpstack_simulator/core/gateway.h>
#include <chirpstack_simulator/util/helper.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <thread>
#include <chrono>

namespace chirpstack_simulator {

using namespace std::chrono_literals;

void gateway::run() {
    if ((_push_socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        spdlog::critical("Failed to create socket file descriptor");
        exit(EXIT_FAILURE);
    }
    if ((_pull_socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        spdlog::critical("Failed to create socket file descriptor");
        exit(EXIT_FAILURE);
    }
    _keep_alive = std::async(std::launch::async, &gateway::keep_alive, this);
    _handle_downlink_frame = std::async(std::launch::async, &gateway::handle_downlink_frame, this);
}

void gateway::stop() {
    _stopped = true;
    _connected = false;
    _handle_downlink_frame.get();
    _keep_alive.get();
    close(_push_socket_fd);
    close(_pull_socket_fd);
}

void gateway::send_uplink_frame(gw::UplinkFrame frame) {
    // Generate PUSH_DATA packet
    gw::UplinkRXInfo* rx_info = frame.mutable_rx_info();
    *rx_info = _uplink_rx_info;
    auto packet = generate_push_data_packet(frame);

    // Send PUSH_DATA packet
    sendto(_push_socket_fd, packet.data(), packet.size(), MSG_CONFIRM, (const sockaddr*)&_server, sizeof(_server));
    spdlog::debug("Gateway {}: Send PUSH_DATA packet", _gateway_id.string());

    // Handle PUSH_ACK packet
    byte ack[1024];
    auto ack_len = timeout_recvfrom(_push_socket_fd, ack, 1024, _server, 4);
    if (_stopped) {
        return;
    }
    if (ack_len > 0) {
        ack[ack_len] = '\0';
        if (ack_len == 4) {
            spdlog::debug("Gateway {}: Receive PUSH_ACK packet", _gateway_id.string());
        } else {
            spdlog::error("Gateway {}: Receive invalid ACK packet", _gateway_id.string());
        }
    } else {
        spdlog::error("Gateway {}: Not receive any packet", _gateway_id.string());
    }
}

void gateway::keep_alive() {
    while (!_stopped) {
        // Generate PULL_DATA packet
        auto packet = generate_pull_data_packet();

        // Send PULL_DATA packet
        sendto(_pull_socket_fd, packet.data(), packet.size(), MSG_CONFIRM, (const sockaddr*)&_server, sizeof(_server));
        spdlog::debug("Gateway {}: Send PULL_DATA packet", _gateway_id.string());

        // Wait for the next cycle
        std::this_thread::sleep_for(1s);
        if (_connected) {
            std::this_thread::sleep_for(15s);
        }
    }
}

void gateway::handle_downlink_frame() {
    while (!_stopped) {
        // Handle PULL_RESP / PULL_ACK packet
        byte ack[1024];
        auto ack_len = timeout_recvfrom(_pull_socket_fd, ack, 1024, _server, 16);
        if (_stopped) {
            return;
        }
        if (ack_len > 0) {
            ack[ack_len] = '\0';
            if (ack_len > 4) {
                spdlog::debug("Gateway {}: Receive PULL_RESP packet", _gateway_id.string());
            } else if (ack_len == 4) {
                spdlog::debug("Gateway {}: Receive PULL_ACK packet", _gateway_id.string());
                _connected = true;
            } else {
                spdlog::error("Gateway {}: Receive invalid ACK packet", _gateway_id.string());
            }
        } else if (ack_len < 0) {
            spdlog::error("Gateway {}: Not receive any packet", _gateway_id.string());
            _connected = false;
        }

        // Wait for next cycle
        std::this_thread::sleep_for(50ms);
    }
}

std::vector<byte> gateway::generate_push_data_packet(const gw::UplinkFrame& payload) {
    std::vector<byte> packet;

    // Set protocol version
    packet.push_back(0x02);

    // Set random token
    packet.push_back(get_random_byte());
    packet.push_back(get_random_byte());

    // Set PUSH_DATA identifier
    packet.push_back(0x00);

    // Set gateway identifier
    for (const auto& byte : _gateway_id._value) {
        packet.push_back(byte);
    }

    // Set JSON object
    rxpk data;
    data.add("time", get_current_timestamp());
    data.add("tmst", get_time_since_epoch());
    data.add("chan", payload.rx_info().channel());
    data.add("rfch", payload.rx_info().rf_chain());
    data.add("freq", payload.tx_info().frequency() / 1000000.);
    data.add("stat", payload.rx_info().crc_status() == gw::CRC_OK ? 1 : 0);
    if (payload.tx_info().modulation() == common::LORA) {
        data.add("modu", "LORA");
        if (payload.tx_info().has_lora_modulation_info()) {
            std::basic_stringstream<byte> datr;
            datr << "SF" << payload.tx_info().lora_modulation_info().spreading_factor();
            datr << "BW" << payload.tx_info().lora_modulation_info().bandwidth();
            data.add("datr", datr.str());
            data.add("codr", payload.tx_info().lora_modulation_info().code_rate());
        }
    } else {
        data.add("modu", "FSK");
        if (payload.tx_info().has_fsk_modulation_info()) {
            data.add("datr", payload.tx_info().fsk_modulation_info().datarate());
        }
    }
    data.add("rssi", payload.rx_info().rssi());
    data.add("lsnr", payload.rx_info().lora_snr());
    data.add("size", payload.phy_payload().size());
    data.add("data", base64_encode(payload.phy_payload()));
    auto&& data_str = data.string();
    std::copy(data_str.begin(), data_str.end(), std::back_inserter(packet));

    // Return
    return packet;
}

std::vector<byte> gateway::generate_pull_data_packet() {
    std::vector<byte> packet;

    // Set protocol version
    packet.push_back(0x02);

    // Set random token
    packet.push_back(get_random_byte());
    packet.push_back(get_random_byte());

    // Set PULL_DATA identifier
    packet.push_back(0x02);

    // Set gateway identifier
    for (const auto& byte : _gateway_id._value) {
        packet.push_back(byte);
    }

    // Return
    return packet;
}

void rxpk::add(std::string field_name, const std::string& field_value) {
    std::stringstream ss;
    ss << R"(")" << field_value << R"(")";
    _fields.emplace_back(std::move(field_name), ss.str());
}

void rxpk::add(std::string field_name, const char* field_value) {
    std::stringstream ss;
    ss << R"(")" << field_value << R"(")";
    _fields.emplace_back(std::move(field_name), ss.str());
}

std::string rxpk::string() const {
    std::stringstream ss;
    ss << R"({"rxpk":[{)";
    for (auto it = _fields.begin(); it != _fields.end();) {
        ss << R"(")" << it->first << R"(":)" << it->second;
        ss << (std::next(it) != _fields.end() ? "," : "");
        it = std::next(it);
    }
    ss << R"(}]})";
    return ss.str();
}

}
