//
// Created by chungphb on 25/5/21.
//

#include <chirpstack_simulator/core/simulator.h>
#include <chirpstack_simulator/util/helper.h>
#include <toml/toml.h>
#include <spdlog/spdlog.h>
#include <unistd.h>
#include <iostream>

namespace chirpstack_simulator {

void simulator::init() {
    // Create socket file descriptor
    if ((_socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        spdlog::critical("Failed to create socket file descriptor");
        exit(EXIT_FAILURE);
    }

    // Parse config
    std::ifstream config_file("chirpstack_simulator.toml");
    toml::ParseResult res = toml::parse(config_file);
    if (!res.valid()) {
        throw std::runtime_error("Invalid config file");
    }
    const toml::Value& config = res.value;

    // Initialize network server
    const toml::Value* val = config.find("server.network_server");
    if (val && val->is<std::string>()) {
        _config._network_server._host = get_host(val->as<std::string>());
        _config._network_server._port = get_port(val->as<std::string>());
    }
    _server_addr.sin_family = AF_INET;
    _server_addr.sin_port = htons(_config._network_server._port);
    if (strcmp(_config._network_server._host.c_str(), DEFAULT_NS_HOST) == 0) {
        _server_addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        inet_aton(_config._network_server._host.c_str(), &_server_addr.sin_addr);
    }

    // Initialize application server
    val = config.find("server.application_server");
    if (val && val->is<std::string>()) {
        _config._application_server._host = get_host(val->as<std::string>());
        _config._application_server._port = get_port(val->as<std::string>());
    }

    // Initialize log level
    spdlog::set_level(spdlog::level::info);
    val = config.find("general.log_level");
    if (val && val->is<int>()) {
        auto log_level = val->as<int>();
        if (log_level == 0) {
            spdlog::set_level(spdlog::level::off);
        } else if (log_level == 1) {
            spdlog::set_level(spdlog::level::critical);
        } else if (log_level == 2) {
            spdlog::set_level(spdlog::level::err);
        } else if (log_level == 3) {
            spdlog::set_level(spdlog::level::warn);
        } else if (log_level == 4) {
            spdlog::set_level(spdlog::level::info);
        } else if (log_level == 5) {
            spdlog::set_level(spdlog::level::debug);
        } else if (log_level == 6) {
            spdlog::set_level(spdlog::level::trace);
        }
    }

    // Initialize JWT token and service-profile ID
    val = config.find("simulator.jwt_token");
    if (val && val->is<std::string>()) {
        _config._jwt_token = val->as<std::string>();
        // TODO: Validation
        if (_config._jwt_token.empty()) {
            throw std::invalid_argument("Invalid JWT token");
        }
    }
    val = config.find("simulator.service_profile_id");
    if (val && val->is<std::string>()) {
        _config._service_profile_id = val->as<std::string>();
        // TODO: Validation
        if (_config._service_profile_id.empty()) {
            throw std::invalid_argument("Invalid service-profile ID");
        }
    }

    // Initialize device configs
    val = config.find("simulator.device.count");
    if (val && val->is<int>()) {
        _config._dev_count = val->as<int>();
    }
    val = config.find("simulator.device.f_port");
    if (val && val->is<int>()) {
        _config._f_port = val->as<int>();
    }
    val = config.find("simulator.device.payload");
    if (val && val->is<std::string>()) {
        _config._payload = val->as<std::string>();
    }
    val = config.find("simulator.device.frequency");
    if (val && val->is<int>()) {
        _config._freq = val->as<int>();
    }
    val = config.find("simulator.device.spreading_factor");
    if (val && val->is<int>()) {
        _config._s_factor = val->as<int>();
    }

    // Initialize gateway configs
    val = config.find("simulator.gateway.min_count");
    if (val && val->is<int>()) {
        _config._gw_min_count = val->as<int>();
    }
    val = config.find("simulator.gateway.max_count");
    if (val && val->is<int>()) {
        _config._gw_max_count = val->as<int>();
    }

    // Initialize device list
    for (int i = 0; i < _config._dev_count; ++i) {
        std::vector<byte> dev_addr;
        dev_addr.reserve(DEV_ADDR_LEN);
        for (int j = 0; j < DEV_ADDR_LEN; j++) {
            dev_addr.push_back(get_random_byte());
        }
        _dev_list.emplace_back(std::move(dev_addr), _config);
    }

    // Initialize gateway list
    for (int i = 0; i < _config._gw_max_count; ++i) {
        std::vector<byte> gw_mac;
        gw_mac.reserve(GW_MAC_LEN);
        for (int j = 0; j < GW_MAC_LEN; j++) {
            gw_mac.push_back(get_random_byte());
        }
        _gw_list.emplace_back(std::move(gw_mac), _config);
    }

    // Log config
    spdlog::debug("[Config] {:25}: {}", "Network server", to_string(_config._network_server));
    spdlog::debug("[Config] {:25}: {}", "Application server", to_string(_config._application_server));
    spdlog::debug("[Config] {:25}: {}", "JWT token", _config._jwt_token);
    spdlog::debug("[Config] {:25}: {}", "Service-profile ID", _config._service_profile_id);
    spdlog::debug("[Config] {:25}: {}", "Device count", _config._dev_count);
    spdlog::debug("[Config] {:25}: {}", "Gateway count", _config._gw_max_count);
}

void simulator::start() {
    for (auto& dev : _dev_list) {
        auto gw_id = get_random(0, _gw_list.size() - 1);
        dev.send_payload(_gw_list[gw_id], _socket_fd, _server_addr);
    }
}

void simulator::stop() {
    close(_socket_fd);
}

}