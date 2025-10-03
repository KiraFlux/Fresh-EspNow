#pragma once

#include <cstdint>
#include <cstring>
#include <esp_mac.h>
#include <esp_now.h>
#include <functional>
#include <rs/Result.hpp>
#include <rs/aliases.hpp>

#include <kf/espnow/Error.hpp>
#include <kf/espnow/Mac.hpp>

namespace kf::espnow {

/// Обёртка над ESP-NOW API с использованием С++
struct Protocol {

    /// Статус доставки
    enum class DeliveryStatus {
        Ok = 0x00,  ///< Пакет дошел до получателя
        Fail = 0x01,///< Не удалось доставить пакет
    };

    static DeliveryStatus translateDeliveryStatus(esp_now_send_status_t status) {
        return (status == ESP_NOW_SEND_SUCCESS) ? DeliveryStatus::Ok : DeliveryStatus::Fail;
    }

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
            ._on_receive = nullptr};

        return instance;
    }

    /// Инициализировать протокол ESP-NOW
    static rs::Result<void, Error> init() {
        auto result = esp_now_init();

        if (result == ESP_OK) {
            return {};
        } else {
            return {translateEspnowError(result)};
        }
    }

    /// Установить обработчик входящих сообщений
    rs::Result<void, Error> setReceiveHandler(OnReceiveFunction &&handler) {
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
            return {translateEspnowError(result)};
        }
    }

    /// Установить обработчик при доставке сообщений
    rs::Result<void, Error> setDeliveryHandler(OnDeliveryFunction &&handler) {
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
            return {translateEspnowError(result)};
        }
    }

    /// Завершить работу протокола
    static void quit() { esp_now_deinit(); }

    /// Отправить сообщение
    template<typename T> static rs::Result<void, Error> send(const Mac &mac, const T &value) {
        static_assert(sizeof(T) < ESP_NOW_MAX_DATA_LEN, "Message is too big!");

        const auto result = esp_now_send(
            mac.data(),
            reinterpret_cast<const rs::u8 *>(&value),
            sizeof(T));

        if (result == ESP_OK) {
            return {};
        } else {
            return {translateEspnowError(result)};
        }
    }

    /// Отправить сообщение (данные из буфера)
    static rs::Result<void, Error> send(const Mac &mac, const void *data, rs::u8 size) {
        if (size > ESP_NOW_MAX_DATA_LEN) { return {Error::TooBigMessage}; }

        auto result = esp_now_send(
            mac.data(),
            reinterpret_cast<const rs::u8 *>(data),
            size);

        if (result == ESP_OK) {
            return {};
        } else {
            return {translateEspnowError(result)};
        }
    }

private:
    static void onReceive(const rs::u8 *mac, const rs::u8 *data, int size) {
        instance()._on_receive(
            castMac(mac),
            static_cast<const void *>(data),
            static_cast<rs::u8>(size));
    }

    static void onDelivery(const rs::u8 *mac, esp_now_send_status_t status) {
        instance()._on_delivery(
            castMac(mac),
            translateDeliveryStatus(status));
    }

    inline static const Mac &castMac(const rs::u8 *mac) {
        return *reinterpret_cast<const Mac *>(mac);
    }

    /// Получить свой MAC адрес
    static Mac getSelfMac() {
        Mac mac{};
        esp_read_mac(mac.data(), ESP_MAC_WIFI_STA);
        return mac;
    }

public:
    Protocol() = delete;

    Protocol(const Protocol &) = delete;

    Protocol &operator=(const Protocol &) = delete;
};
}// namespace kf::espnow