//
// Created by chungphb on 25/5/21.
//

#include <chirpstack_simulator/core/gateway.h>
#include <chirpstack_simulator/util/helper.h>
#include <spdlog/spdlog.h>
#include <json11.hpp>
#include <sstream>
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
    _connected = false;
    if (_handle_downlink_frame.valid()) {
        _handle_downlink_frame.get();
    }
    if (_keep_alive.valid()) {
        _keep_alive.get();
    }
    if (_push_socket_fd >= 0) {
        close(_push_socket_fd);
    }
    if (_pull_socket_fd >= 0) {
        close(_pull_socket_fd);
    }
}

void gateway::add_device(lora::eui64 dev_eui, std::shared_ptr<channel<gw::DownlinkFrame>> channel) {
    _devices.emplace(dev_eui, std::move(channel));
}

void gateway::send_uplink_frame(gw::UplinkFrame frame) {
    std::lock_guard<std::mutex> lock{_push_mutex};
    if (_stopped) {
        return;
    }

    // Generate PUSH_DATA packet
    auto* rx_info = frame.mutable_rx_info();
    *rx_info = _uplink_rx_info;
    auto packet = generate_push_data_packet(frame);

    // Send PUSH_DATA packet
    sendto(_push_socket_fd, packet.data(), packet.size(), MSG_CONFIRM, (const sockaddr*)&_server, sizeof(_server));
    spdlog::trace("GTW {}: Send PUSH_DATA packet", _gateway_id.string());

    // Handle PUSH_ACK packet
    byte resp[1024];
    auto resp_len = timeout_recvfrom(_push_socket_fd, resp, 1024, _server, 4);
    if (_stopped) {
        return;
    }
    if (resp_len > 0) {
        resp[resp_len] = '\0';
        if (is_push_ack(resp, resp_len, packet)) {
            spdlog::trace("GTW {}: Receive PUSH_ACK packet", _gateway_id.string());
        } else {
            spdlog::error("GTW {}: Receive invalid packet", _gateway_id.string());
        }
    } else {
        spdlog::error("GTW {}: Not receive any packet", _gateway_id.string());
    }
}

void gateway::keep_alive() {
    while (!_stopped) {
        // Generate PULL_DATA packet
        auto packet = generate_pull_data_packet();

        // Send PULL_DATA packet
        sendto(_pull_socket_fd, packet.data(), packet.size(), MSG_CONFIRM, (const sockaddr*)&_server, sizeof(_server));
        spdlog::trace("GTW {}: Send PULL_DATA packet", _gateway_id.string());

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
        byte resp[1024];
        auto resp_len = timeout_recvfrom(_pull_socket_fd, resp, 1024, _server, 16);
        if (_stopped) {
            return;
        }
        if (resp_len > 0) {
            resp[resp_len] = '\0';
            if (is_pull_resp(resp, resp_len)) {
                spdlog::trace("GTW {}: Receive PULL_RESP packet", _gateway_id.string());

                // Handle PULL_RESP packet
                gw::DownlinkFrame frame;
                std::string res(resp + 4, resp_len - 4);
                std::string err;
                auto json = json11::Json::parse(res, err);
                frame.set_phy_payload(json["txpk"]["data"].string_value());
                for (auto& device : _devices) {
                    device.second->put(frame);
                }

                // Send TX_ACK packet
                gw::DownlinkTXAck ack;
                uint32_t token;
                std::array<byte, sizeof(token)> bytes{resp[1], resp[2], 0x00, 0x00};
                std::memcpy(&token, bytes.data(), sizeof(token));
                ack.set_token(token);
                send_downlink_tx_ack(std::move(ack));
            } else if (is_pull_ack(resp, resp_len)) {
                spdlog::trace("GTW {}: Receive PULL_ACK packet", _gateway_id.string());
                _connected = true;
            } else {
                spdlog::error("GTW {}: Receive invalid packet", _gateway_id.string());
            }
        } else if (resp_len < 0) {
            spdlog::error("GTW {}: Not receive any packet", _gateway_id.string());
            _connected = false;
        }

        // Wait for next cycle
        std::this_thread::sleep_for(50ms);
    }
}

