#pragma once
// Optional untracked overrides; copy config.example.hpp to config.hpp.
#if __has_include("config.hpp")
#include "config.hpp"
#endif
#ifndef ZS_WIFI_SSID
#define ZS_WIFI_SSID ""
#endif
#ifndef ZS_WIFI_PASSWORD
#define ZS_WIFI_PASSWORD ""
#endif
#ifndef ZS_AP_SSID
#define ZS_AP_SSID "ZS-M7-Display"
#endif
#ifndef ZS_AP_PASSWORD
#define ZS_AP_PASSWORD "minidisc-test"
#endif
#if CONFIG_IDF_TARGET_ESP32C3
#ifndef ZS_SDT1_PIN
#define ZS_SDT1_PIN 7
#endif
#ifndef ZS_SCK1_PIN
#define ZS_SCK1_PIN 6
#endif
#ifndef ZS_CS1_PIN
#define ZS_CS1_PIN 10
#endif
#ifndef ZS_SDI2_PIN
#define ZS_SDI2_PIN 4
#endif
#ifndef ZS_SCK2_PIN
#define ZS_SCK2_PIN 5
#endif
#ifndef ZS_CS2_PIN
#define ZS_CS2_PIN 3
#endif
#endif
#ifndef ZS_SDT1_PIN
#define ZS_SDT1_PIN 26
#endif
#ifndef ZS_SCK1_PIN
#define ZS_SCK1_PIN 0
#endif
#ifndef ZS_CS1_PIN
#define ZS_CS1_PIN 36
#endif
#ifndef ZS_SDI2_PIN
#define ZS_SDI2_PIN 32
#endif
#ifndef ZS_SCK2_PIN
#define ZS_SCK2_PIN 33
#endif
#ifndef ZS_CS2_PIN
#define ZS_CS2_PIN 36
#endif

constexpr int kBusPins[] = {ZS_SDT1_PIN, ZS_SCK1_PIN, ZS_CS1_PIN,
                           ZS_SDI2_PIN, ZS_SCK2_PIN, ZS_CS2_PIN};
constexpr bool distinct_pins() {
    for (unsigned a = 0; a < 6; ++a)
        for (unsigned b = a + 1; b < 6; ++b)
            if (kBusPins[a] == kBusPins[b] && !(a == 2 && b == 5)) return false;
    return true;
}
static_assert(distinct_pins(), "Only the two CS inputs may share a GPIO");
static_assert(sizeof(ZS_AP_PASSWORD) >= 9 && sizeof(ZS_AP_PASSWORD) <= 64,
              "Access point password must be 8-63 characters");
