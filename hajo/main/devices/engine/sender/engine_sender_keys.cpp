#include "engine_sender_internal.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "gpio_define.h"
#include "n2k/n2k_sender.h"
#include "n2k/N2kVarilog.h"

static const char *TAG = "keys";

static TimerHandle_t beepTimer;

static void send_engine_state() {
    tN2kMsg message;
    SetN2kPGNVarilogEngineState(message, engineSenderData.engineInstanceId, engineSenderData.mainOn);
    NMEA2000.SendMsg(message);
}

static void send_ack(const N2kPGNVarilogEngineKeyPress &data) {
    tN2kMsg ackMsg;
    SetN2kPGNVarilogEngineKeyPressAck(ackMsg, data.instanceId, data.sid);
    NMEA2000.SendMsg(ackMsg);
    ESP_LOGI(TAG, "key ack sid %02x sent", data.sid);
}

static void set_output(int index, bool state) {
    gpio_set_pin_state(OUTPUTS, RELAY_CLASS, index, state);
}

static void warning_beep() {
    ESP_LOGW(TAG, "beep-beep");
    set_output(PIN_INDEX_BUZZER, true);
    xTimerStart(beepTimer, portMAX_DELAY);
}

static void init_test_data(bool on) {
    if (on) {
        engineSenderData.engineSpeed = 1725.0;
        engineSenderData.chargerFailure = false;
        engineSenderData.coolingWaterTemperatureFailure = false;
        engineSenderData.oilPressureFailure = true;
    } else {
        engineSenderData.engineSpeed = 0;
        engineSenderData.chargerFailure = false;
        engineSenderData.coolingWaterTemperatureFailure = false;
        engineSenderData.oilPressureFailure = false;
    }
}

static void turn_main_off() {
    ESP_LOGI(TAG, "turning main off");
    engineSenderData.lightOn = engineSenderData.mainOn = false;
    set_output(PIN_INDEX_MAIN, false);
    set_output(PIN_INDEX_LIGHT, false);
    set_output(PIN_INDEX_START, false);
    set_output(PIN_INDEX_STOP, false);
    send_engine_state();
}

void turn_main_on() {
    ESP_LOGI(TAG, "turning main on");
    engineSenderData.mainOn = true;
    set_output(PIN_INDEX_MAIN, true);
    send_engine_state();
}

static void main_pressed() {
    ESP_LOGI(TAG, "main pressed");
    if (engineSenderData.mainOn) {
        if (engineSenderData.engineRunning) {
            ESP_LOGW(TAG, "engine running, won't turn off");
            warning_beep();
        } else {
            turn_main_off();
        }
    } else {
        turn_main_on();
    }
}

static void light_switched() {
    engineSenderData.lightOn = !engineSenderData.lightOn;
    ESP_LOGI(TAG, "light pressed, turning %s", engineSenderData.lightOn ? "on" : "off");
    set_output(PIN_INDEX_LIGHT, engineSenderData.lightOn);
}

static void start_pressed(const N2kPGNVarilogEngineKeyPress &data) {
    ESP_LOGI(TAG, "start %s", data.keysPressed.Keys.start ? "pressed" : "released");
    if (data.keysPressed.Keys.start) {
        if (engineSenderData.engineRunning) {
            ESP_LOGW(TAG, "already running, won't start");
            warning_beep();
        } else if (engineSenderData.stopping || engineSenderData.starting) {
            ESP_LOGW(TAG, "already starting/stopping");
            warning_beep();
        } else {
            engineSenderData.starting = true;
            ESP_LOGI(TAG, "starting");
            set_output(PIN_INDEX_START, true);
        }
    } else {
        ESP_LOGI(TAG, "start ended");
        if (engineSenderData.simulate) {
            init_test_data(true);
        }
        engineSenderData.starting = false;
        set_output(PIN_INDEX_START, false);
        // TODO maybe should be driven by the charging or oil pressure signal
        engineSenderData.engineRunning = true;
    }
}

static void stop_pressed(const N2kPGNVarilogEngineKeyPress &data) {
    ESP_LOGI(TAG, "stop %s", data.keysPressed.Keys.stop ? "pressed" : "released");
    if (data.keysPressed.Keys.stop) {
        if (!engineSenderData.engineRunning) {
            ESP_LOGW(TAG, "not running, won't stop");
            warning_beep();
        } else if (engineSenderData.stopping || engineSenderData.starting) {
            ESP_LOGW(TAG, "already starting/stopping");
            warning_beep();
        } else {
            engineSenderData.stopping = true;
            ESP_LOGI(TAG, "stopping");
            set_output(PIN_INDEX_STOP, true);
        }
    } else {
        ESP_LOGI(TAG, "stop ended");
        engineSenderData.stopping = false;
        set_output(PIN_INDEX_STOP, false);
        if (engineSenderData.simulate) {
            init_test_data(false);
        }
        // TODO maybe should be driven by the charging or oil pressure signal
        engineSenderData.engineRunning = false;
    }
}

static bool is_valid_sid(uint8_t sid) {
    if (engineSenderData.lastSidIsValid && sid == engineSenderData.lastSid) {
        ESP_LOGW(TAG, "repeated key press with sid %02x, skipping", sid);
        return false;
    }
    engineSenderData.lastSid = sid;
    engineSenderData.lastSidIsValid = true;
    return true;
}

void process_engine_key_press(const tN2kMsg &N2kMsg) {
    N2kPGNVarilogEngineKeyPress data;
    if (!ParseN2kPGNVarilogEngineKeyPress(N2kMsg, data)) {
        return;
    }
    ESP_LOGI(TAG, "key press png sid=%02x pressed=%02x changed=%02x",
             data.sid, data.keysPressed.ByteValue, data.keysChanged.ByteValue);
    send_ack(data);
    if (!is_valid_sid(data.sid)) {
        return;
    }
    if (data.keysChanged.Keys.main && data.keysPressed.Keys.main) {
        main_pressed();
        return;
    }
    if (!engineSenderData.mainOn) {
        ESP_LOGW(TAG, "not on, skipping key");
        return;
    }
    if (data.keysChanged.Keys.light && data.keysPressed.Keys.light) {
        light_switched();
    }
    if (data.keysChanged.Keys.start) {
        start_pressed(data);
    }
    if (data.keysChanged.Keys.stop) {
        stop_pressed(data);
    }
}

static void beep_timer_callback(TimerHandle_t xTimer) {
    ESP_LOGI(TAG, "beep ended");
    set_output(PIN_INDEX_BUZZER, false);
}

void setup_keys() {
    beepTimer = xTimerCreate(
        TAG,
        pdMS_TO_TICKS(200),
        false,
        nullptr,
        beep_timer_callback);
}
