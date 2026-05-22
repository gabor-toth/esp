#include "assert_check.h"
#include "driver/pulse_cnt.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "engine_sender.h"
#include "engine_sender_internal.h"
#include "freertos/FreeRTOS.h"
#include "gpio_define.h"
#include "n2k/n2k_receiver.h"
#include "n2k/n2k_sender.h"
#include "n2k/N2kVarilog.h"
#include "nvs_main.h"

static const char *TAG = "sender";

static int myDeviceIndex;
static uint32_t engineMinutes;
static uint8_t ticksPerRevolution;

EngineSenderData engineSenderData;

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
    if (engineSenderData.engineSpeed >= MINIMAL_ENGINE_RPM) {
        if (!engineSenderData.mainOn) {
            turn_main_on();
        }
        engineSenderData.engineRunning = true;
    }
    if (index > 0 || !engineSenderData.mainOn) {
        return false;
    }
    SetN2kPGN127488(message,
                    engineSenderData.engineInstanceId,
                    engineSenderData.engineSpeed);
    return true;
}

static double get_failure_value(bool failure) {
    return failure ? N2kDoubleNA : 0.0;
}

static bool send_dynamic(int index, tN2kMsg &message, int &deviceIndex) {
    deviceIndex = myDeviceIndex;
    if (index > 0 || !engineSenderData.mainOn) {
        return false;
    }
    SetN2kPGN127489(message,
                    engineSenderData.engineInstanceId,
                    get_failure_value(engineSenderData.oilPressureFailure),
                    N2kDoubleNA,
                    get_failure_value(engineSenderData.coolingWaterTemperatureFailure),
                    get_failure_value(engineSenderData.chargerFailure),
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
        engineMinutes = 0;
        ESP_LOGI(TAG, "Initialized %s %ld", NV_KEY_ENGINE_MINUTES, engineMinutes);
    } else {
        ESP_ERROR_CHECK(result);
    }

    result = nvs_get_u8(nvs_handle, NV_KEY_ENGINE_MINUTES, &engineSenderData.engineInstanceId);
    if (result == ESP_OK) {
        ESP_LOGI(TAG, "Loaded %s %02x", NV_KEY_ENGINE_INSTANCE_ID, engineSenderData.engineInstanceId);
    } else if (result == ESP_ERR_NVS_NOT_FOUND) {
        engineSenderData.engineInstanceId = 1;
        ESP_LOGI(TAG, "Initialized %s %02x", NV_KEY_ENGINE_INSTANCE_ID, engineSenderData.engineInstanceId);
    } else {
        ESP_ERROR_CHECK(result);
    }

    result = nvs_get_u8(nvs_handle, NV_KEY_ENGINE_TICKS_PER_REVOLUTION, &ticksPerRevolution);
    if (result == ESP_OK) {
        ESP_LOGI(TAG, "Loaded %s %02x", NV_KEY_ENGINE_TICKS_PER_REVOLUTION, ticksPerRevolution);
    } else if (result == ESP_ERR_NVS_NOT_FOUND) {
        ticksPerRevolution = 97;
        ESP_LOGI(TAG, "Initialized %s %d", NV_KEY_ENGINE_TICKS_PER_REVOLUTION, ticksPerRevolution);
    } else {
        ESP_ERROR_CHECK(result);
    }

    nvs_close_storage(nvs_handle);

    engineSenderData.engineSpeed = 0.0;
    engineSenderData.chargerFailure = false;
    engineSenderData.coolingWaterTemperatureFailure = false;
    engineSenderData.oilPressureFailure = false;

    engineSenderData.engineRunning =
            engineSenderData.lightOn =
            engineSenderData.mainOn =
            engineSenderData.starting =
            engineSenderData.stopping = false;

    if (engineSenderData.simulate) {
        engineMinutes = 6789 * 60;
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

static void define_input_pins(gpio_config_t *io_conf, void *user_context) {
    gpio_add_class(INPUTS, "sensor", 4, low_is_on);

    ASSERT_CHECK(PIN_INDEX_OIL_SENSOR == gpio_add_pin(INPUTS, SENSOR_CLASS, PIN_INPUT_OIL_SENSOR,
        inherit, &io_conf->pin_bit_mask));
    ASSERT_CHECK(PIN_INDEX_TEMP_SENSOR == gpio_add_pin(INPUTS, SENSOR_CLASS, PIN_INPUT_TEMP_SENSOR,
        inherit, &io_conf->pin_bit_mask));
    ASSERT_CHECK(PIN_INDEX_CHARGE_SENSOR == gpio_add_pin(INPUTS, SENSOR_CLASS, PIN_INPUT_CHARGE_SENSOR,
        inherit, &io_conf->pin_bit_mask));
}

static void define_output_pins(gpio_config_t *io_conf, void *user_context) {
    gpio_add_class(OUTPUTS, "relay", 6, high_is_on);

    ASSERT_CHECK(PIN_INDEX_MAIN == gpio_add_pin(OUTPUTS, RELAY_CLASS, PIN_OUTPUT_MAIN,
        inherit, &io_conf->pin_bit_mask));
    ASSERT_CHECK(PIN_INDEX_START== gpio_add_pin(OUTPUTS, RELAY_CLASS, PIN_OUTPUT_START,
        inherit, &io_conf->pin_bit_mask));
    ASSERT_CHECK(PIN_INDEX_STOP == gpio_add_pin(OUTPUTS, RELAY_CLASS, PIN_OUTPUT_STOP,
        inherit, &io_conf->pin_bit_mask));
    ASSERT_CHECK(PIN_INDEX_LIGHT == gpio_add_pin(OUTPUTS, RELAY_CLASS, PIN_OUTPUT_LIGHT,
        inherit, &io_conf->pin_bit_mask));
    ASSERT_CHECK(PIN_INDEX_BUZZER_ENABLE == gpio_add_pin(OUTPUTS, RELAY_CLASS, PIN_OUTPUT_BUZZER_ENABLE,
        inherit, &io_conf->pin_bit_mask));
    ASSERT_CHECK(PIN_INDEX_BUZZER_SOUND == gpio_add_pin(OUTPUTS, RELAY_CLASS, PIN_OUTPUT_BUZZER_SOUND,
        inherit, &io_conf->pin_bit_mask));
}

static void gpio_changed_callback(gpio_num_t io_num, int state) {
    state = !state;
    switch (io_num) {
        case PIN_INPUT_OIL_SENSOR:
            ESP_LOGI(TAG, "Failure oil %d", state);
            engineSenderData.oilPressureFailure = state;
            break;
        case PIN_INPUT_TEMP_SENSOR:
            ESP_LOGI(TAG, "Failure temp %d", state);
            engineSenderData.coolingWaterTemperatureFailure = state;
            break;
        case PIN_INPUT_CHARGE_SENSOR:
            ESP_LOGI(TAG, "Failure charge %d", state);
            engineSenderData.chargerFailure = state;
            break;
        default:
            ESP_LOGW(TAG, "Unknown input gpio %d", io_num);
            break;
    }
}

static void setup_failure_state() {
    engineSenderData.oilPressureFailure = gpio_get_pin_state(true, SENSOR_CLASS, PIN_INDEX_OIL_SENSOR);
    engineSenderData.coolingWaterTemperatureFailure = gpio_get_pin_state(true, SENSOR_CLASS, PIN_INDEX_TEMP_SENSOR);
    engineSenderData.chargerFailure = gpio_get_pin_state(true, SENSOR_CLASS, PIN_INDEX_CHARGE_SENSOR);
}

void engine_sender_main(int iDev) {
    // simulate = true;
    engineSenderData.lastSidIsValid = false;

    init_data();

    myDeviceIndex = iDev;
    setup_keys();
    setup_rpm(ticksPerRevolution, &engineSenderData.engineSpeed);
    setup_n2k_device(iDev);

    nk2_register_sender(send_rapid_update, "engine_rapid_update", N2K_PGN_ENGINE_PARAMETERS_RAPID_UPDATE_INTERVAL_MS,
                        75, true);
    nk2_register_sender(send_dynamic, "engine_dynamic", N2K_PGN_ENGINE_PARAMETERS_DYNAMIC_INTERVAL_MS,
                        80, true);

    gpio_init(nullptr, define_input_pins, define_output_pins, gpio_changed_callback);
    setup_failure_state();
}
