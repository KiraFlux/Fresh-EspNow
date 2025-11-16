#pragma once

#include <esp_now.h>
#include <kf/aliases.hpp>


namespace kf::espnow {

/// @brief Перечисление ошибок операций API Espnow
enum class Error : kf::u8 {
    // Инициализация

    /// @brief Внутренняя ошибка ESP-NOW API
    InternalError,

    /// @brief Неизвестная ошибка ESP API
    UnknownError,

    /// @brief Протокол ESP-NOW не был инициализирован
    NotInitialized,

    /// @brief Установлен неверный режим интерфейса WiFi
    IncorrectWiFiMode,

    // Работа с пирами

    /// @brief Список пиров полон
    PeerListIsFull,

    /// @brief Неверный аргумент
    InvalidArg,

    /// @brief Не хватает памяти для добавления пира
    NoMemory,

    /// @brief Пир уже добавлен
    PeerAlreadyExists,

    /// @brief Пир не найден в списке добавленных
    PeerNotFound,

    // Сообщение

    /// @brief Слишком большое сообщение
    TooBigMessage,
};

/// @brief Перевод результата esp error в значение ошибки
Error translateEspnowError(esp_err_t result) {
    switch (result) {
        case ESP_ERR_ESPNOW_INTERNAL:return Error::InternalError;
        case ESP_ERR_ESPNOW_NOT_INIT:return Error::NotInitialized;
        case ESP_ERR_ESPNOW_ARG:return Error::InvalidArg;
        case ESP_ERR_ESPNOW_NO_MEM:return Error::NoMemory;
        case ESP_ERR_ESPNOW_NOT_FOUND:return Error::PeerNotFound;
        case ESP_ERR_ESPNOW_IF:return Error::IncorrectWiFiMode;
        case ESP_ERR_ESPNOW_FULL:return Error::PeerListIsFull;
        case ESP_ERR_ESPNOW_EXIST:return Error::PeerAlreadyExists;
        default:return Error::UnknownError;
    }
}

#define return_case(__v) case __v: return #__v
#define return_default() default: return "Invalid"

static const char *stringFromError(Error value) {
    switch (value) {
        return_case(Error::NotInitialized);
        return_case(Error::InternalError);
        return_case(Error::UnknownError);
        return_case(Error::TooBigMessage);
        return_case(Error::InvalidArg);
        return_case(Error::NoMemory);
        return_case(Error::PeerNotFound);
        return_case(Error::IncorrectWiFiMode);
        return_case(Error::PeerListIsFull);
        return_case(Error::PeerAlreadyExists);
        return_default();
    }
}

#undef return_case
#undef return_default

}// namespace kf::espnow
