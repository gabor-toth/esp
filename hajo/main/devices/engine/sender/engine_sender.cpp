#include "assert_check.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "engine_sender.h"
#include "engine_sender_internal.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "gpio_define.h"
#include "n2k/n2k_sender.h"
#include "n2k/n2k_receiver.h"
#include "n2k/N2kVarilog.h"
#include "nvs_main.h"

static const char *TAG = "sender";

#define RELAY_CLASS       0

#define PIN_INDEX_MAIN      0
#define PIN_INDEX_START     1
#define PIN_INDEX_STOP      2
#define PIN_INDEX_LIGHT     3
#define PIN_INDEX_BUZZER    4

static TimerHandle_t beepTimer;
static int myDeviceIndex;
static uint8_t engineInstanceId;
static uint32_t engineMinutes;
static double engineSpeed;
static bool chargerFailure;
static bool oilPressureFailure;
static bool coolingWaterTemperatureFailure;
static bool hasFailure;
static bool mainOn;
static bool engineRunning;
static bool lightOn;
static bool starting;
static bool stopping;
static bool simulate;

static void process_incoming_pgn(const tN2kMsg &message);

static void setup_n2k_device(int iDev) {
    static constexpr unsigned long TransmitMessages[ ] = {
        N2K_PGN_ENGINE_PARAMETERS_RAPID_UPDATE,
        N2K_PGN_ENGINE_PARAMETERS_DYNAMIC,
        N2K_PGN_VARILOG_ENGINE_KEY_PRESS_ACK,
        N2K_PGN_VARILOG_ENGINE_STATE,
        0
    };

    static constexpr unsigned long ReceiveMessages[ ] = {
        N2K_PGN_VARILOG_ENGINE_KEY_PRESS,
        N2K_PGN_VARILOG_ENGINE_SET_STATE,
        0
    };

    static constexpr tNMEA2000::tProductInformation ProductInformation = {
        2100, // N2kVersion
        103, // Manufacturer's product code
        "Engine sender", // Manufacturer's Model ID
        "1.0.0 (2025-11-22)", // Manufacturer's Software version code
        "1.0.0 (2025-11-22)", // Manufacturer's Model version
        "00000001", // Manufacturer's Model serial code
        0, // CertificationLevel
        1 // LoadEquivalency
    };

    NMEA2000.SetProductInformation(&ProductInformation, iDev);

    NMEA2000.SetDeviceInformation(n2k_get_device_id(), // Unique number. Use e.g. Serial number.
                                  160, // Engine Gateway
                                  50, // Device class=Propulsion
                                  N2K_MANUFACTURER_CODE_VARILOG,
                                  4, // Marine
                                  iDev
    );

    NMEA2000.ExtendTransmitMessages(TransmitMessages, iDev);
    NMEA2000.ExtendReceiveMessages(ReceiveMessages, iDev);
    NMEA2000.AttachMsgHandler(new N2kIncomingMessageHandler(&NMEA2000, process_incoming_pgn));
}

static bool send_rapid_update(int index, tN2kMsg &message, int &deviceIndex) {
    deviceIndex = myDeviceIndex;
    if (index > 0 || !mainOn) {
        return false;
    }
    SetN2kPGN127488(message,
                    engineInstanceId,
                    engineSpeed);
    return true;
}

static double get_failure_value(bool failure) {
    return failure ? N2kDoubleNA : 0.0;
}

static bool send_dynamic(int index, tN2kMsg &message, int &deviceIndex) {
    deviceIndex = myDeviceIndex;
    if (index > 0 || !mainOn) {
        return false;
    }
    SetN2kPGN127489(message,
                    engineInstanceId,
                    get_failure_value(oilPressureFailure),
                    N2kDoubleNA,
                    get_failure_value(coolingWaterTemperatureFailure),
                    get_failure_value(chargerFailure),
                    N2kDoubleNA,
                    static_cast<double>(engineMinutes) / 60.0);
    return true;
}

static void save_hours() {
    uint32_t nvs_handle = nvs_open_storage();
    ESP_ERROR_CHECK(nvs_set_u32( nvs_handle, NV_KEY_ENGINE_MINUTES, engineMinutes ));
    nvs_close_storage(nvs_handle);
}

