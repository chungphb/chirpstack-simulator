//
// Created by chungphb on 21/5/21.
//

#include <chirpstack_simulator/core/simulator.h>
#include <argagg/argagg.hpp>

constexpr char version[] = "1.0.";
constexpr char default_config_file[] = "chirpstack_simulator.toml";

int main(int argc, char** argv) {
    chirpstack_simulator::simulator sim;

    // Handle command-line options
    argagg::parser parser{{
        {"version", {"-v", "--version"}, "Show version info and exit.", 0},
        {"help", {"--help"}, "Show this text and exit.", 0},
        {"generate-config-file", {"--generate-config-file"}, "Generate a new configuration file and exit.", 1},
        {"config", {"-c", "--config"}, "Specify configuration file.", 1}
    }};
    argagg::parser_results options;
    try {
        options = parser.parse(argc, argv);
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << '\n';
        return EXIT_FAILURE;
    }
    if (options["help"]) {
        std::cerr << "Usage: chirpstack_simulator [options] ARGS..." << '\n';
        std::cerr << parser << '\n';
        return EXIT_SUCCESS;
    }
    if (options["version"]) {
        std::cerr << "Chirpstack Simulator " << version << '\n';
        return EXIT_SUCCESS;
    }

    try {
        // Generate a new configuration file
        if (options["generate-config-file"]) {
            sim.generate_config_file(options["generate-config-file"].as<std::string>());
            return EXIT_SUCCESS;
        }

        // Set configuration file
        auto config_file = options["config"].as<std::string>(default_config_file);
        sim.set_config_file(config_file);

        // Initialize simulator
        sim.init();

        // Run simulator
        sim.run();
    } catch (const std::exception& ex) {
        // Stop simulator
        sim.stop();

        // Exit
        std::cerr << ex.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}