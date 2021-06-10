//
// Created by chungphb on 4/6/21.
//

#include <chirpstack_simulator/core/lora/phy_payload.h>
#include <chirpstack_simulator/core/lora/mac_payload.h>
#include <chirpstack_simulator/util/helper.h>
#include <spdlog/spdlog.h>
#include <cmac.h>
#include <aes.h>
#include <modes.h>
#include <filters.h>

namespace chirpstack_simulator {
namespace lora {

std::vector<byte> aes128key::marshal_binary() {
    std::vector<byte> res{_value.rbegin(), _value.rend()};
    return res;
}

void aes128key::unmarshal_binary(const std::vector<byte>& data) {
    if (data.size() != _value.size()) {
        throw std::runtime_error("lora: failed to unmarshal eui64");
    }
    for (int i = 0; i < _value.size(); ++i) {
        _value[_value.size() - (i + 1)] = data[i];
    }
}

std::basic_string<byte> aes128key::string() {
    return to_hex_string(_value.data(), _value.size());
}

bool mic::operator==(const mic& rhs) const {
    for (int i = 0; i < _value.size(); ++i) {
        if (_value[i] != rhs._value[i]) {
            return false;
        }
    }
    return true;
}

bool mic::operator!=(const mic& rhs) const {
    return !operator==(rhs);
}

std::vector<byte> mhdr::marshal_binary() {
    byte res = (static_cast<byte>(_m_type) << 5) | (static_cast<byte>(_major) & 0x03);
    return {res};
}

void mhdr::unmarshal_binary(const std::vector<byte>& data) {
    if (data.size() != 1) {
        throw std::runtime_error("lora: failed to unmarshal mac header");
    }
    _m_type = static_cast<m_type>(data[0] >> 5);
    _major = static_cast<major>(data[0] & 0x03);
}

void phy_payload::set_uplink_data_mic(uplink_data_info& info) {
    _mic = calculate_uplink_data_mic(info);
}

bool phy_payload::validate_uplink_data_mic(uplink_data_info& info) {
    auto mic = calculate_uplink_data_mic(info);
    return _mic == mic;
}

bool phy_payload::validate_uplink_data_micf(aes128key f_nwk_s_int_key) {
    // Calculate mic
    uplink_data_info info{};
    info._m_ver = mac_version::lorawan_1_1;
    info._conf_f_cnt = 0;
    info._tx_ch = 0;
    info._tx_dr = 0;
    info._f_nwk_s_int_key = f_nwk_s_int_key;
    info._s_nwk_s_int_key = f_nwk_s_int_key;
    auto mic = calculate_uplink_data_mic(info);

    // Validate micf
    for (int i = 2; i < _mic._value.size(); ++i) {
        if (_mic._value[i] != mic._value[i]) {
            return false;
        }
    }
    return true;
}

mic phy_payload::calculate_uplink_data_mic(uplink_data_info& info) {
    if (!_mac_payload ) {
        throw std::runtime_error("lora: mac_payload should not be null");
    }
    auto payload = std::dynamic_pointer_cast<mac_payload>(_mac_payload);
    if (!payload) {
        throw std::runtime_error("lora: mac_payload should be of type mac_payload");
    }
    mic mic{};
    std::vector<byte> plain;
    std::vector<byte> bytes;

    // Reset frame count
    info._conf_f_cnt = payload->_fhdr._f_ctrl._ack ? (info._conf_f_cnt % (1 << 16)) : 0;

    // Marshal MAC header
    bytes = _mhdr.marshal_binary();
    plain.insert(plain.end(), bytes.begin(), bytes.end());

    // Marshal MAC payload
    bytes = _mac_payload->marshal_binary();
    plain.insert(plain.end(), bytes.begin(), bytes.end());

    std::vector<byte> bytes_0(16, 0x00);
    std::vector<byte> bytes_1(16, 0x00);
    bytes_0[0] = 0x49;
    bytes_1[0] = 0x49;

    // Marshal device address
    bytes = payload->_fhdr._dev_addr.marshal_binary();
    std::copy(bytes.begin(), bytes.end(), bytes_0.begin() + 6);
    std::copy(bytes.begin(), bytes.end(), bytes_1.begin() + 6);

    // Marshal frame count
    auto f_cnt = payload->_fhdr._f_cnt;
    std::memcpy(bytes_0.data() + 10, &f_cnt, sizeof(f_cnt));
    std::memcpy(bytes_1.data() + 10, &f_cnt, sizeof(f_cnt));

    // Marshal message length
    bytes_0[15] = static_cast<byte>(plain.size());
    bytes_1[15] = static_cast<byte>(plain.size());

    // Marshal remaining fields
    auto conf_f_cnt = static_cast<uint16_t>(info._conf_f_cnt);
    std::memcpy(bytes_1.data() + 1, &conf_f_cnt, sizeof(conf_f_cnt));
    bytes_1[3] = static_cast<byte>(info._tx_dr);
    bytes_1[4] = static_cast<byte>(info._tx_ch);

    std::vector<byte> f_encoded;
    std::vector<byte> s_encoded;

    // Encode second half
    CryptoPP::SecByteBlock key{CryptoPP::AES::DEFAULT_KEYLENGTH};
    for (int i = 0; i < key.size(); ++i) {
        key[i] = (CryptoPP::byte)info._s_nwk_s_int_key._value[i];
    }
    try {
        CryptoPP::CMAC<CryptoPP::AES> cmac{key.data(), key.size()};
        cmac.Update((const CryptoPP::byte*)bytes_1.data(), bytes_1.size());
        cmac.Update((const CryptoPP::byte*)plain.data(), plain.size());
        s_encoded.resize(cmac.DigestSize());
        cmac.Final((CryptoPP::byte*)&s_encoded[0]);
    } catch (const CryptoPP::Exception& ex) {
        spdlog::error("Failed to encrypt: {}",  ex.what());
    }
    if (s_encoded.size() < 4) {
        throw std::runtime_error("lora: the hash returned less than 4 bytes");
    }

    // Encode first half
    for (int i = 0; i < key.size(); ++i) {
        key[i] = (CryptoPP::byte)info._f_nwk_s_int_key._value[i];
    }
    try {
        CryptoPP::CMAC<CryptoPP::AES> cmac{key.data(), key.size()};
        cmac.Update((const CryptoPP::byte*)bytes_0.data(), bytes_0.size());
        cmac.Update((const CryptoPP::byte*)plain.data(), plain.size());
        f_encoded.resize(cmac.DigestSize());
        cmac.Final((CryptoPP::byte*)&f_encoded[0]);
    } catch (const CryptoPP::Exception& ex) {
        spdlog::error("Failed to encrypt: {}",  ex.what());
    }
    if (f_encoded.size() < 2) {
        throw std::runtime_error("lora: the hash returned less than 2 bytes");
    }

    // Generate MIC
    switch (info._m_ver) {
        case mac_version::lorawan_1_0: {
            std::copy_n(f_encoded.begin(), 4, mic._value.begin());
            break;
        }
        case mac_version::lorawan_1_1: {
            std::copy_n(s_encoded.begin(), 2, mic._value.begin());
            std::copy_n(f_encoded.begin(), 2, mic._value.begin() + 2);
            break;
        }
    }
    return mic;
}

void phy_payload::set_downlink_data_mic(downlink_data_info& info) {
    _mic = calculate_downlink_data_mic(info);
}

bool phy_payload::validate_downlink_data_mic(downlink_data_info& info) {
    auto mic = calculate_downlink_data_mic(info);
    return _mic == mic;
}

mic phy_payload::calculate_downlink_data_mic(downlink_data_info& info) {
    return mic{};
}

void phy_payload::set_uplink_join_mic(uplink_join_info& info) {
    _mic = calculate_uplink_join_mic(info);
}

bool phy_payload::validate_uplink_join_mic(uplink_join_info& info) {
    auto mic = calculate_uplink_join_mic(info);
    return _mic == mic;
}

mic phy_payload::calculate_uplink_join_mic(uplink_join_info& info) {
    if (!_mac_payload ) {
        throw std::runtime_error("lora: mac_payload should not be null");
    }
    mic mic{};
    std::vector<byte> encoded;
    std::vector<byte> plain;
    std::vector<byte> bytes;

    // Marshal MAC header
    bytes = _mhdr.marshal_binary();
    plain.insert(plain.end(), bytes.begin(), bytes.end());

    // Marshal MAC payload
    bytes = _mac_payload->marshal_binary();
    plain.insert(plain.end(), bytes.begin(), bytes.end());

    // Encode
    CryptoPP::SecByteBlock key{CryptoPP::AES::DEFAULT_KEYLENGTH};
    for (int i = 0; i < key.size(); ++i) {
        key[i] = (CryptoPP::byte)info._key._value[i];
    }
    try {
        CryptoPP::CMAC<CryptoPP::AES> cmac{key.data(), key.size()};
        cmac.Update((const CryptoPP::byte*)plain.data(), plain.size());
        encoded.resize(cmac.DigestSize());
        cmac.Final((CryptoPP::byte*)&encoded[0]);
    } catch (const CryptoPP::Exception& ex) {
        spdlog::error("Failed to encrypt: {}",  ex.what());
    }
    if (encoded.size() < 4) {
        throw std::runtime_error("lora: the hash returned less than 4 bytes");
    }

    // Generate MIC
    std::copy_n(encoded.begin(), 4, mic._value.begin());
    return mic;
}

void phy_payload::set_downlink_join_mic(downlink_join_info& info) {
    _mic = calculate_downlink_join_mic(info);
}

bool phy_payload::validate_downlink_join_mic(downlink_join_info& info) {
    auto mic = calculate_downlink_join_mic(info);
    return _mic == mic;
}

mic phy_payload::calculate_downlink_join_mic(downlink_join_info& info) {
    if (!_mac_payload ) {
        throw std::runtime_error("lora: mac_payload should not be null");
    }
    auto payload = std::dynamic_pointer_cast<join_accept_payload>(_mac_payload);
    if (!payload) {
        throw std::runtime_error("lora: mac_payload should be of type join_accept_payload");
    }
    mic mic{};
    std::vector<byte> encoded;
    std::vector<byte> plain;
    std::vector<byte> bytes;

    // Marshal input
    if (payload->_dl_settings._opt_neg) {
        // Marshal join type
        plain.push_back(static_cast<byte>(info._join_req_type));

        // Marshal join EUI
        bytes = info._join_eui.marshal_binary();
        plain.insert(plain.end(), bytes.begin(), bytes.end());

        // Marshal device nonce
        bytes = info._dev_nonce.marshal_binary();
        plain.insert(plain.end(), bytes.begin(), bytes.end());
    }

    // Marshal MAC header
    bytes = _mhdr.marshal_binary();
    plain.insert(plain.end(), bytes.begin(), bytes.end());

    // Marshal MAC payload
    bytes = _mac_payload->marshal_binary();
    plain.insert(plain.end(), bytes.begin(), bytes.end());

    // Encode
    CryptoPP::SecByteBlock key{CryptoPP::AES::DEFAULT_KEYLENGTH};
    for (int i = 0; i < key.size(); ++i) {
        key[i] = (CryptoPP::byte)info._key._value[i];
    }
    try {
        CryptoPP::CMAC<CryptoPP::AES> cmac{key.data(), key.size()};
        cmac.Update((const CryptoPP::byte*)plain.data(), plain.size());
        encoded.resize(cmac.DigestSize());
        cmac.Final((CryptoPP::byte*)&encoded[0]);
    } catch (const CryptoPP::Exception& ex) {
        spdlog::error("Failed to encrypt: {}",  ex.what());
    }
    if (encoded.size() < 4) {
        throw std::runtime_error("lora: the hash returned less than 4 bytes");
    }
    std::copy_n(encoded.begin(), 4, mic._value.begin());
    return mic;
}

void phy_payload::encrypt_join_accept_payload(aes128key key) {

}

void phy_payload::decrypt_join_accept_payload(aes128key key) {
    // Prepare data
    auto payload = std::dynamic_pointer_cast<data_payload>(_mac_payload);
    auto data = payload->_data;
    data.insert(data.end(), _mic._value.begin(), _mic._value.end());
    if (data.size() % 16) {
        throw std::runtime_error("lora: plain text must be a multiple of 16 bytes");
    }

    // Decrypt data
    std::string encrypted;
    CryptoPP::ECB_Mode<CryptoPP::AES>::Encryption encryption;
    encryption.SetKey((const CryptoPP::byte*)key._value.data(), CryptoPP::AES::DEFAULT_KEYLENGTH);
    for (int i = 0; i < data.size() / 16; i++) {
        std::string plain{data.begin() + i * 16, data.begin() + i * 16 + 16};
        std::string cipher;
        CryptoPP::StreamTransformationFilter encryptor{encryption, new CryptoPP::StringSink(cipher)};
        encryptor.Put((const CryptoPP::byte*)plain.data(), plain.size());
        encrypted += cipher;
    }

    // Store data
    _mac_payload = std::make_shared<join_accept_payload>();
    std::copy_n(encrypted.data() + encrypted.size() - 4, 4, _mic._value.data());
    _mac_payload->unmarshal_binary({encrypted.data(), encrypted.data() + encrypted.size() - 4}, is_uplink());
}

void phy_payload::encrypt_f_opts(aes128key nwk_s_enc_key) {

}

void phy_payload::decrypt_f_opts(aes128key nwk_s_enc_key) {

}

void phy_payload::decode_f_opts_to_mac_commands() {

}

std::vector<byte> phy_payload::encrypt_f_opts(f_opts_info& info) {
    return {};
}

void phy_payload::encrypt_frm_payload(aes128key key) {
    auto payload = std::dynamic_pointer_cast<mac_payload>(_mac_payload);
    if (!payload) {
        throw std::runtime_error("lora: mac_payload should be of type mac_payload");
    }
    if (payload->_frm_payload.empty()) {
        return;
    }
    std::vector<byte> data;

    // Prepare data
    data = payload->marshal_payload();

    // Encrypt data
    frm_payload_info info;
    info._key = key;
    info._is_uplink = is_uplink();
    info._dev_addr = payload->_fhdr._dev_addr;
    info._f_cnt = payload->_fhdr._f_cnt;
    info._data = std::move(data);
    data = encrypt_frm_payload(info);

    // Store data
    data_payload frm_payload;
    frm_payload._data = std::move(data);
    payload->_frm_payload.clear();
    payload->_frm_payload.push_back(std::make_unique<data_payload>(std::move(frm_payload)));
}

void phy_payload::decrypt_frm_payload(aes128key key) {

}

void phy_payload::decode_frm_payload_to_mac_commands() {

}

std::vector<byte> phy_payload::encrypt_frm_payload(frm_payload_info& info) {
    // Add padding
    auto data_len = info._data.size();
    if (data_len % 16) {
        std::vector<byte> padding(16 - data_len % 16, 0x00);
        info._data.insert(info._data.end(), padding.begin(), padding.end());
    }

    // Prepare data
    std::string plain(16, 0x00);
    plain[0] = 0x01;
    if (!info._is_uplink) {
        plain[5] = 0x01;
    }
    auto bytes = info._dev_addr.marshal_binary();
    std::copy(bytes.begin(), bytes.end(), plain.begin() + 6);
    std::memcpy(&plain[10], &info._f_cnt, sizeof(info._f_cnt));

    // Encrypt data
    CryptoPP::ECB_Mode<CryptoPP::AES>::Encryption encryption;
    encryption.SetKey((const CryptoPP::byte*)info._key._value.data(), CryptoPP::AES::DEFAULT_KEYLENGTH);
    for (int i = 0; i < info._data.size() / 16; i++) {
        plain[15] = static_cast<byte>(i + 1);
        std::string cipher;
        CryptoPP::StreamTransformationFilter encryptor{encryption, new CryptoPP::StringSink(cipher)};
        encryptor.Put((const CryptoPP::byte*)plain.data(), plain.size());
        for (int j = 0; j < cipher.size(); ++j) {
            info._data[i * 16 + j] ^= cipher[j];
        }
    }

    // Return data
    return {info._data.begin(), info._data.begin() + data_len};
}

std::vector<byte> phy_payload::marshal_binary() {
    if (!_mac_payload ) {
        throw std::runtime_error("lora: mac_payload should not be null");
    }
    std::vector<byte> res;
    std::vector<byte> bytes;

    // Marshal MAC header
    bytes = _mhdr.marshal_binary();
    res.insert(res.end(), bytes.begin(), bytes.end());

    // Marshal MAC payload
    bytes = _mac_payload->marshal_binary();
    res.insert(res.end(), bytes.begin(), bytes.end());

    // Marshal MIC
    res.insert(res.end(), _mic._value.begin(), _mic._value.end());

    // Return
    return res;
}

void phy_payload::unmarshal_binary(const std::vector<byte>& data) {
    if (data.size() < 5) {
        throw std::runtime_error("lora: failed to unmarshal physical payload");
    }
    std::vector<byte> bytes;

    // Unmarshal MAC header
    bytes = {data.begin(), data.begin() + 1};
    _mhdr.unmarshal_binary(bytes);

    // Unmarshal MAC payload
    bytes = {data.begin() + 1, data.begin() + data.size() - 4};
    switch (_mhdr._m_type) {
        case m_type::join_request: {
            _mac_payload = std::make_shared<join_request_payload>();
            break;
        }
        case m_type::join_accept: {
            _mac_payload = std::make_shared<data_payload>();
            break;
        }
        case m_type::rejoin_request: {
            switch (data[1]) {
                case 0:
                case 2: {
                    _mac_payload = std::make_shared<rejoin_request_type_02_payload>();
                    break;
                }
                case 1: {
                    _mac_payload = std::make_shared<rejoin_request_type_1_payload>();
                    break;
                }
                default: {
                    throw std::runtime_error("lora: invalid rejoin type");
                }
            }
            break;
        }
        case m_type::proprietary: {
            _mac_payload = std::make_shared<data_payload>();
            break;
        }
        default: {
            _mac_payload = std::make_shared<mac_payload>();
            break;
        }
    }
    _mac_payload->unmarshal_binary(bytes, is_uplink());

    // Unmarshal MIC
    for (int i = 0; i < 4; ++i) {
        _mic._value[i] = data[data.size() - 4 + i];
    }
}

bool phy_payload::is_uplink() {
    switch (_mhdr._m_type) {
        case m_type::join_request:
        case m_type::unconfirmed_data_up:
        case m_type::confirmed_data_up:
        case m_type::rejoin_request: {
            return true;
        }
        default: {
            return false;
        }
    }
}

}
}