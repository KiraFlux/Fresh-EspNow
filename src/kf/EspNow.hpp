#pragma once

#include <array>
#include <esp_now.h>
#include <kf/Result.hpp>
#include <kf/String.hpp>
#include <kf/aliases.hpp>

static constexpr auto mac_array_string_size = sizeof("0000-0000-0000");

namespace kf {

/// @brief Инкапсулирует работу ESP NOW в безопастных абстракциях
struct EspNow {

    /// @brief Безопасный тип для MAC адреса
    using Mac = std::array<u8, ESP_NOW_ETH_ALEN>;

    /// @brief Перечисление ошибок операций API Espnow
    enum class Error : u8 {

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

        // Другое

        /// @brief Слишком большое сообщение
        TooBigMessage,
    };

    /// @brief Пир
    struct Peer {

        /// @brief MAC адрес пира
        const Mac mac;

        /// @brief Добавить пир
        static Result<Peer, Error> add(const Mac &mac) {
            esp_now_peer_info_t peer = {
                .channel = 0,
                .ifidx = WIFI_IF_STA,
                .encrypt = false,
            };

            std::copy(mac.begin(), mac.end(), peer.peer_addr);

            const auto result = esp_now_add_peer(&peer);

            if (ESP_OK == result) {
                return {Peer{mac}};
            } else {
                return {translateEspnowError(result)};
            }
        }

        /// @brief Удалить пир
        [[nodiscard]] Result<void, Error> del() {
            const auto result = esp_now_del_peer(mac.data());

            if (ESP_OK == result) {
                return {};
            } else {
                return {translateEspnowError(result)};
            }
        }

        /// @brief Проверить существование пира
        [[nodiscard]] bool exist() {
            return esp_now_is_peer_exist(mac.data());
        }

    private:
        // Создание пира только через Peer::add
        Peer(const Mac &mac) : mac{mac} {}
    };

    // methods

    /// @brief Инициализировать протокол ESP-NOW
    [[nodiscard]] static Result<void, Error> init() {
        const auto result = esp_now_init();

        if (ESP_OK == result) {
            return {};
        } else {
            return {translateEspnowError(result)};
        }
    }

    /// @brief Завершить работу протокола
    static void quit() {
        esp_now_deinit();
    }

public:
    // to string

    /// @brief Преобразовать MAC адрес в массив-строку
    static ArrayString<mac_array_string_size> stringFromMac(const Mac &mac) {
        ArrayString<mac_array_string_size> ret{};
        const auto p = mac.data();
        formatTo(ret, "%02x%02x-%02x%02x-%02x%02x", p[0], p[1], p[2], p[3], p[4], p[5]);
        return ret;
    }

#define return_case(__v) \
    case __v: return #__v
#define return_default() \
    default: return "Invalid"

    static const char *stringFromError(kf::EspNow::Error error) {
        switch (error) {
            return_case(kf::EspNow::Error::NotInitialized);
            return_case(kf::EspNow::Error::InternalError);
            return_case(kf::EspNow::Error::UnknownError);
            return_case(kf::EspNow::Error::TooBigMessage);
            return_case(kf::EspNow::Error::InvalidArg);
            return_case(kf::EspNow::Error::NoMemory);
            return_case(kf::EspNow::Error::PeerNotFound);
            return_case(kf::EspNow::Error::IncorrectWiFiMode);
            return_case(kf::EspNow::Error::PeerListIsFull);
            return_case(kf::EspNow::Error::PeerAlreadyExists);
            return_default();
        }
    }

#undef return_case
#undef return_default

private:
    /// @brief Перевод результата esp error в значение ошибки
    static Error translateEspnowError(esp_err_t result) {
        switch (result) {
            case ESP_ERR_ESPNOW_INTERNAL: return Error::InternalError;
            case ESP_ERR_ESPNOW_NOT_INIT: return Error::NotInitialized;
            case ESP_ERR_ESPNOW_ARG: return Error::InvalidArg;
            case ESP_ERR_ESPNOW_NO_MEM: return Error::NoMemory;
            case ESP_ERR_ESPNOW_NOT_FOUND: return Error::PeerNotFound;
            case ESP_ERR_ESPNOW_IF: return Error::IncorrectWiFiMode;
            case ESP_ERR_ESPNOW_FULL: return Error::PeerListIsFull;
            case ESP_ERR_ESPNOW_EXIST: return Error::PeerAlreadyExists;
            default: return Error::UnknownError;
        }
    }
};

}// namespace kf
