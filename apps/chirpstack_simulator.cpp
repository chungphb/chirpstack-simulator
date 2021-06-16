//
// Created by chungphb on 21/5/21.
//

#include <chirpstack_simulator/core/simulator.h>
#include <argagg/argagg.hpp>
#include <csignal>
#include <csetjmp>
#include <atomic>

constexpr char version[] = "1.0.";
constexpr char default_config_file[] = "chirpstack_simulator.toml";

std::atomic<bool> quit{false};
jmp_buf buf;

void signal_handler(int signal) {
    if (quit.load()) {
        exit(EXIT_SUCCESS);
    } else {
        quit.store(true);
        longjmp(buf, 1);
    }
}

int main(int argc, char** argv) {
    chirpstack_simulator::simulator sim;

    // Handle SIGINT signal
    signal(SIGINT, signal_handler);
    if (setjmp(buf) == 1) {
        sim.stop();
        return EXIT_SUCCESS;
    }

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

    // Start simulator
    try {
        if (options["generate-config-file"]) {
            sim.generate_config_file(options["generate-config-file"].as<std::string>());
            return EXIT_SUCCESS;
        }
        auto config_file = options["config"].as<std::string>(default_config_file);
        sim.set_config_file(config_file);
        sim.init();
        sim.run();
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << '\n';
        sim.stop();
        return EXIT_FAILURE;
    } catch (...) {
        sim.stop();
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}