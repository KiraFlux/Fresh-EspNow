#pragma once

#include <cstdint>
#include <functional>
#include <cstring>

#include <esp_now.h>
#include <esp_mac.h>

#include "rs/Result.hpp"
#include "rs/primitives.hpp"
#include "rs/macro.hpp"

#include "Peer.hpp"
#include "Mac.hpp"


namespace espnow {

/// Обёртка над ESP-NOW API с использованием С++
struct Protocol {

    /// Статус доставки
    enum class DeliveryStatus : rs::u8 {
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
    enum class Init : rs::u8 {
        Ok = 0x00, ///< Инициализация прошла успешно
        InternalError, ///< Внутренняя ошибка ESP-NOW API
        UnknownError, ///< Неизвестная ошибка ESP API
    };

    /// Инициализировать протокол ESP-NOW
    static rs::Result<Init> init() {
        return {translateInit(esp_now_init())};
    }

    enum class SetHandler : rs::u8 {
        Ok = 0x00, ///< Обработчик успешно подключен
        NotInit, ///< Протокол ESP-NOW не был инициализирован
        InternalError, ///< Внутренняя ошибка ESP-NOW API
        UnknownError, ///< Неизвестная ошибка ESP API
    };

    /// Установить обработчик входящих сообщений
    rs::Result<SetHandler> setReceiveHandler(OnReceiveFunction &&handler) {
        _on_receive = std::move(handler);

        esp_err_t result;

        if (_on_receive == nullptr) {
            result = esp_now_unregister_recv_cb();
        } else {
            result = esp_now_register_recv_cb(onReceive);
        }

        return {translateSetHandler(result)};
    }

    /// Установить обработчик при доставке сообщений
    rs::Result<SetHandler> setDeliveryHandler(OnDeliveryFunction &&handler) {
        _on_delivery = std::move(handler);

        esp_err_t result;

        if (_on_delivery == nullptr) {
            result = esp_now_unregister_send_cb();
        } else {
            result = esp_now_register_send_cb(onDelivery);
        }

        return {translateSetHandler(result)};
    }

    /// Завершить работу протокола
    static void quit() { esp_now_deinit(); }

    /// Результат отправки сообщения
    enum class Send : rs::u8 {
        Ok = 0x00, ///< Сообщение успешно отправлено
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
    template<typename T> static rs::Result<Send> send(const Mac &mac, const T &value) {
        static_assert(sizeof(T) < ESP_NOW_MAX_DATA_LEN, "Message is too big!");

        return {
            translateSend(esp_now_send(
                mac.data(),
                reinterpret_cast<const rs::u8 *>(&value),
                sizeof(T)
            ))
        };
    }

    /// Отправить сообщение (данные из буфера)
    static rs::Result<Send> send(const Mac &mac, const void *data, rs::u8 size) {
        if (size > ESP_NOW_MAX_DATA_LEN) { return {Send::TooBigMessage}; }

        return {
            translateSend(esp_now_send(
                mac.data(),
                reinterpret_cast<const rs::u8 *>(data),
                size
            ))
        };
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

    static Init translateInit(esp_err_t result) {
        switch (result) {
            case ESP_OK:
                return Init::Ok;
            case ESP_ERR_ESPNOW_INTERNAL:
                return Init::InternalError;
            default:
                return Init::UnknownError;
        }
    }

    static SetHandler translateSetHandler(esp_err_t result) {
        switch (result) {
            case ESP_OK:
                return SetHandler::Ok;
            case ESP_ERR_ESPNOW_NOT_INIT:
                return SetHandler::NotInit;
            case ESP_ERR_ESPNOW_INTERNAL:
                return SetHandler::InternalError;
            default:
                return SetHandler::UnknownError;
        }
    }

    static Send translateSend(esp_err_t result) {
        switch (result) {
            case ESP_OK:
                return Send::Ok;
            case ESP_ERR_ESPNOW_NOT_INIT:
                return Send::NotInit;
            case ESP_ERR_ESPNOW_ARG:
                return Send::InvalidArg;
            case ESP_ERR_ESPNOW_INTERNAL:
                return Send::InternalError;
            case ESP_ERR_ESPNOW_NO_MEM:
                return Send::NoMemory;
            case ESP_ERR_ESPNOW_NOT_FOUND:
                return Send::PeerNotFound;
            case ESP_ERR_ESPNOW_IF:
                return Send::IncorrectWiFiMode;
            default:
                return Send::UnknownError;
        }
    }

public:

    Protocol() = delete;

    Protocol(const Protocol &) = delete;

    Protocol &operator=(const Protocol &) = delete;
};
}

namespace rs {
static str toString(espnow::Protocol::SetHandler value) {
    switch (value) {
        return_case(espnow::Protocol::SetHandler::Ok)
        return_case(espnow::Protocol::SetHandler::NotInit)
        return_case(espnow::Protocol::SetHandler::InternalError)
        return_case(espnow::Protocol::SetHandler::UnknownError)
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

static str toString(espnow::Protocol::Init value) {
    switch (value) {
        return_case(espnow::Protocol::Init::Ok)
        return_case(espnow::Protocol::Init::InternalError)
        return_case(espnow::Protocol::Init::UnknownError)
        return_default()
    }
}


static str toString(espnow::Protocol::Send value) {
    switch (value) {
        return_case(espnow::Protocol::Send::Ok)
        return_case(espnow::Protocol::Send::TooBigMessage)
        return_case(espnow::Protocol::Send::NotInit)
        return_case(espnow::Protocol::Send::InvalidArg)
        return_case(espnow::Protocol::Send::InternalError)
        return_case(espnow::Protocol::Send::NoMemory)
        return_case(espnow::Protocol::Send::PeerNotFound)
        return_case(espnow::Protocol::Send::IncorrectWiFiMode)
        return_case(espnow::Protocol::Send::UnknownError)
        return_default()
    }
}

}