void gateway::send_downlink_tx_ack(gw::DownlinkTXAck ack) {
    // Generate TX_ACK packet
    auto packet = generate_tx_ack_packet(ack);

    // Send TX_ACK packet
    sendto(_pull_socket_fd, packet.data(), packet.size(), MSG_CONFIRM, (const sockaddr*)&_server, sizeof(_server));
    spdlog::trace("GTW {}: Send TX_ACK packet", _gateway_id.string());
}

std::vector<byte> gateway::generate_push_data_packet(const gw::UplinkFrame& frame) {
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
    data.add("chan", frame.rx_info().channel());
    data.add("rfch", frame.rx_info().rf_chain());
    data.add("freq", frame.tx_info().frequency() / 1000000.);
    data.add("stat", frame.rx_info().crc_status() == gw::CRC_OK ? 1 : 0);
    if (frame.tx_info().modulation() == common::LORA) {
        data.add("modu", "LORA");
        if (frame.tx_info().has_lora_modulation_info()) {
            std::basic_stringstream<byte> datr;
            datr << "SF" << frame.tx_info().lora_modulation_info().spreading_factor();
            datr << "BW" << frame.tx_info().lora_modulation_info().bandwidth();
            data.add("datr", datr.str());
            data.add("codr", frame.tx_info().lora_modulation_info().code_rate());
        }
    } else {
        data.add("modu", "FSK");
        if (frame.tx_info().has_fsk_modulation_info()) {
            data.add("datr", frame.tx_info().fsk_modulation_info().datarate());
        }
    }
    data.add("rssi", frame.rx_info().rssi());
    data.add("lsnr", frame.rx_info().lora_snr());
    data.add("size", frame.phy_payload().size());
    data.add("data", base64_encode(frame.phy_payload()));
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

std::vector<byte> gateway::generate_tx_ack_packet(const gw::DownlinkTXAck& ack) {
    std::vector<byte> packet;

    // Set protocol version
    packet.push_back(0x02);

    // Set random token
    auto token = ack.token();
    std::array<byte, sizeof(token)> bytes{};
    std::memcpy(bytes.data(), &token, sizeof(token));
    packet.push_back(bytes[0]);
    packet.push_back(bytes[1]);

    // Set TX_ACK identifier
    packet.push_back(0x05);

    // Set gateway identifier
    for (const auto& byte : _gateway_id._value) {
        packet.push_back(byte);
    }

    // Return
    return packet;
}

bool gateway::is_push_ack(const byte* resp, size_t resp_len, const std::vector<byte>& packet) {
    if (resp_len != 4) {
        return false;
    }
    if (resp[0] != 0x02 || resp[1] != packet[1] || resp[2] != packet[2] || resp[3] != 0x01) {
        return false;
    }
    return true;
}

bool gateway::is_pull_ack(const byte* resp, size_t resp_len) {
    if (resp_len != 4) {
        return false;
    }
    // TODO: Compare PULL_DATA and PULL_ACK token
    if (resp[0] != 0x02 || resp[3] != 0x04) {
        return false;
    }
    return true;
}

bool gateway::is_pull_resp(const byte* resp, size_t resp_len) {
    if (resp_len <= 4) {
        return false;
    }
    if (resp[0] != 0x02 || resp[3] != 0x03) {
        return false;
    }
    return true;
}

void rxpk::add(std::string field_name, const std::string& field_value) {
    std::basic_stringstream<byte> ss;
    ss << R"(")" << field_value << R"(")";
    _fields.emplace_back(std::move(field_name), ss.str());
}

void rxpk::add(std::string field_name, const char* field_value) {
    std::basic_stringstream<byte> ss;
    ss << R"(")" << field_value << R"(")";
    _fields.emplace_back(std::move(field_name), ss.str());
}

std::string rxpk::string() const {
    std::basic_stringstream<byte> ss;
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
