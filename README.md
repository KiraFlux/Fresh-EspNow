# Fresh EspNow

**Удобная и современная C++ обёртка для протокола ESP NOW**

## Обзор

Библиотека предоставляет типобезопасный интерфейс для работы с ESP-NOW на ESP32/ESP8266. Абстрагирует низкоуровневые детали.

## Основные возможности

- Единая система обработки ошибок через `rs::Result<void, kf::espnow::Error>`
- Типобезопасные MAC-адреса (`kf::espnow::Mac`)
- Упрощение отправки данных
- Header-only реализация
- Нулевые аллокации памяти

## Установка

Добавьте в `platformio.ini`:
```ini
lib_deps =
    https://github.com/KiraFlux/Fresh-EspNow.git
```

## Пример использования
```cpp
#include <Arduino.h>
#include <kf/espnow.hpp> 

using kf::espnow::Mac;

struct SensorData {
    float temp;
    float humidity;
};

Mac partner = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

void setup() {
    Serial.begin(115200);
    
    // Инициализация ESP-NOW
    if (auto result = kf::espnow::Protocol::init(); result.fail()) {
        Serial.printf("Ошибка инициализации: %s\n", rs::toString(result.error));
        return;
    }

    auto &e = kf::espnow::Protocol::instance();

    // Обработчики событий
    e.setDeliveryHandler([](const Mac &mac, auto status) {
        Serial.printf("Доставка %s: %s\n", 
            rs::toArrayString(mac).data(), 
            status == kf::espnow::Protocol::DeliveryStatus::Ok ? "OK" : "FAIL");
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
    if (auto res = kf::espnow::Peer::add(partner); res.fail()) {
        Serial.printf("Ошибка добавления: %s\n", rs::toString(res.error));
    } else {
        Serial.printf("Пир %s добавлен\n", rs::toArrayString(partner).data());
    }
}

void loop() {
    static uint32_t counter = 0;
    
    SensorData data{
        .temp = 25.0f + (counter % 10),
        .humidity = 50.0f + (counter % 5)
    };
    
    counter += 1;
    
    if (auto res = kf::espnow::Protocol::send(partner, data); res.fail()) {
        Serial.printf("Ошибка отправки: %s\n", rs::toString(res.error));
    }
    
    delay(2000);
}
```

## Ключевые компоненты

### 1. Инициализация и завершение
```cpp
// Инициализация ESP-NOW
kf::espnow::Protocol::init() -> rs::Result<void, kf::espnow::Error>

// Завершение работы ESP-NOW  
kf::espnow::Protocol::quit()

// Пример использования:
if (auto res = kf::espnow::Protocol::init(); res.fail()) {
    Serial.printf("Ошибка: %s\n", rs::toString(res.error));
}
```

### 2. Работа с MAC-адресами
```cpp
// Безопасный тип для MAC адреса
using Mac = std::array<rs::u8, ESP_NOW_ETH_ALEN>;

// Пример использования:
Mac device = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC};
Serial.printf("MAC: %s\n", rs::toArrayString(device).data());

// Получение собственного MAC-адреса
auto my_mac = kf::espnow::Protocol::instance().mac;
```

### 3. Управление пирами
```cpp
// Добавление пира
kf::espnow::Peer::add(const Mac &mac) -> rs::Result<void, kf::espnow::Error>

// Удаление пира
kf::espnow::Peer::del(const Mac &mac) -> rs::Result<void, kf::espnow::Error>

// Проверка существования пира
kf::espnow::Peer::exist(const Mac &mac) -> bool

// Пример использования:
if (auto res = kf::espnow::Peer::add(mac); res.fail()) {
    Serial.printf("Ошибка: %s\n", rs::toString(res.error));
}
```

### 4. Передача данных
```cpp
// Отправка структуры
template<typename T>
kf::espnow::Protocol::send(const Mac &mac, const T &value) 
    -> rs::Result<void, kf::espnow::Error>

// Отправка сырых данных
kf::espnow::Protocol::send(const Mac &mac, const void *data, rs::u8 size) 
    -> rs::Result<void, kf::espnow::Error>

// Пример использования:
if (auto res = kf::espnow::Protocol::send(target, data); res.fail()) {
    Serial.printf("Ошибка: %s\n", rs::toString(res.error));
}
```

### 5. Обработка событий
```cpp
// Установка обработчика доставки
kf::espnow::Protocol::setDeliveryHandler(OnDeliveryFunction &&handler) 
    -> rs::Result<void, kf::espnow::Error>

// Установка обработчика приёма
kf::espnow::Protocol::setReceiveHandler(OnReceiveFunction &&handler) 
    -> rs::Result<void, kf::espnow::Error>

// Пример использования:
e.setDeliveryHandler([](const Mac& mac, DeliveryStatus status) {
    Serial.printf("Статус %s: %s\n", 
        rs::toArrayString(mac).data(), 
        status == kf::espnow::Protocol::DeliveryStatus::Ok ? "OK" : "FAIL");
});
```

## Единый тип ошибок

Все методы используют общий тип ошибок `kf::espnow::Error`:

```cpp
enum class Error : rs::u8 {
    NotInitialized,        // Протокол ESP-NOW не был инициализирован
    InternalError,         // Внутренняя ошибка ESP-NOW API
    UnknownError,          // Неизвестная ошибка ESP API
    TooBigMessage,         // Слишком большое сообщение
    InvalidArg,            // Неверный аргумент
    NoMemory,              // Не хватает памяти
    PeerNotFound,          // Пир не найден
    IncorrectWiFiMode,     // Неверный режим WiFi
    PeerListIsFull,        // Список пиров полон
    PeerAlreadyExists,     // Пир уже существует
};
```

## Лицензия

MIT License - Подробнее в [LICENSE](LICENSE)
