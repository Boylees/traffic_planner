#include "utils.h"

// mode_to_string 函数的实现
const char* mode_to_string(TransportMode mode) {
    switch (mode) {
        case DRIVING: return "driving";
        case HIGH_SPEED_RAIL: return "high_speed_rail";
        case FLIGHT: return "flight";
        case BUS: return "bus";
        default: return "unknown";
    }
}

// mode_to_string_cn 函数的实现
const char* mode_to_string_cn(TransportMode mode) {
    switch (mode) {
        case DRIVING: return "驾车";
        case HIGH_SPEED_RAIL: return "高铁";
        case FLIGHT: return "飞机";
        case BUS: return "公交";
        default: return "未知";
    }
} 