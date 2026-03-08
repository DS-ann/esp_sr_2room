#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/dac.h"
#include "esp_sr.h"
#include "esp_sr_models.h"

#define ROOM1_LIGHT  GPIO_NUM_13
#define ROOM1_FAN    GPIO_NUM_4
#define ROOM2_LIGHT  GPIO_NUM_19
#define ROOM2_FAN    GPIO_NUM_21
#define NIGHT_BULB   GPIO_NUM_5

#define SPEAKER_DAC  DAC_CHANNEL_1  // TTS output

static const char* TAG = "ESP_SR";

unsigned long activeStartTime = 0;
const unsigned long ACTIVE_DURATION = 5 * 60 * 1000; // 5 min
bool isActive = false;

void process_command(const char* cmd) {
    // Room 1 devices
    if(strcmp(cmd, "room 1 light on") == 0) gpio_set_level(ROOM1_LIGHT, 0);
    if(strcmp(cmd, "room 1 light off") == 0) gpio_set_level(ROOM1_LIGHT, 1);
    if(strcmp(cmd, "room 1 fan on") == 0) gpio_set_level(ROOM1_FAN, 0);
    if(strcmp(cmd, "room 1 fan off") == 0) gpio_set_level(ROOM1_FAN, 1);

    // Room 2 devices
    if(strcmp(cmd, "room 2 light on") == 0) gpio_set_level(ROOM2_LIGHT, 0);
    if(strcmp(cmd, "room 2 light off") == 0) gpio_set_level(ROOM2_LIGHT, 1);
    if(strcmp(cmd, "room 2 fan on") == 0) gpio_set_level(ROOM2_FAN, 0);
    if(strcmp(cmd, "room 2 fan off") == 0) gpio_set_level(ROOM2_FAN, 1);

    // Night bulb
    if(strcmp(cmd, "night bulb on") == 0) gpio_set_level(NIGHT_BULB, 0);
    if(strcmp(cmd, "night bulb off") == 0) gpio_set_level(NIGHT_BULB, 1);

    // Multi-step commands
    if(strcmp(cmd, "turn on room 1 light and fan") == 0) {
        gpio_set_level(ROOM1_LIGHT, 0);
        gpio_set_level(ROOM1_FAN, 0);
    }
    if(strcmp(cmd, "turn off room 1 light and fan") == 0) {
        gpio_set_level(ROOM1_LIGHT, 1);
        gpio_set_level(ROOM1_FAN, 1);
    }
    if(strcmp(cmd, "turn on room 2 light and fan") == 0) {
        gpio_set_level(ROOM2_LIGHT, 0);
        gpio_set_level(ROOM2_FAN, 0);
    }
    if(strcmp(cmd, "turn off room 2 light and fan") == 0) {
        gpio_set_level(ROOM2_LIGHT, 1);
        gpio_set_level(ROOM2_FAN, 1);
    }
}

void app_main(void) {
    // Initialize GPIOs
    gpio_reset_pin(ROOM1_LIGHT); gpio_set_direction(ROOM1_LIGHT, GPIO_MODE_OUTPUT);
    gpio_reset_pin(ROOM1_FAN);   gpio_set_direction(ROOM1_FAN, GPIO_MODE_OUTPUT);
    gpio_reset_pin(ROOM2_LIGHT); gpio_set_direction(ROOM2_LIGHT, GPIO_MODE_OUTPUT);
    gpio_reset_pin(ROOM2_FAN);   gpio_set_direction(ROOM2_FAN, GPIO_MODE_OUTPUT);
    gpio_reset_pin(NIGHT_BULB);  gpio_set_direction(NIGHT_BULB, GPIO_MODE_OUTPUT);

    // Turn all devices OFF initially (active LOW)
    gpio_set_level(ROOM1_LIGHT, 1);
    gpio_set_level(ROOM1_FAN, 1);
    gpio_set_level(ROOM2_LIGHT, 1);
    gpio_set_level(ROOM2_FAN, 1);
    gpio_set_level(NIGHT_BULB, 1);

    // Enable DAC for TTS
    dac_output_enable(SPEAKER_DAC);

    // Initialize ESP-SR
    esp_sr_init();
    esp_sr_wakenet_init();    // Wake word detection
    esp_sr_multinet_init();   // Command recognition

    ESP_LOGI(TAG, "ESP32 ready. Listening for wake word...");

    while(1) {
        // Check wake word
        if(esp_sr_wakenet_detect()) {
            ESP_LOGI(TAG, "Wake word detected!");
            // TODO: Play TTS "Welcome to Ranjana's home" via DAC
            isActive = true;
            activeStartTime = esp_timer_get_time() / 1000; // milliseconds
        }

        // If active, check for commands
        if(isActive) {
            const char* cmd = esp_sr_multinet_recognize();
            if(cmd != NULL) {
                ESP_LOGI(TAG, "Command recognized: %s", cmd);

                // Room prompts
                if(strstr(cmd, "room 1") != NULL) {
                    ESP_LOGI(TAG, "Listening Room 1");
                    // TODO: play TTS "Listening Room 1"
                }
                if(strstr(cmd, "room 2") != NULL) {
                    ESP_LOGI(TAG, "Listening Room 2");
                    // TODO: play TTS "Listening Room 2"
                }

                process_command(cmd);
            }
        }

        // Deactivate after 5 minutes
        if(isActive && (esp_timer_get_time() / 1000 - activeStartTime > ACTIVE_DURATION)) {
            isActive = false;
            ESP_LOGI(TAG, "5 minutes passed. Back to wake word mode...");
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}