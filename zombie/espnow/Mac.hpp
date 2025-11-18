#pragma once

#include <esp_now.h>
#include <kf/String.hpp>
#include <kf/aliases.hpp>


namespace kf::espnow {

/// @brief Безопасный тип для MAC адреса
using Mac = std::array<kf::u8, ESP_NOW_ETH_ALEN>;

static constexpr auto mac_array_string_size = sizeof("0000-0000-0000");


/// @brief Преобразовать MAC адрес в массив-строку
static kf::ArrayString<mac_array_string_size> stringFromMac(const kf::espnow::Mac &mac) {
    constexpr char mac_format[] = "%02x%02x-%02x%02x-%02x%02x";

    kf::ArrayString<mac_array_string_size> ret{};
    const auto p = mac.data();
    kf::formatTo(ret, mac_format, p[0], p[1], p[2], p[3], p[4], p[5]);

    return ret;
}

}// namespace kf::espnow




