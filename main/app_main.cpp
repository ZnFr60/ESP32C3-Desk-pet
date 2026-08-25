// IDF entry point for the migrated (native, no-arduino) pet-robot firmware.
#include "nvs_flash.h"
#include "esp_check.h"

// Defined in esp32-pet-robot-arduino.cpp
void setup();
void loop();

extern "C" void app_main(void) {
    // NVS is required by Preferences (settings / wifi config) and WiFi.
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    setup();
    for (;;) {
        loop();
    }
}
