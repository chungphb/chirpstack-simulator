//
// Created by chungphb on 21/5/21.
//

#include <chirpstack_simulator/core/simulator.h>

int main() {
    using namespace chirpstack_simulator;
    simulator sim;
    sim.init();
    sim.start();
    sim.stop();
    return 0;
}