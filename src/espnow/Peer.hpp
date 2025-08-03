#pragma once

#include "rs/Result.hpp"

#include "Mac.hpp"


namespace espnow {

/// Управление пирами
struct Peer {

public:

    /// Результат добавления пира
    enum class Add : rs::u8 {
        Ok = 0x00, ///< Пир успешно добавлен
        NotInit, ///< Протокол ESP-NOW не был инициализирован
        InvalidArg, ///< Неверный аргумент
        Full, ///< Список пиров полон
        NoMemory, ///< Не хватает памяти для добавления пира
        Exists, ///< Пир уже добавлен
        UnknownError, ///< Неизвестная ошибка ESP API
    };

    /// Добавить пир
    static rs::Result<Add> add(const Mac &mac) {
        esp_now_peer_info_t peer = {
            .channel = 0,
            .ifidx = WIFI_IF_STA,
            .encrypt = false,
        };

        std::copy(mac.begin(), mac.end(), peer.peer_addr);

        return {translatePeerAdd(esp_now_add_peer(&peer))};
    }

    /// Удалить пир
    enum class Del : rs::u8 {
        Ok = 0x00, /// Пир успешно удален
        NotInit, /// Протокол ESP-NOW не был инициализирован
        InvalidArg, /// Неверный аргумент
        NotFound, /// Пир не найден в списке добавленных
        UnknownError, /// Неизвестная ошибка ESP API
    };

    /// Удалить пир
    static rs::Result<Del> del(const Mac &mac) {
        return {translatePeerDelete(esp_now_del_peer(mac.data()))};
    }

    /// Проверить существование пира
    static bool exist(const Mac &mac) {
        return esp_now_is_peer_exist(mac.data());
    }

private:

    static Del translatePeerDelete(esp_err_t result) {
        switch (result) {
            case ESP_OK:
                return Del::Ok;
            case ESP_ERR_ESPNOW_NOT_INIT:
                return Del::NotInit;
            case ESP_ERR_ESPNOW_ARG:
                return Del::InvalidArg;
            case ESP_ERR_ESPNOW_NOT_FOUND:
                return Del::NotFound;
            default:
                return Del::UnknownError;
        }
    }

    static Add translatePeerAdd(esp_err_t result) {
        switch (result) {
            case ESP_OK:
                return Add::Ok;
            case ESP_ERR_ESPNOW_NOT_INIT:
                return Add::NotInit;
            case ESP_ERR_ESPNOW_ARG:
                return Add::InvalidArg;
            case ESP_ERR_ESPNOW_FULL:
                return Add::Full;
            case ESP_ERR_ESPNOW_NO_MEM:
                return Add::NoMemory;
            case ESP_ERR_ESPNOW_EXIST:
                return Add::Exists;
            default:
                return Add::UnknownError;
        }
    }
};

}

namespace rs {

#define return_case(__v) case __v: return #__v;
#define return_default() default: return "Invalid";

static str toString(espnow::Peer::Add value) {
    switch (value) {
        return_case(espnow::Peer::Add::Ok)
        return_case(espnow::Peer::Add::NotInit)
        return_case(espnow::Peer::Add::InvalidArg)
        return_case(espnow::Peer::Add::Full)
        return_case(espnow::Peer::Add::NoMemory)
        return_case(espnow::Peer::Add::Exists)
        return_case(espnow::Peer::Add::UnknownError)
        return_default()
    }
}

static str toString(espnow::Peer::Del value) {
    switch (value) {
        return_case(espnow::Peer::Del::Ok)
        return_case(espnow::Peer::Del::NotInit)
        return_case(espnow::Peer::Del::InvalidArg)
        return_case(espnow::Peer::Del::NotFound)
        return_case(espnow::Peer::Del::UnknownError)
        return_default()
    }
}

#undef return_case
#undef return_default

}