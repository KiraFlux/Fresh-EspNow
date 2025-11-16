#pragma once

#include <cstring>
#include <esp_mac.h>
#include <esp_now.h>
#include <functional>

#include <kf/aliases.hpp>
#include <kf/Result.hpp>
#include "kf/tools/meta/Singleton.hpp"

#include <kf/espnow/Error.hpp>
#include <kf/espnow/Mac.hpp>


namespace kf::espnow {

/// Обёртка над ESP-NOW API с использованием С++
struct Protocol : tools::Singleton<Protocol> {
    friend struct Singleton<Protocol>;

    /// Статус доставки
    enum class DeliveryStatus : u8 {

        /// Пакет дошел до получателя
        Ok = 0x00,

        /// Не удалось доставить пакет
        Fail = 0x01,
    };

    /// Обработчик доставки
    using DeliveryHandler = std::function<void(const Mac &, DeliveryStatus)>;

    /// Обработчик приёма
    using ReceiveHandler = std::function<void(const Mac &, kf::slice<const void>)>;

    /// собственный Мак адрес
    const Mac mac{
        []() -> Mac {
            Mac ret{};
            esp_read_mac(ret.data(), ESP_MAC_WIFI_STA);
            return ret;
        }()
    };

    /// Обработчик доставки сообщения
    DeliveryHandler delivery_handler{nullptr};

    /// Обработчик получения сообщения
    ReceiveHandler receive_handler{nullptr};

    /// Инициализировать протокол ESP-NOW
    static Result<void, Error> init() {
        const auto result = esp_now_init();

        if (ESP_OK == result) {
            return {};
        } else {
            return {translateEspnowError(result)};
        }
    }

    /// Завершить работу протокола
    static void quit() {
        esp_now_deinit();
    }

    /// Установить обработчик входящих сообщений
    Result<void, Error> setReceiveHandler(ReceiveHandler &&handler) {
        receive_handler = std::move(handler);

        esp_err_t result;

        if (nullptr == receive_handler) {
            result = esp_now_unregister_recv_cb();
        } else {
            result = esp_now_register_recv_cb(onReceive);
        }

        if (ESP_OK == result) {
            return {};
        } else {
            return {translateEspnowError(result)};
        }
    }

    /// Установить обработчик при доставке сообщений
    Result<void, Error> setDeliveryHandler(DeliveryHandler &&handler) {
        delivery_handler = std::move(handler);

        esp_err_t result;

        if (nullptr == delivery_handler) {
            result = esp_now_unregister_send_cb();
        } else {
            result = esp_now_register_send_cb(onDelivery);
        }

        if (ESP_OK == result) {
            return {};
        } else {
            return {translateEspnowError(result)};
        }
    }

    /// Отправить сообщение
    template<typename T> static Result<void, Error> send(const Mac &mac, const T &value) {
        static_assert(sizeof(T) < ESP_NOW_MAX_DATA_LEN, "Message is too big!");

        const auto result = esp_now_send(
            mac.data(),
            reinterpret_cast<const u8 *>(&value),
            sizeof(T)
        );

        if (ESP_OK == result) {
            return {};
        } else {
            return {translateEspnowError(result)};
        }
    }

    static Result<void, Error> send(const Mac &mac, slice<const void> source) {
        if (source.size > ESP_NOW_MAX_DATA_LEN) {
            return {Error::TooBigMessage};
        }

        const auto result = esp_now_send(
            mac.data(),
            reinterpret_cast<const u8 *>(source.ptr),
            source.size
        );

        if (ESP_OK == result) {
            return {};
        } else {
            return {translateEspnowError(result)};
        }
    }

private:

    static void onReceive(const u8 *mac, const u8 *data, int size) {
        instance().receive_handler(
            *reinterpret_cast<const Mac *>(mac),
            {
                .ptr = static_cast<const void *>(data),
                .size = static_cast<usize>(size)
            }
        );
    }

    static void onDelivery(const u8 *mac, esp_now_send_status_t status) {
        instance().delivery_handler(
            *reinterpret_cast<const Mac *>(mac),
            (status == ESP_NOW_SEND_SUCCESS) ? DeliveryStatus::Ok : DeliveryStatus::Fail
        );
    }
};

}// namespace kf::espnow