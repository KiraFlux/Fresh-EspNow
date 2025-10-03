#pragma once

#include <rs/Result.hpp>

#include <kf/espnow/Error.hpp>
#include <kf/espnow/Mac.hpp>

namespace kf::espnow {

/// Управление пирами
struct Peer {

public:
    /// Добавить пир
    static rs::Result<void, Error> add(const Mac &mac) {
        esp_now_peer_info_t peer = {
            .channel = 0,
            .ifidx = WIFI_IF_STA,
            .encrypt = false,
        };

        std::copy(mac.begin(), mac.end(), peer.peer_addr);

        const auto result = esp_now_add_peer(&peer);

        if (result == ESP_OK) {
            return {};
        } else {
            return {translateEspnowError(result)};
        }
    }

    /// Удалить пир
    static rs::Result<void, Error> del(const Mac &mac) {
        const auto result = esp_now_del_peer(mac.data());

        if (result == ESP_OK) {
            return {};
        } else {
            return {translateEspnowError(result)};
        }
    }

    /// Проверить существование пира
    static bool exist(const Mac &mac) {
        return esp_now_is_peer_exist(mac.data());
    }
};

}// namespace kf::espnow
