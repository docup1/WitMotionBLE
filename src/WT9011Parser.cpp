#include "WT9011Parser.h"
#include <cstring>
#include <sstream>
#include <iomanip>

std::optional<WT9011Data> WT9011Parser::parse(const std::array<uint8_t, 20>& data) {
    if (data.size() < 20) return std::nullopt;
    if (data[0] != 0x55 || data[1] != 0x61) return std::nullopt;

    auto to_int16 = [](uint8_t low, uint8_t high) -> int16_t {
        return static_cast<int16_t>((high << 8) | low);
    };

    int16_t ax_raw = to_int16(data[2], data[3]);
    int16_t ay_raw = to_int16(data[4], data[5]);
    int16_t az_raw = to_int16(data[6], data[7]);

    int16_t gx_raw = to_int16(data[8], data[9]);
    int16_t gy_raw = to_int16(data[10], data[11]);
    int16_t gz_raw = to_int16(data[12], data[13]);

    int16_t roll_raw = to_int16(data[14], data[15]);
    int16_t pitch_raw = to_int16(data[16], data[17]);
    int16_t yaw_raw = to_int16(data[18], data[19]);

    WT9011Data result;
    result.ax = ax_raw / 32768.0f * 16.0f;
    result.ay = ay_raw / 32768.0f * 16.0f;
    result.az = az_raw / 32768.0f * 16.0f;

    result.gx = gx_raw / 32768.0f * 2000.0f;
    result.gy = gy_raw / 32768.0f * 2000.0f;
    result.gz = gz_raw / 32768.0f * 2000.0f;

    result.roll = roll_raw / 32768.0f * 180.0f;
    result.pitch = pitch_raw / 32768.0f * 180.0f;
    result.yaw = yaw_raw / 32768.0f * 180.0f;

    if (result.yaw > 180.0f) {
        result.yaw -= 360.0f;
    }

    return result;
}

std::string WT9011Parser::format(const WT9011Data& data) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    oss << "Accel: x=" << data.ax << "g y=" << data.ay << "g z=" << data.az << "g | ";
    oss << std::setprecision(1);
    oss << "Gyro: x=" << data.gx << "°/s y=" << data.gy << "°/s z=" << data.gz << "°/s | ";
    oss << "Angle: roll=" << data.roll << "° pitch=" << data.pitch << "° yaw=" << data.yaw << "°";
    return oss.str();
}
