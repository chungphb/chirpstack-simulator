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
        for (int j = 0; j < DEV_ADDR_LEN; ++j) {
            dev_addr.push_back(get_random_byte(0, 127));
        }
        _dev_list.emplace_back(std::move(dev_addr), _config);
    }

    // Initialize gateway list
    for (int i = 0; i < _config._gw_max_count; ++i) {
        std::vector<byte> gw_mac;
        gw_mac.reserve(GW_MAC_LEN);
        for (int j = 0; j < GW_MAC_LEN; ++j) {
            gw_mac.push_back(get_random_byte(0, 127));
        }
        _gw_list.emplace_back(std::move(gw_mac), _config);
    }

    // Log config
    spdlog::debug("<Config> {:25}: {}", "Network server", to_string(_config._network_server));
    spdlog::debug("<Config> {:25}: {}", "Application server", to_string(_config._application_server));
    spdlog::debug("<Config> {:25}: {}", "Device count", _config._dev_count);
    spdlog::debug("<Config> {:25}: {}", "Gateway count", _config._gw_max_count);

    // Setup client
    setup_client();
}

void simulator::start() {
    for (auto& dev : _dev_list) {
        auto gw_id = get_random_number<size_t>(0, _gw_list.size() - 1);
        dev.send_payload(_gw_list[gw_id], _socket_fd, _server_addr);
    }
}

void simulator::stop() {
    close(_socket_fd);
}

void simulator::setup_client() {
    // Configure client
    chirpstack_client_config client_config{};
    client_config.jwt_token = _config._jwt_token;
    _client = std::make_unique<chirpstack_client>(to_string(_config._application_server), client_config);

    // Setup client
    client_info info;
    info._device_profile_name = get_random_uuid_v4();
    info._application_name = get_random_uuid_v4();
    setup_service_profile(info);
    setup_gateway(info);
    setup_device_profile(info);
    setup_application(info);
    setup_device(info);
    setup_device_keys(info);
}

void simulator::setup_service_profile(client_info& info) {
    // Prepare request
    api::GetServiceProfileRequest request;
    request.set_id(_config._service_profile_id);

    // Get service profile
    auto response = _client->get_service_profile(request);
    if (!response.is_valid()) {
        spdlog::error("Failed to setup service profile: {}", response.error_code());
    } else {
        info._service_profile = response.get().service_profile();
        spdlog::debug("Setup service profile {}", _config._service_profile_id);
    }
}

void simulator::setup_gateway(client_info& info) {
    for (const auto& gw : _gw_list) {
        // Prepare request
        api::CreateGatewayRequest request;
        api::Gateway* gateway = request.mutable_gateway();
        gateway->set_id(gw.id());
        gateway->set_name(gw.id());
        gateway->set_description(gw.id());
        common::Location* location = gateway->mutable_location();
        location->set_latitude(0);
        location->set_longitude(0);
        location->set_altitude(0);
        location->set_source(common::LocationSource::UNKNOWN);
        location->set_accuracy(0);
        gateway->set_organization_id(info._service_profile.organization_id());
        gateway->set_network_server_id(info._service_profile.network_server_id());
        gateway->set_service_profile_id(info._service_profile.id());

        // Create gateway
        auto response = _client->create_gateway(request);
        if (!response.is_valid()) {
            spdlog::error("Failed to setup gateway: {}", response.error_code());
        } else {
            spdlog::debug("Setup gateway {}", gw.id());
        }
    }

}

void simulator::setup_device_profile(client_info& info) {
    // Prepare request
    api::CreateDeviceProfileRequest request;
    api::DeviceProfile* device_profile = request.mutable_device_profile();
    device_profile->set_name(info._device_profile_name);
    device_profile->set_organization_id(info._service_profile.organization_id());
    device_profile->set_network_server_id(info._service_profile.network_server_id());
    device_profile->set_mac_version("1.0.3");
    device_profile->set_reg_params_revision("B");
    device_profile->set_supports_join(true);

    // Create device profile
    auto response = _client->create_device_profile(request);
    if (!response.is_valid()) {
        spdlog::error("Failed to setup device profile: {}", response.error_code());
    } else {
        info._device_profile_id = response.get().id();
        spdlog::debug("Setup device profile {}", info._device_profile_name);
    }
}

void simulator::setup_application(client_info& info) {
    // Prepare request
    api::CreateApplicationRequest request;
    api::Application* application = request.mutable_application();
    application->set_name(info._application_name);
    application->set_description(info._application_name);
    application->set_organization_id(info._service_profile.organization_id());
    application->set_service_profile_id(info._service_profile.id());

    // Create application
    auto response = _client->create_application(request);
    if (!response.is_valid()) {
        spdlog::error("Failed to setup application: {}", response.error_code());
    } else {
        info._application_id = response.get().id();
        spdlog::debug("Setup application {}", info._application_name);
    }
}

void simulator::setup_device(client_info& info) {
    for (const auto& dev : _dev_list) {
        // Prepare request
        api::CreateDeviceRequest request;
        api::Device* device = request.mutable_device();
        device->set_dev_eui(dev.eui());
        device->set_name(dev.eui());
        device->set_description(dev.eui());
        device->set_application_id(info._application_id);
        device->set_device_profile_id(info._device_profile_id);

        // Create device
        auto response = _client->create_device(request);
        if (!response.is_valid()) {
            spdlog::error("Failed to setup device: {}", response.error_code());
        } else {
            spdlog::debug("Setup device {}", dev.addr());
        }
    }
}

void simulator::setup_device_keys(client_info& info) {
    for (const auto& dev : _dev_list) {
        // Prepare request
        api::CreateDeviceKeysRequest request;
        api::DeviceKeys* device_keys = request.mutable_device_keys();
        device_keys->set_dev_eui(dev.eui());
        device_keys->set_nwk_key(get_random_aes128key());

        // Create device keys
        auto response = _client->create_device_keys(request);
        if (!response.is_valid()) {
            spdlog::error("Failed to setup device keys for device: {}", response.error_code());
        } else {
            spdlog::debug("Setup device keys for device {}", dev.addr());
        }
    }
}

}