//
// Created by chungphb on 21/5/21.
//

#include <supg/core/simulator.h>

int main() {
    using namespace supg;
    simulator sim;
    sim.init();
    sim.start();
    sim.stop();
    return 0;
}