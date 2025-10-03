#pragma once

#include <esp_now.h>
#include <rs/ArrayString.hpp>
#include <rs/aliases.hpp>

namespace kf::espnow {

/// @brief Безопасный тип для MAC адреса
using Mac = std::array<rs::u8, ESP_NOW_ETH_ALEN>;

}// namespace kf::espnow

//

static constexpr auto mac_array_string_size = sizeof("0000-0000-0000");

namespace rs {

/// @brief Преобразовать MAC адрес в массив-строку
static rs::ArrayString<mac_array_string_size> toArrayString(const kf::espnow::Mac &mac) {
    constexpr char mac_format[] = "%02x%02x-%02x%02x-%02x%02x";
    const auto p = mac.data();
    return rs::formatted<mac_array_string_size>(mac_format, p[0], p[1], p[2], p[3], p[4], p[5]);
}

}// namespace rs