#pragma once

#include <cstdint>
#include <functional>
#include <cstring>

#include <esp_now.h>
#include <esp_mac.h>

#include "rs/Result.hpp"
#include "rs/primitives.hpp"

#include "Peer.hpp"
#include "Mac.hpp"


namespace espnow {

/// Обёртка над ESP-NOW API с использованием С++
struct Protocol {

    /// Статус доставки
    enum class DeliveryStatus {
        Ok = 0x00, ///< Пакет дошел до получателя
        Fail = 0x01, ///< Не удалось доставить пакет
    };

    using OnDeliveryFunction = std::function<void(const Mac &, DeliveryStatus)>;
    using OnReceiveFunction = std::function<void(const Mac &, const void *, rs::u8)>;

    /// Мак адрес этого устройства
    const Mac mac;
    /// Обработчик доставки сообщения
    OnDeliveryFunction _on_delivery;
    /// Обработчик получения сообщения
    OnReceiveFunction _on_receive;

    /// Получить экземпляр протокола для настройки
    static Protocol &instance() {
        static Protocol instance = {
            .mac = getSelfMac(),
            ._on_delivery = nullptr,
            ._on_receive = nullptr
        };

        return instance;
    }

    /// Результат инициализации
    enum class InitError {
        InternalError, ///< Внутренняя ошибка ESP-NOW API
        UnknownError, ///< Неизвестная ошибка ESP API
    };

    /// Инициализировать протокол ESP-NOW
    static rs::Result<void, InitError> init() {
        auto result = esp_now_init();

        if (result == ESP_OK) {
            return {};
        } else {
            return {translateInit(result)};
        }
    }

    enum class HandlerSetError {
        NotInit, ///< Протокол ESP-NOW не был инициализирован
        InternalError, ///< Внутренняя ошибка ESP-NOW API
        UnknownError, ///< Неизвестная ошибка ESP API
    };

    /// Установить обработчик входящих сообщений
    rs::Result<void, HandlerSetError> setReceiveHandler(OnReceiveFunction &&handler) {
        _on_receive = std::move(handler);

        esp_err_t result;

        if (_on_receive == nullptr) {
            result = esp_now_unregister_recv_cb();
        } else {
            result = esp_now_register_recv_cb(onReceive);
        }

        if (result == ESP_OK) {
            return {};
        } else {
            return {translateSetHandler(result)};
        }
    }

    /// Установить обработчик при доставке сообщений
    rs::Result<void, HandlerSetError> setDeliveryHandler(OnDeliveryFunction &&handler) {
        _on_delivery = std::move(handler);

        esp_err_t result;

        if (_on_delivery == nullptr) {
            result = esp_now_unregister_send_cb();
        } else {
            result = esp_now_register_send_cb(onDelivery);
        }

        if (result == ESP_OK) {
            return {};
        } else {
            return {translateSetHandler(result)};
        }
    }

    /// Завершить работу протокола
    static void quit() { esp_now_deinit(); }

    /// Результат отправки сообщения
    enum class SendError {
        NotInit, ///< Протокол ESP-NOW не был инициализирован
        TooBigMessage, ///< Слишком большое сообщение
        InvalidArg, ///< Неверный аргумент
        InternalError, ///< Внутренняя ошибка ESP-NOW API
        NoMemory, ///< Не хватает памяти для отправки сообщения
        PeerNotFound, ///< Целевой пир не найден
        IncorrectWiFiMode, ///< Установлен неверный режим интерфейса WiFi
        UnknownError, ///< Неизвестная ошибка ESP API
    };

    /// Отправить сообщение
    template<typename T> static rs::Result<void, SendError> send(const Mac &mac, const T &value) {
        static_assert(sizeof(T) < ESP_NOW_MAX_DATA_LEN, "Message is too big!");

        auto result = esp_now_send(
            mac.data(),
            reinterpret_cast<const rs::u8 *>(&value),
            sizeof(T)
        );

        if (result == ESP_OK) {
            return {};
        } else {
            return {translateSend(result)};
        }
    }

