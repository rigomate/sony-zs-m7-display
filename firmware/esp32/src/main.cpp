#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#if CONFIG_IDF_TARGET_ESP32
#include <driver/adc.h>
#include <driver/rtc_io.h>
#include <esp_adc_cal.h>
#endif
#include <sstream>
#include "board_config.hpp"
#include "esp32_spi_source.hpp"
#include "zs_m7/live_display.hpp"
#include "web_page.hpp"

namespace {
Esp32SpiSource source;
zs_m7::LiveDisplay display;
WebServer server(80);
std::string capture_error;
std::string gpio0_voltage_json = "null";
void probe_gpio0_voltage() {
#if CONFIG_IDF_TARGET_ESP32
    // ADC2 is unavailable during Wi-Fi. Sample once, before Wi-Fi and SPI
    // initialization, then return GPIO0 to digital input operation.
    if (ZS_SCK1_PIN != 0) return;
    esp_adc_cal_characteristics_t calibration{};
    const auto cal = esp_adc_cal_characterize(ADC_UNIT_2, ADC_ATTEN_DB_11,
        ADC_WIDTH_BIT_12, 1100, &calibration);
    unsigned minimum = 4095, maximum = 0, samples = 0, saturated = 0;
    const auto status = adc2_config_channel_atten(ADC2_CHANNEL_1, ADC_ATTEN_DB_11);
    const auto start = micros();
    if (status == ESP_OK) {
        while (static_cast<std::uint32_t>(micros() - start) < 120000) {
            int raw = 0;
            if (adc2_get_raw(ADC2_CHANNEL_1, ADC_WIDTH_BIT_12, &raw) == ESP_OK) {
                minimum = std::min(minimum, static_cast<unsigned>(raw));
                maximum = std::max(maximum, static_cast<unsigned>(raw));
                ++samples;
                if (raw == 4095) ++saturated;
            }
        }
    }
    rtc_gpio_deinit(GPIO_NUM_0);
    pinMode(0, INPUT);
    std::ostringstream out;
    out << "{\"samples\":" << samples
        << ",\"min_raw\":" << minimum << ",\"max_raw\":" << maximum
        << ",\"approx_min_mv\":" << esp_adc_cal_raw_to_voltage(minimum, &calibration)
        << ",\"approx_max_mv\":" << esp_adc_cal_raw_to_voltage(maximum, &calibration)
        << ",\"saturated_samples\":" << saturated
        << ",\"calibration\":" << static_cast<unsigned>(cal) << '}';
    gpio0_voltage_json = out.str();
#endif
}
std::string state_json() {
    auto json = display.json(millis(), source.dropped_frames(), capture_error);
    json.pop_back();
    json += ",\"receiver\":" + source.diagnostics_json() + ",\"gpio0_voltage_at_boot\":" + gpio0_voltage_json + '}';
    return json;
}
}

void setup() {
    Serial.begin(115200);
    // No pull-ups, no output pins, no connection to SDO2. The existing
    // controller continues handling the player while this board listens.
    // GPIO25 shares the external G36 connection on the PLUS. Keep it floating.
#if CONFIG_IDF_TARGET_ESP32
    pinMode(25, INPUT);
#endif
    for (const int pin : kBusPins) pinMode(pin, INPUT);
    probe_gpio0_voltage();
    const auto err = source.begin();
    if (err != ESP_OK) {
        capture_error = esp_err_to_name(err);
        Serial.printf("Capture initialization failed: %s\n", capture_error.c_str());
    }
    WiFi.mode(WIFI_AP_STA);
    WiFi.setSleep(false);
    if (!WiFi.softAP(ZS_AP_SSID, ZS_AP_PASSWORD)) Serial.println("Access point failed to start");
    Serial.printf("Connect to %s; dashboard: http://%s/\n", ZS_AP_SSID,
                  WiFi.softAPIP().toString().c_str());
    if (ZS_WIFI_SSID[0]) {
        WiFi.setAutoReconnect(true);
        WiFi.begin(ZS_WIFI_SSID, ZS_WIFI_PASSWORD);
    }
    if (MDNS.begin("zs-m7")) MDNS.addService("http", "tcp", 80);
    server.on("/", HTTP_GET, [] {
        server.sendHeader("Cache-Control", "no-store");
        server.send_P(200, "text/html; charset=utf-8", kWebPage);
    });
    server.on("/api/state", HTTP_GET, [] {
        const auto json = state_json();
        server.sendHeader("Cache-Control", "no-store");
        server.send(200, "application/json", json.c_str());
    });
#if CONFIG_IDF_TARGET_ESP32C3
    server.on("/api/capture/pause", HTTP_POST, [] {
        source.pause();
        server.send(200, "application/json", state_json().c_str());
    });
    server.on("/api/capture/restart", HTTP_POST, [] {
        server.send(200, "application/json", "{\"restarting\":true}");
        delay(100);
        ESP.restart();
    });
#endif
    server.onNotFound([] { server.send(404, "text/plain", "Not found"); });
    server.begin();
}

void loop() {
    static zs_m7::SpiFrame frame;
    for (unsigned budget = 0; budget < 64 && source.read_next(frame); ++budget) {
        display.consume(frame, static_cast<std::uint32_t>(frame.end_time_seconds * 1000.0));
    }
    source.sample_gpio_levels();
    server.handleClient();
    static bool was_connected = false;
    const bool connected = WiFi.status() == WL_CONNECTED;
    if (connected && !was_connected)
        Serial.printf("Router dashboard: http://%s/ (also http://zs-m7.local/)\n",
                      WiFi.localIP().toString().c_str());
    was_connected = connected;
    static std::uint32_t last_report_ms = 0;
    const auto now = millis();
    if (static_cast<std::uint32_t>(now - last_report_ms) >= 5000) {
        last_report_ms = now;
        Serial.println(state_json().c_str());
    }
    delay(1);
}
