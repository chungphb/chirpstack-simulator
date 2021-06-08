//
// Created by chungphb on 21/5/21.
//

#include <chirpstack_simulator/core/simulator.h>

int main(int argc, char** argv) {
    using namespace chirpstack_simulator;
    simulator sim;
    sim.init();
    sim.run();
    return 0;
}