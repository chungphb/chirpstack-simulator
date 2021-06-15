//
// Created by chungphb on 6/6/21.
//

#include <chirpstack_simulator/core/config.h>
#include <chirpstack_simulator/util/helper.h>
#include <toml/toml.h>

namespace chirpstack_simulator {

void config::init(const std::string& config_file) {
    spdlog::info("[CONFIG]");

    // Parse config
    std::ifstream ifs(config_file);
    toml::ParseResult res = toml::parse(ifs);
    if (!res.valid()) {
        throw std::runtime_error("Simulator: Invalid config file");
    }
    const toml::Value& config = res.value;

    // Initialize network server
    const toml::Value* val = config.find("server.network_server");
    if (val && val->is<std::string>()) {
        _network_server._host = get_host(val->as<std::string>());
        _network_server._port = get_port(val->as<std::string>());
    }

    // Initialize application server
    val = config.find("server.application_server");
    if (val && val->is<std::string>()) {
        _application_server._host = get_host(val->as<std::string>());
        _application_server._port = get_port(val->as<std::string>());
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
        _jwt_token = val->as<std::string>();
        if (_jwt_token.empty()) {
            throw std::invalid_argument("Simulator: Invalid JWT token");
        }
    }
    val = config.find("simulator.service_profile_id");
    if (val && val->is<std::string>()) {
        _service_profile_id = val->as<std::string>();
        if (_service_profile_id.empty()) {
            throw std::invalid_argument("Simulator: Invalid service profile ID");
        }
    }
    val = config.find("simulator.duration");
    if (val && val->is<int>()) {
        _duration = val->as<int>();
    }
    val = config.find("simulator.activation_time");
    if (val && val->is<int>()) {
        _activation_time = val->as<int>();
    }

    // Initialize device configs
    val = config.find("simulator.device.count");
    if (val && val->is<int>()) {
        _dev_count = val->as<int>();
    }
    val = config.find("simulator.device.uplink_interval");
    if (val && val->is<int>()) {
        _uplink_interval = val->as<int>();
    }
    val = config.find("simulator.device.f_port");
    if (val && val->is<int>()) {
        _f_port = val->as<int>();
    }
    val = config.find("simulator.device.payload");
    if (val && val->is<std::string>()) {
        _uplink_payload = val->as<std::string>();
    }
    val = config.find("simulator.device.frequency");
    if (val && val->is<int>()) {
        _freq = val->as<int>();
    }
    val = config.find("simulator.device.bandwidth");
    if (val && val->is<int>()) {
        _bandwidth = val->as<int>();
    }
    val = config.find("simulator.device.spreading_factor");
    if (val && val->is<int>()) {
        _s_factor = val->as<int>();
    }

    // Initialize gateway configs
    val = config.find("simulator.gateway.min_count");
    if (val && val->is<int>()) {
        _gw_min_count = val->as<int>();
    }
    val = config.find("simulator.gateway.max_count");
    if (val && val->is<int>()) {
        _gw_max_count = val->as<int>();
    }

    // Initialize client configs
    val = config.find("simulator.client.enable_downlink_test");
    if (val && val->is<bool>()) {
        _enable_downlink_test = val->as<bool>();
    }
    if (_enable_downlink_test) {
        val = config.find("simulator.client.downlink_interval");
        if (val && val->is<int>()) {
            _downlink_interval = val->as<int>();
        }
        val = config.find("simulator.client.payload");
        if (val && val->is<std::string>()) {
            _downlink_payload = val->as<std::string>();
        }
    }

    // Log config
    spdlog::debug("{:<20}:{:>20}", "Network server", to_string(_network_server));
    spdlog::debug("{:<20}:{:>20}", "Application server", to_string(_application_server));
    spdlog::debug("{:<20}:{:>20}", "Device count", _dev_count);
    spdlog::debug("{:<20}:{:>20}", "Gateway count", _gw_max_count);
    spdlog::debug("{:<20}:{:>20}", "Downlink test", _enable_downlink_test ? "Enabled" : "Disabled");
}

}