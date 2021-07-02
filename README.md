# ChirpStack Simulator

**ChirpStack Simulator** is a C++ variant of the open-source simulator [ChirpStack Simulator](https://github.com/brocaar/chirpstack-simulator) developed by [brocaar](https://github.com/brocaar) and co. for the ChirpStack open-source LoRaWAN® Network-Server stack. By simulating real LoRa gateways and LoRa devices, **ChirpStack Simulator** allows ChirpStack users to check their setups, to debug failures and to have a thorough understanding of how ChirpStack really works in action.

## Building

### How to build
As for many C++ projects, **ChirpStack Simulator** can be built using the CMake build system. The minimum required version of CMake is 3.15. To build **ChirpStack Simulator**, use the following commands:

```bash
$ bash scripts/pre-build.sh
$ bash scripts/build.sh
```

The binary file will be located at `bin/chirpstack_simulator`.

### Note

* **About cryptopp**: Running the bash script `scripts/pre-build.sh` before building is required because **cryptopp** is not fully supporting CMake as of now.
* **About gRPC**: Installing and configuring **gRPC** can be fairly complicated. See also: [grpc-cpp](https://github.com/chungphb/grpc-cpp), [chirpstack-client](https://github.com/chungphb/chirpstack-client).

## Configuration

### How to generate a new configuration file

Having a valid TOML configuration file is mandatory to run **ChirpStack Simulator**. This configuration file could be generated using the `--generate-config-file` command-line option:

```bash
./bin/chirpstack_simulator --generate-config-file [OUTPUT_FILE]
```

where `OUTPUT_FILE` is the path to the output file.

For example: `./bin/chirpstack_simulator --generate-config-file ./bin/chirpstack_simulator.toml`

### How to configure

| Name                                    | Type   | Meaning                                                      | Requirements                     |
| --------------------------------------- | :----- | ------------------------------------------------------------ | -------------------------------- |
| `general.log_level`                     | int    | Log level (trace = 6, debug = 5, info = 4, warn = 3, error = 2, critical = 1, off = 0). | >= 0, <= 6                       |
| `server.network_server`                 | string | ChirpStack Gateway Bridge's UDP listener.                    | host:port                        |
| `server.application_server`             | string | ChirpStack Application Server's external API server.         | host:port                        |
| `simulator.jwt_token`                   | string | The JWT token to connect to the ChirpStack Application Server API. | Not empty                        |
| `simulator.service_profile_id`          | string | The service-profile created for simulating purpose.          | Not empty                        |
| `simulator.duration`                    | int    | The time running the simulation (s).                         | > 0                              |
| `simulator.activation_time`             | int    | The time taken to activate the devices (s).                  | > 0, < `simulator.duration`      |
| `simulator.device.count`                | int    | Number of devices.                                           | > 0, <= 1000                     |
| `simulator.device.uplink_interval`      | int    | The time between two consecutive uplink transmissions (s).   | > 0, < `simulator.duration`      |
| `simulator.device.f_port`               | int    | FPort.                                                       | > 0                              |
| `simulator.device.payload`              | string | Uplink payload.                                              | Not empty                        |
| `simulator.device.frequency`            | int    | Frequency (Hz).                                              | > 0                              |
| `simulator.device.bandwidth`            | int    | Bandwidth (Hz).                                              | > 0                              |
| `simulator.device.spreading_factor`     | int    | Spreading-factor.                                            | > 0                              |
| `simulator.gateway.min_count`           | int    | Minimum number of receiving gateways.                        | > 0                              |
| `simulator.gateway.max_count`           | int    | Maximum number of receiving gateways.                        | >= `simulator.gateway.min_count` |
| `simulator.client.enable_downlink_test` | bool   | Enable simulating downlink tranmissions.                     | NONE                             |
| `simulator.client.downlink_interval`    | int    | The time between two consecutive downlink tranmissions (s).  | > 0, < `simulator.duration`      |
| `simulator.client.payload`              | string | Downlink payload.                                            | Not empty                        |

### Example

```toml
[general]
log_level = 4

[server]
network_server = "localhost:1700"
application_server = "localhost:8080"

[simulator]
jwt_token = ""
service_profile_id = ""
duration = 60
activation_time = 5

[simulator.device]
count = 100
uplink_interval = 10
f_port = 10
payload = "uplink_packet_1234"
frequency = 868100000
bandwidth = 125
spreading_factor = 12

[simulator.gateway]
min_count = 3
max_count = 5

[simulator.client]
enable_downlink_test = false
downlink_interval = 15
payload = "downlink_packet_1234"
```

### Note
* **How to set JWT token:** JWT token can be acquired in the `Token` field after creating a new API key using **ChirpStack Application Server** web-interface.
* **How to set service-profile ID:** Service-profile ID can be acquired in the URL after creating a new service-profile using **ChirpStack Application Server** web-interface.

## Running

### How to run

To start **ChirpStack Simulator**, use the following command:

```bash
./bin/chirpstack_simulator --config [CONFIG_FILE]
```

where `CONFIG_FILE` is the path to the configuration file.

For example: `./bin/chirpstack_simulator --config ./bin/chirpstack_simulator.toml`

### What happens during the simulation

After parsing and validating the configuration parameters, **ChirpStack Simulator** uses the configured `simulator.jwt_token` and `simulator.service_profile_id` to create gateways and devices. Then, the simulation starts.

The simulation will be gracefully stopped after a `simulator.duration` interval. During this interval, devices will periodically send uplink frames (and handle downlink frames, if any) to the configured `server.network_server` through the gateways assigned to them.

If downlink test is enabled, downlink frames will also be sent periodically to the configured `server.application_server` while waiting to stop.

### How to stop

**ChirpStack Simulator** will be terminated by any of the following conditions:

* After a `simulator.duration` interval.
* When an exception is thrown.
* When receiving a SIGINT signal.

When the simulation stops, the gateways and devices created earlier will be deleted.

## License
This project is licensed under the terms of the MIT license.