static void init_data() {
    uint32_t nvs_handle = nvs_open_storage();
    esp_err_t result = nvs_get_u32(nvs_handle, NV_KEY_ENGINE_MINUTES, &engineMinutes);
    if (result == ESP_OK) {
        ESP_LOGI(TAG, "Loaded %s %ld", NV_KEY_ENGINE_MINUTES, engineMinutes);
    } else if (result == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "No %s stored yet, defaulting to 0", NV_KEY_ENGINE_MINUTES);
        engineMinutes = 0;
    } else {
        ESP_ERROR_CHECK(result);
    }

    result = nvs_get_u8(nvs_handle, NV_KEY_ENGINE_MINUTES, &engineInstanceId);
    if (result == ESP_OK) {
        ESP_LOGI(TAG, "Loaded %s %02x", NV_KEY_ENGINE_INSTANCE_ID, engineInstanceId);
    } else if (result == ESP_ERR_NVS_NOT_FOUND) {
        engineInstanceId = 1;
    } else {
        ESP_ERROR_CHECK(result);
    }
    nvs_close_storage(nvs_handle);

    engineSpeed = 0.0;
    chargerFailure = false;
    coolingWaterTemperatureFailure = false;
    oilPressureFailure = false;

    engineRunning = lightOn = mainOn = starting = stopping = false;
}

static void send_ack(const N2kPGNVarilogEngineKeyPress &data) {
    tN2kMsg ackMsg;
    SetN2kPGNVarilogEngineKeyPressAck(ackMsg, data.instanceId, data.sid);
    NMEA2000.SendMsg(ackMsg);
}

static void set_output(int index, bool state) {
    gpio_set_pin_state(OUTPUTS, RELAY_CLASS, index, state);
}

static void beep_timer_callback(TimerHandle_t xTimer) {
    ESP_LOGI(TAG, "beep ended");
    set_output(PIN_INDEX_BUZZER, false);
}

static void warning_beep() {
    ESP_LOGW(TAG, "beep-beep");
    set_output(PIN_INDEX_BUZZER, true);
    xTimerStart(beepTimer, portMAX_DELAY);
}

static void init_test_data() {
    engineMinutes = 6789 * 60;
    engineSpeed = 6789.0;
    chargerFailure = false;
    coolingWaterTemperatureFailure = false;
    oilPressureFailure = true;
}

static void send_engine_state() {
    tN2kMsg message;
    SetN2kPGNVarilogEngineState(message, engineInstanceId, mainOn);
    NMEA2000.SendMsg(message);
}

static void process_engine_key_press(const tN2kMsg &N2kMsg) {
    N2kPGNVarilogEngineKeyPress data;
    if (!ParseN2kPGNVarilogEngineKeyPress(N2kMsg, data)) {
        return;
    }
    ESP_LOGI(TAG, "key press png sid=%02x pressed=%02x changed=%02x",
             data.sid, data.keysPressed.ByteValue, data.keysChanged.ByteValue);
    send_ack(data);
    if (data.keysChanged.Keys.main && data.keysPressed.Keys.main) {
        ESP_LOGI(TAG, "main pressed");
        if (mainOn) {
            if (engineRunning) {
                ESP_LOGW(TAG, "engine running, won't turn off");
                warning_beep();
            } else {
                ESP_LOGI(TAG, "turning main on");
                lightOn = mainOn = false;
                set_output(PIN_INDEX_MAIN, false);
                set_output(PIN_INDEX_LIGHT, false);
                set_output(PIN_INDEX_START, false);
                set_output(PIN_INDEX_STOP, false);
                send_engine_state();
            }
        } else {
            ESP_LOGI(TAG, "turning main on");
            mainOn = true;
            set_output(PIN_INDEX_MAIN, true);
            send_engine_state();
        }
    }
    if (mainOn) {
        if (data.keysChanged.Keys.light && data.keysPressed.Keys.light) {
            lightOn = !lightOn;
            ESP_LOGI(TAG, "light pressed, turning %s", lightOn ? "on" : "off");
            set_output(PIN_INDEX_LIGHT, lightOn);
        }
        if (data.keysChanged.Keys.start) {
            ESP_LOGI(TAG, "start %s", data.keysPressed.Keys.start ? "pressed" : "released");
            if (data.keysPressed.Keys.start) {
                if (stopping || starting) {
                    ESP_LOGW(TAG, "already starting/stopping");
                    warning_beep();
                } else {
                    starting = true;
                    ESP_LOGI(TAG, "starting");
                    set_output(PIN_INDEX_START, true);
                }
            } else {
                ESP_LOGI(TAG, "start ended");
                if (simulate) {
                    init_test_data();
                }
                starting = false;
                set_output(PIN_INDEX_START, false);
            }
        }
        if (data.keysChanged.Keys.stop) {
            ESP_LOGI(TAG, "stop %s", data.keysPressed.Keys.stop ? "pressed" : "released");
            if (data.keysPressed.Keys.stop) {
                if (stopping || starting) {
                    ESP_LOGW(TAG, "already starting/stopping");
                    warning_beep();
                } else {
                    stopping = true;
                    ESP_LOGI(TAG, "stopping");
                    set_output(PIN_INDEX_STOP, true);
                }
            } else {
                ESP_LOGI(TAG, "stop ended");
                stopping = false;
                set_output(PIN_INDEX_STOP, false);
            }
        }
    } else {
        ESP_LOGW(TAG, "not on, skipping key");
    }
}

