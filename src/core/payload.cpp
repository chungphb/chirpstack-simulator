//
// Created by chungphb on 25/5/21.
//

#include <chirpstack_simulator/core/payload.h>

namespace chirpstack_simulator {

byte_array payload::as_byte_array() const {
    return _data;
}

}
