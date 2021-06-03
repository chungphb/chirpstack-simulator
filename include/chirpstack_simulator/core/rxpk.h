//
// Created by chungphb on 25/5/21.
//

#pragma once

#include <chirpstack_simulator/core/payload.h>
#include <string>

namespace chirpstack_simulator {

struct rxpk {
public:
    byte_array as_byte_array() const;
    template <typename field_value_t>
    void add(std::string field_name, field_value_t field_value) {
        _fields.emplace_back(std::move(field_name), std::to_string(field_value));
    }
    void add(std::string field_name, const std::string& field_value);
    void add(std::string field_name, const char* field_value);
    rxpk clone();

private:
    std::vector<std::pair<std::string, std::string>> _fields;
};

}