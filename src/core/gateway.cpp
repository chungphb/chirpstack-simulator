//
// Created by chungphb on 25/5/21.
//

#include <chirpstack_simulator/core/gateway.h>
#include <chirpstack_simulator/util/helper.h>
#include <spdlog/spdlog.h>
#include <sstream>

namespace chirpstack_simulator {

gateway::~gateway() {
    stop();
}

void gateway::run() {
    if ((_socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        spdlog::critical("Failed to create socket file descriptor");
        exit(EXIT_FAILURE);
    }
}

void gateway::stop() const {
    close(_socket_fd);
}

void gateway::send_uplink_frame(gw::UplinkFrame frame) {
    // Generate PUSH_DATA packet
    gw::UplinkRXInfo* rx_info = frame.mutable_rx_info();
    *rx_info = _uplink_rx_info;
    auto packet = generate_push_data_packet(frame);

    // Send packet
    sendto(_socket_fd, packet.data(), packet.size(), MSG_CONFIRM, (const sockaddr*)&_server, sizeof(_server));
    spdlog::info("Gateway {}: Send packet", _gateway_id.string());

    // Receive ACK
    byte ack[1024];
    auto ack_len = timeout_recvfrom(_socket_fd, ack, 1024, _server, 12);
    if (ack_len < 0) {
        spdlog::info("Gateway {}: Not receive ACK", _gateway_id.string());
    } else {
        ack[ack_len] = '\0';
        spdlog::info("Gateway {}: Receive {} ACK", _gateway_id.string(), (ack_len == 4 ? "valid" : "invalid"));
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
    data.add("stat", payload.rx_info().crc_status());
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
