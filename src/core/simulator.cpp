//
// Created by chungphb on 25/5/21.
//

#include <chirpstack_simulator/core/simulator.h>
#include <chirpstack_simulator/util/helper.h>
#include <unistd.h>
#include <algorithm>
#include <iostream>

namespace chirpstack_simulator {

void simulator::init() {
    // Parse config
    _config.init("chirpstack_simulator.toml");

    // Initialize gateway list
    for (int i = 0; i < _config._gw_max_count; ++i) {
        auto gw = std::make_shared<gateway>();

        // Set gateway ID
        std::array<byte, 8> gw_mac{};
        for (int j = 0; j < 8; ++j) {
            gw_mac[j] = get_random_byte();
        }
        gw->_gateway_id._value = gw_mac;

        // Set network server
        gw->_server.sin_family = AF_INET;
        gw->_server.sin_port = htons(_config._network_server._port);
        if (strcmp(_config._network_server._host.c_str(), default_ns_host) == 0) {
            gw->_server.sin_addr.s_addr = INADDR_ANY;
        } else {
            inet_aton(_config._network_server._host.c_str(), &gw->_server.sin_addr);
        }

        // Set uplink RX info
        gw->_uplink_rx_info.set_rssi(50);
        gw->_uplink_rx_info.set_lora_snr(5.5);
        gw->_uplink_rx_info.set_channel(0);
        gw->_uplink_rx_info.set_rf_chain(0);
        gw->_uplink_rx_info.set_crc_status(gw::CRC_OK);

        // Add gateway
        _gw_list.emplace_back(gw);
    }

    // Initialize device list
    for (int i = 0; i < _config._dev_count; ++i) {
        auto dev = std::make_shared<device>();

        // Set device EUI
        std::array<byte, 8> dev_eui{};
        for (int j = 0; j < 8; ++j) {
            dev_eui[j] = get_random_byte();
        }
        dev->_dev_eui._value = dev_eui;

        // Set app key
        std::array<byte, 16> app_key{};
        for (int j = 0; j < 16; ++j) {
            dev_eui[j] = get_random_byte();
        }
        dev->_app_key._value = app_key;

        // Set uplink interval
        dev->_uplink_interval = _config._uplink_interval;

        // Set OTAA delay
        dev->_otaa_delay = _config._activation_time;

        // Set uplink payload
        dev->_f_port = static_cast<uint8_t>(_config._f_port);
        dev->_confirmed = false;
        std::copy(_config._payload.begin(), _config._payload.end(), std::back_inserter(dev->_payload));

        // Set gateway
        auto gw_count = get_random_number(_config._gw_min_count, _config._gw_max_count);
        std::vector<size_t> gw_list;
        for (int j = 0; j < gw_count; ++j) {
            auto rand_id = get_random_number(0, _config._gw_max_count);
            auto gw_it = std::find_if(gw_list.begin(), gw_list.end(), [rand_id](auto gw_id) {
                return rand_id == gw_id;
            });
            if (gw_it == gw_list.end()) {
                dev->_gateways.push_back(_gw_list[rand_id]);
            } else {
                --j;
            }
        }

        // Set uplink TX info
        dev->_uplink_tx_info.set_frequency(_config._freq);
        dev->_uplink_tx_info.set_modulation(common::LORA);
        gw::LoRaModulationInfo* lora_modulation_info = dev->_uplink_tx_info.mutable_lora_modulation_info();
        lora_modulation_info->set_bandwidth(_config._bandwidth);
        lora_modulation_info->set_spreading_factor(_config._s_factor);
        lora_modulation_info->set_code_rate("3/4");

        // Add device
        _dev_list.emplace_back(std::move(dev));
    }

    // Log config
    spdlog::debug("CONFIG");
    spdlog::debug("{:<25}: {:>20}", "Network server", to_string(_config._network_server));
    spdlog::debug("{:<25}: {:>20}", "Application server", to_string(_config._application_server));
    spdlog::debug("{:<25}: {:>20}", "Device count", _config._dev_count);
    spdlog::debug("{:<25}: {:>20}", "Gateway count", _config._gw_max_count);
}

void simulator::run() {
    // Setup client
    setup_client();

    // Run gateways
    for (const auto& gw : _gw_list) {
        gw->run();
    }

    // Run devices
    for (const auto& dev : _dev_list) {
        dev->run();
    }
}

void simulator::stop() {
    // Stop devices
    for (const auto& dev : _dev_list) {
        dev->stop();
    }

    // Stop gateways
    for (const auto& gw : _gw_list) {
        gw->run();
    }
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
    spdlog::debug("SETUP");
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
        gateway->set_id(gw->_gateway_id.string());
        gateway->set_name(gw->_gateway_id.string());
        gateway->set_description(gw->_gateway_id.string());
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
            spdlog::debug("Setup gateway {}", gw->_gateway_id.string());
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
        device->set_dev_eui(dev->_dev_eui.string());
        device->set_name(dev->_dev_eui.string());
        device->set_description(dev->_dev_eui.string());
        device->set_application_id(info._application_id);
        device->set_device_profile_id(info._device_profile_id);

        // Create device
        auto response = _client->create_device(request);
        if (!response.is_valid()) {
            spdlog::error("Failed to setup device: {}", response.error_code());
        } else {
            spdlog::debug("Setup device {}", dev->_dev_eui.string());
        }
    }
}

void simulator::setup_device_keys(client_info& info) {
    for (const auto& dev : _dev_list) {
        // Prepare request
        api::CreateDeviceKeysRequest request;
        api::DeviceKeys* device_keys = request.mutable_device_keys();
        device_keys->set_dev_eui(dev->_dev_eui.string());
        device_keys->set_nwk_key(get_random_aes128key());

        // Create device keys
        auto response = _client->create_device_keys(request);
        if (!response.is_valid()) {
            spdlog::error("Failed to setup device keys for device: {}", response.error_code());
        } else {
            spdlog::debug("Setup device keys for device {}", dev->_dev_eui.string());
        }
    }
}

}