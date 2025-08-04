#pragma once

#include "rs/Result.hpp"

#include "Mac.hpp"


namespace espnow {

/// Управление пирами
struct Peer {

public:

    /// Результат добавления пира
    enum class AddError {
        NotInit, ///< Протокол ESP-NOW не был инициализирован
        InvalidArg, ///< Неверный аргумент
        Full, ///< Список пиров полон
        NoMemory, ///< Не хватает памяти для добавления пира
        Exists, ///< Пир уже добавлен
        UnknownError, ///< Неизвестная ошибка ESP API
    };

    /// Добавить пир
    static rs::Result<void, AddError> add(const Mac &mac) {
        esp_now_peer_info_t peer = {
            .channel = 0,
            .ifidx = WIFI_IF_STA,
            .encrypt = false,
        };

        std::copy(mac.begin(), mac.end(), peer.peer_addr);

        auto result = esp_now_add_peer(&peer);

        if (result == ESP_OK) {
            return {};
        } else {
            return {translatePeerAdd(result)};
        }
    }

    /// Удалить пир
    enum class DeleteError {
        NotInit, /// Протокол ESP-NOW не был инициализирован
        InvalidArg, /// Неверный аргумент
        NotFound, /// Пир не найден в списке добавленных
        UnknownError, /// Неизвестная ошибка ESP API
    };

    /// Удалить пир
    static rs::Result<void, DeleteError> del(const Mac &mac) {
        auto result = esp_now_del_peer(mac.data());

        if (result == ESP_OK) {
            return {};
        } else {
            return {translatePeerDelete(result)};
        }
    }

    /// Проверить существование пира
    static bool exist(const Mac &mac) {
        return esp_now_is_peer_exist(mac.data());
    }

private:

    static DeleteError translatePeerDelete(esp_err_t result) {
        switch (result) {
            case ESP_ERR_ESPNOW_NOT_INIT:
                return DeleteError::NotInit;
            case ESP_ERR_ESPNOW_ARG:
                return DeleteError::InvalidArg;
            case ESP_ERR_ESPNOW_NOT_FOUND:
                return DeleteError::NotFound;
            default:
                return DeleteError::UnknownError;
        }
    }

    static AddError translatePeerAdd(esp_err_t result) {
        switch (result) {
            case ESP_ERR_ESPNOW_NOT_INIT:
                return AddError::NotInit;
            case ESP_ERR_ESPNOW_ARG:
                return AddError::InvalidArg;
            case ESP_ERR_ESPNOW_FULL:
                return AddError::Full;
            case ESP_ERR_ESPNOW_NO_MEM:
                return AddError::NoMemory;
            case ESP_ERR_ESPNOW_EXIST:
                return AddError::Exists;
            default:
                return AddError::UnknownError;
        }
    }
};

}

namespace rs {

#define return_case(__v) case __v: return #__v;
#define return_default() default: return "Invalid";

static str toString(espnow::Peer::AddError value) {
    switch (value) {
        return_case(espnow::Peer::AddError::NotInit)
        return_case(espnow::Peer::AddError::InvalidArg)
        return_case(espnow::Peer::AddError::Full)
        return_case(espnow::Peer::AddError::NoMemory)
        return_case(espnow::Peer::AddError::Exists)
        return_case(espnow::Peer::AddError::UnknownError)
        return_default()
    }
}

static str toString(espnow::Peer::DeleteError value) {
    switch (value) {
        return_case(espnow::Peer::DeleteError::NotInit)
        return_case(espnow::Peer::DeleteError::InvalidArg)
        return_case(espnow::Peer::DeleteError::NotFound)
        return_case(espnow::Peer::DeleteError::UnknownError)
        return_default()
    }
}

#undef return_case
#undef return_default

}