static void process_engine_set_state(const tN2kMsg &N2kMsg) {
    N2kPGNVarilogEngineState data;
    ParseN2kPGNVarilogEngineSetState(N2kMsg, data);
    ESP_LOGI(TAG, "got engine state from instance %02x state %s", data.instanceId, data.engineOn ? "on" : "off");
}

static void process_incoming_pgn(const tN2kMsg &message) {
    switch (message.PGN) {
        case N2K_PGN_VARILOG_ENGINE_KEY_PRESS:
            process_engine_key_press(message);
            break;
        case N2K_PGN_VARILOG_ENGINE_SET_STATE:
            process_engine_set_state(message);
            break;
        default:
            break;
    }
}

static void define_output_pins(gpio_config_t *io_conf, void *user_context) {
    gpio_add_class(OUTPUTS, "relays", 6, low_is_on);

    ASSERT_CHECK(PIN_INDEX_MAIN == gpio_add_pin(OUTPUTS, RELAY_CLASS, PIN_OUTPUT_MAIN,
        low_is_on, &io_conf->pin_bit_mask));
    ASSERT_CHECK(PIN_INDEX_START== gpio_add_pin(OUTPUTS, RELAY_CLASS, PIN_OUTPUT_START,
        low_is_on, &io_conf->pin_bit_mask));
    ASSERT_CHECK(PIN_INDEX_STOP == gpio_add_pin(OUTPUTS, RELAY_CLASS, PIN_OUTPUT_STOP,
        low_is_on, &io_conf->pin_bit_mask));
    ASSERT_CHECK(PIN_INDEX_LIGHT == gpio_add_pin(OUTPUTS, RELAY_CLASS, PIN_OUTPUT_LIGHT,
        low_is_on, &io_conf->pin_bit_mask));
    ASSERT_CHECK(PIN_INDEX_BUZZER == gpio_add_pin(OUTPUTS, RELAY_CLASS, PIN_OUTPUT_BUZZER,
        low_is_on, &io_conf->pin_bit_mask));
}

static void setup_timers() {
    beepTimer = xTimerCreate(
        TAG,
        pdMS_TO_TICKS(200),
        false,
        nullptr,
        beep_timer_callback);
}

void engine_sender_main(int iDev) {
    myDeviceIndex = iDev;
    setup_timers();
    setup_n2k_device(iDev);

    nk2_register_sender(send_rapid_update, "engine_rapid_update", N2K_PGN_ENGINE_PARAMETERS_RAPID_UPDATE_INTERVAL_MS,
                        75, true);
    nk2_register_sender(send_dynamic, "engine_dynamic", N2K_PGN_ENGINE_PARAMETERS_DYNAMIC_INTERVAL_MS, 80, true);

    gpio_init(nullptr, nullptr, define_output_pins, nullptr);
    init_data();
    simulate = true;
    // init_test_data();
}
