#pragma once
#include <array>
#include <optional>
#include <string>

struct WT9011Data {
    float ax, ay, az;   // accel g
    float gx, gy, gz;   // gyro °/s
    float roll, pitch, yaw; // angle °
};

class WT9011Parser {
public:
    // Парсим raw данные, если валидны - возвращаем WT9011Data, иначе std::nullopt
    static std::optional<WT9011Data> parse(const std::array<uint8_t, 20>& data);
    // Формируем строку с данными для логов
    static std::string format(const WT9011Data& data);
};
