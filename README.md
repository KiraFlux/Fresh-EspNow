# Fresh-EspNow

**Удобная и современная C++ обёртка для ESP-NOW протокола**

## Обзор

Библиотека предоставляет типобезопасный интерфейс для работы с ESP-NOW на ESP32/ESP8266. Абстрагирует низкоуровневые детали с нулевыми накладными расходами.

## Основные возможности

- Обработка ошибок через `rs::Result<void, E>`, где E - это перечисление ошибок основанных на возвращаемых значениях `esp_err_t`
- Типобезопасные MAC-адреса (`espnow::Mac`)
- Упрощение отправки данных
- Header-only реализация
- Нулевые аллокации памяти

## Установка

Добавьте в `platformio.ini`:
```ini
lib_deps =
    https://github.com/JamahaW/Fresh-EspNow.git
```

## Пример использования
```cpp
#include "Arduino.h"
#include "espnow/Protocol.hpp"

using espnow::Mac;

struct SensorData {
    float temp;
    float humidity;
};

Mac partner = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

void setup() {
    Serial.begin(115200);
    
    // Инициализация ESP-NOW
    if (auto res = espnow::Protocol::init(); res.fail()) {
        Serial.printf("Ошибка инициализации: %s\n", rs::toString(res.error));
        return;
    }

    auto &e = espnow::Protocol::instance();

    // Обработчики событий
    e.setDeliveryHandler([](const Mac &mac, auto status) {
        Serial.printf("Доставка %s: %s\n", 
            rs::toArrayString(mac).data(), 
            rs::toString(status));
    });

    e.setReceiveHandler([](const Mac &mac, const void *data, auto size) {
        if (size == sizeof(SensorData)) {
            auto msg = static_cast<const SensorData*>(data);
            Serial.printf("Данные от %s: %.1fC, %.1f%%\n", 
                rs::toArrayString(mac).data(), 
                msg->temp, msg->humidity);
        }
    });

    // Добавление пира
    if (auto res = espnow::Peer::add(partner); res.fail()) {
        Serial.printf("Ошибка добавления: %s\n", rs::toString(res.error));
    } else {
        Serial.printf("Пир %s добавлен\n", rs::toArrayString(partner).data());
    }
}

void loop() {
    static uint32_t counter = 0;
    
    SensorData data = {
        .temp = 25.0f + (counter % 10),
        .humidity = 50.0f + (counter % 5)
    };
    counter++;
    
    if (auto res = espnow::Protocol::send(partner, data); res.fail()) {
        Serial.printf("Ошибка отправки: %s\n", rs::toString(res.error));
    }
    
    delay(2000);
}
```

## Ключевые компоненты

### 1. Инициализация
```cpp
espnow::Protocol::init() -> rs::Result<void, espnow::Protocol::InitError>

/// Результат инициализации
enum class InitError {
    InternalError,     // Внутренняя ошибка ESP-NOW API
    UnknownError,      // Неизвестная ошибка ESP API
};

// Пример использования:
if (auto res = espnow::Protocol::init(); res.fail()) {
    Serial.printf("Ошибка: %s\n", rs::toString(res.error));
}
```

### 2. Работа с MAC-адресами
```cpp
// Заголовочный файл: espnow/Mac.hpp

/// Безопасный тип для MAC адреса
using Mac = std::array<rs::u8, ESP_NOW_ETH_ALEN>;

// Пример использования:
Mac device = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC};
Serial.printf("MAC: %s\n", rs::toArrayString(device).data());
```

### 3. Управление пирами
```cpp
// Добавление пира
espnow::Peer::add(const Mac &mac) 
    -> rs::Result<void, espnow::Peer::AddError>

/// Ошибки добавления пира
enum class AddError {
    NotInit,        // Протокол ESP-NOW не был инициализирован
    InvalidArg,     // Неверный аргумент
    Full,           // Список пиров полон
    NoMemory,       // Не хватает памяти для добавления пира
    Exists,         // Пир уже добавлен
    UnknownError,   // Неизвестная ошибка ESP API
};

// Удаление пира
espnow::Peer::del(const Mac &mac) 
    -> rs::Result<void, espnow::Peer::DeleteError>

/// Ошибки удаления пира
enum class DeleteError {
    NotInit,        // Протокол ESP-NOW не был инициализирован
    InvalidArg,     // Неверный аргумент
    NotFound,       // Пир не найден в списке добавленных
    UnknownError,   // Неизвестная ошибка ESP API
};

// Проверка существования пира
espnow::Peer::exist(const Mac &mac) -> bool

// Пример использования:
if (auto res = espnow::Peer::add(mac); res.fail()) {
    Serial.printf("Ошибка: %s\n", rs::toString(res.error));
}
```

### 4. Передача данных
```cpp
// Отправка структуры
template<typename T>
espnow::Protocol::send(const Mac &mac, const T &value) 
    -> rs::Result<void, espnow::Protocol::SendError>

// Отправка сырых данных
espnow::Protocol::send(const Mac &mac, const void *data, rs::u8 size) 
    -> rs::Result<void, espnow::Protocol::SendError>

/// Ошибки отправки
enum class SendError {
    NotInit,            // Протокол ESP-NOW не был инициализирован
    TooBigMessage,      // Слишком большое сообщение
    InvalidArg,         // Неверный аргумент
    InternalError,      // Внутренняя ошибка ESP-NOW API
    NoMemory,           // Не хватает памяти для отправки сообщения
    PeerNotFound,       // Целевой пир не найден
    IncorrectWiFiMode,  // Установлен неверный режим интерфейса WiFi
    UnknownError,       // Неизвестная ошибка ESP API
};

// Пример использования:
if (auto res = espnow::Protocol::send(target, data); res.fail()) {
    Serial.printf("Ошибка: %s\n", rs::toString(res.error));
}
```

### 5. Обработка событий
```cpp
// Установка обработчика доставки
espnow::Protocol::setDeliveryHandler(OnDeliveryFunction &&handler) 
    -> rs::Result<void, espnow::Protocol::HandlerSetError>

/// Тип функции обработки доставки
using OnDeliveryFunction = std::function<void(const Mac &, DeliveryStatus)>;

// Установка обработчика приёма
espnow::Protocol::setReceiveHandler(OnReceiveFunction &&handler) 
    -> rs::Result<void, espnow::Protocol::HandlerSetError>

/// Тип функции обработки приёма
using OnReceiveFunction = std::function<void(const Mac &, const void *, rs::u8)>;

/// Статус доставки
enum class DeliveryStatus {
    Ok = 0x00,  // Пакет дошел до получателя
    Fail = 0x01, // Не удалось доставить пакет
};

/// Ошибки установки обработчиков
enum class HandlerSetError {
    NotInit,        // Протокол ESP-NOW не был инициализирован
    InternalError,  // Внутренняя ошибка ESP-NOW API
    UnknownError,   // Неизвестная ошибка ESP API
};

// Пример использования:
e.setDeliveryHandler([](const Mac& mac, DeliveryStatus status) {
    Serial.printf("Статус %s: %s\n", 
        rs::toArrayString(mac).data(), 
        rs::toString(status));
});
```

## Лицензия


MIT License - Подробнее в [LICENSE](LICENSE)
