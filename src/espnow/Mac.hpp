#pragma once

#include <esp_now.h>

#include "rs/aliases.hpp"
#include "rs/ArrayString.hpp"


namespace espnow {

/// Безопасный тип для MAC адреса
using Mac = std::array<rs::u8, ESP_NOW_ETH_ALEN>;

} // namespace espnow

static constexpr auto mac_array_string_size = sizeof("0000-0000-0000");

namespace rs {
/// Преобразовать MAC адрес в массив-строку
static rs::ArrayString<mac_array_string_size> toArrayString(const espnow::Mac &mac) {
    constexpr char mac_format[] = "%02x%02x-%02x%02x-%02x%02x";

    auto raw = mac.data();

    return rs::formatted<mac_array_string_size>(
        mac_format,
        raw[0],
        raw[1],
        raw[2],
        raw[3],
        raw[4],
        raw[5]
    );
}

} // namespace rs