    /// Отправить сообщение (данные из буфера)
    static rs::Result<void, SendError> send(const Mac &mac, const void *data, rs::u8 size) {
        if (size > ESP_NOW_MAX_DATA_LEN) { return {SendError::TooBigMessage}; }

        auto result = esp_now_send(
            mac.data(),
            reinterpret_cast<const rs::u8 *>(data),
            size
        );

        if (result == ESP_OK) {
            return {};
        } else {
            return {translateSend(result)};
        }
    }

private:

    static void onReceive(const rs::u8 *mac, const rs::u8 *data, int size) {
        instance()._on_receive(
            castMac(mac),
            static_cast<const void *>(data),
            static_cast<rs::u8>(size)
        );
    }

    static void onDelivery(const rs::u8 *mac, esp_now_send_status_t status) {
        instance()._on_delivery(
            castMac(mac),
            translateDeliveryStatus(status)
        );
    }

    inline static const Mac &castMac(const rs::u8 *mac) {
        return *reinterpret_cast<const Mac *>(mac);
    }

    /// Получить свой MAC адрес
    static Mac getSelfMac() {
        Mac ret = {};
        esp_read_mac(ret.data(), ESP_MAC_WIFI_STA);
        return ret;
    }

private:

    static DeliveryStatus translateDeliveryStatus(esp_now_send_status_t status) {
        return (status == ESP_NOW_SEND_SUCCESS) ? DeliveryStatus::Ok : DeliveryStatus::Fail;
    }

    static InitError translateInit(esp_err_t result) {
        if (result == ESP_ERR_ESPNOW_INTERNAL) {
            return InitError::InternalError;
        }
        return InitError::UnknownError;
    }

    static HandlerSetError translateSetHandler(esp_err_t result) {
        switch (result) {
            case ESP_ERR_ESPNOW_NOT_INIT:
                return HandlerSetError::NotInit;
            case ESP_ERR_ESPNOW_INTERNAL:
                return HandlerSetError::InternalError;
            default:
                return HandlerSetError::UnknownError;
        }
    }

    static SendError translateSend(esp_err_t result) {
        switch (result) {
            case ESP_ERR_ESPNOW_NOT_INIT:
                return SendError::NotInit;
            case ESP_ERR_ESPNOW_ARG:
                return SendError::InvalidArg;
            case ESP_ERR_ESPNOW_INTERNAL:
                return SendError::InternalError;
            case ESP_ERR_ESPNOW_NO_MEM:
                return SendError::NoMemory;
            case ESP_ERR_ESPNOW_NOT_FOUND:
                return SendError::PeerNotFound;
            case ESP_ERR_ESPNOW_IF:
                return SendError::IncorrectWiFiMode;
            default:
                return SendError::UnknownError;
        }
    }

public:

    Protocol() = delete;

    Protocol(const Protocol &) = delete;

    Protocol &operator=(const Protocol &) = delete;
};
}

namespace rs {

#define return_case(__v) case __v: return #__v;
#define return_default() default: return "Invalid";

static str toString(espnow::Protocol::HandlerSetError value) {
    switch (value) {
        return_case(espnow::Protocol::HandlerSetError::NotInit)
        return_case(espnow::Protocol::HandlerSetError::InternalError)
        return_case(espnow::Protocol::HandlerSetError::UnknownError)
        return_default()
    }
}

static str toString(espnow::Protocol::DeliveryStatus status) {
    switch (status) {
        return_case(espnow::Protocol::DeliveryStatus::Ok)
        return_case(espnow::Protocol::DeliveryStatus::Fail)
        return_default()
    }
}

static str toString(espnow::Protocol::InitError value) {
    switch (value) {
        return_case(espnow::Protocol::InitError::InternalError)
        return_case(espnow::Protocol::InitError::UnknownError)
        return_default()
    }
}


static str toString(espnow::Protocol::SendError value) {
    switch (value) {
        return_case(espnow::Protocol::SendError::TooBigMessage)
        return_case(espnow::Protocol::SendError::NotInit)
        return_case(espnow::Protocol::SendError::InvalidArg)
        return_case(espnow::Protocol::SendError::InternalError)
        return_case(espnow::Protocol::SendError::NoMemory)
        return_case(espnow::Protocol::SendError::PeerNotFound)
        return_case(espnow::Protocol::SendError::IncorrectWiFiMode)
        return_case(espnow::Protocol::SendError::UnknownError)
        return_default()
    }
}

#undef return_case
#undef return_default

}
