#include "driver/gpio.h"
#include "engine_display.h"
#include "engine_display_internal.h"
#include "engine_display_draw.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "gpio_define.h"
#include "n2k/n2k_receiver.h"
#include "n2k/n2k_sender.h"
#include "n2k/N2kVarilog.h"

static const char *TAG = "display";

#define BUTTONS_CLASS       0
#define KEY_ACK_TIMEOUT     100
#define KEY_ACK_RETRY       10

static int myDeviceIndex;
static TimerHandle_t flashTimer;
static TimerHandle_t engineIdleTimer;
static TimerHandle_t displayLogoTimer;
static TimerHandle_t displayOffTimer;
static TimerHandle_t keyAckTimer;
static bool displayLogo;
static bool turningOff;
static uint8_t sid;
static uint8_t sid_last_acked;
static N2kVarilogEngineKeys keysState;
static N2kVarilogEngineKeys keysStateAcked;
static N2kPGNVarilogEngineKeyPress lastDataSent;
display_data_t displayData;
static int8_t ackCounter;

static void process_incoming_pgn(const tN2kMsg &message);

static void send_key_png(bool initial);

static void turn_display_onoff_with_logo(bool on);

static void setup_n2k_device(int iDev) {
    static constexpr unsigned long TransmitMessages[ ] = {
        N2K_PGN_VARILOG_ENGINE_KEY_PRESS,
        N2K_PGN_VARILOG_ENGINE_SET_STATE,
        0
    };

    static constexpr unsigned long ReceiveMessages[ ] = {
        N2K_PGN_ENGINE_PARAMETERS_RAPID_UPDATE,
        N2K_PGN_ENGINE_PARAMETERS_DYNAMIC,
        N2K_PGN_VARILOG_ENGINE_KEY_PRESS_ACK,
        N2K_PGN_VARILOG_ENGINE_STATE,
        0
    };

    static constexpr tNMEA2000::tProductInformation ProductInformation = {
        2100, // N2kVersion
        107, // Manufacturer's product code
        "Engine display", // Manufacturer's Model ID
        "1.0.0 (2025-11-22)", // Manufacturer's Software version code
        "1.0.0 (2025-11-22)", // Manufacturer's Model version
        "00000001", // Manufacturer's Model serial code
        0, // CertificationLevel
        1 // LoadEquivalency
    };

    NMEA2000.SetProductInformation(&ProductInformation, iDev);

    NMEA2000.SetDeviceInformation(n2k_get_device_id(), // Unique number. Use e.g. Serial number.
                                  130, // Display
                                  120, // Device class=Display
                                  N2K_MANUFACTURER_CODE_VARILOG,
                                  4, // Marine
                                  iDev
    );
    NMEA2000.ExtendTransmitMessages(TransmitMessages, iDev);
    NMEA2000.ExtendReceiveMessages(ReceiveMessages, iDev);
    NMEA2000.AttachMsgHandler(new N2kIncomingMessageHandler(&NMEA2000, process_incoming_pgn));
}

static void draw_screen_on_change() {
    if (!displayLogo && engine_display_is_on) {
        engine_display_draw_screen("draw_screen_on_change");
    }
}

static void process_engine_rapid_pgn(const tN2kMsg &N2kMsg) {
    N2kEngineParamRapid data;
    if (!ParseN2kEngineParamRapid(N2kMsg, data)) {
        return;
    }
    bool changed = false;
    n2k_incoming_value(static_cast<int16_t>(data.engineSpeedRpm), displayData.rpm, changed);
    if (!engine_display_is_on) {
        ESP_LOGW(TAG, "turning on display on engine_rapid_pgn");
        xTimerStop(displayLogoTimer, portMAX_DELAY);
        engine_display_onoff(true);
    }
    if (changed) {
        draw_screen_on_change();
    }
}

static void set_flash() {
    ESP_LOGI(TAG, "failure state oil=%d cooling=%d charger=%d",
             displayData.oilPressureFailure,
             displayData.coolingWaterTemperatureFailure,
             displayData.chargerFailure);
    const bool hasFailure = displayData.oilPressureFailure
                            || displayData.coolingWaterTemperatureFailure
                            || displayData.chargerFailure;
    if (hasFailure != displayData.hasFailure) {
        if (hasFailure) {
            // turn flash on
            displayData.flashState = true;
            xTimerStart(flashTimer, portMAX_DELAY);
        } else {
            // turn flash off
            xTimerStop(flashTimer, portMAX_DELAY);
            displayData.flashState = false;
        }
        ESP_LOGW(TAG, "flash state %d", displayData.flashState);
        displayData.hasFailure = hasFailure;
    }
}

static void process_engine_key_press(const tN2kMsg &N2kMsg) {
    N2kEngineDynamicParam data;
    if (!ParseN2kEngineDynamicParam(N2kMsg, data)) {
        return;
    }
    bool changed = false;
    n2k_incoming_value(static_cast<int16_t>(data.engineHours), displayData.hours, changed);
    n2k_incoming_value(data.engineOilPress < 0.0, displayData.oilPressureFailure, changed);
    n2k_incoming_value(data.engineCoolantTemp < 0.0, displayData.coolingWaterTemperatureFailure, changed);
    n2k_incoming_value(data.alternatorVoltage < 0.0, displayData.chargerFailure, changed);
    if (changed) {
        set_flash();
        draw_screen_on_change();
    }
}

static void reset_idle_timer() {
    xTimerStop(displayOffTimer, portMAX_DELAY);
    xTimerReset(engineIdleTimer, portMAX_DELAY);
}

static void process_keypress_ack_pgn(const tN2kMsg &N2kMsg) {
    N2kPGNVarilogEngineKeyPressAck data;
    ParseN2kPGNVarilogEngineKeyPressAck(N2kMsg, data);
    ESP_LOGI(TAG, "got key ack for instance %02x sid %02x", data.instanceId, data.sid);
    keysStateAcked.ByteValue = keysState.ByteValue;
    xTimerStop(keyAckTimer, portMAX_DELAY);
}

static void process_engine_state_pgn(const tN2kMsg &N2kMsg) {
    N2kPGNVarilogEngineState data;
    ParseN2kPGNVarilogEngineState(N2kMsg, data);
    ESP_LOGI(TAG, "got engine state from instance %02x state %s", data.instanceId, data.engineOn ? "on" : "off");
    if (!data.engineOn) {
        turn_display_onoff_with_logo(false);
    }
}

static void process_incoming_pgn(const tN2kMsg &message) {
    switch (message.PGN) {
        case N2K_PGN_ENGINE_PARAMETERS_RAPID_UPDATE:
            process_engine_rapid_pgn(message);
            reset_idle_timer();
            break;
        case N2K_PGN_ENGINE_PARAMETERS_DYNAMIC:
            process_engine_key_press(message);
            reset_idle_timer();
            break;
        case N2K_PGN_VARILOG_ENGINE_KEY_PRESS_ACK:
            process_keypress_ack_pgn(message);
            break;
        case N2K_PGN_VARILOG_ENGINE_STATE:
            process_engine_state_pgn(message);
            break;
        default:
            break;
    }
}

static void send_test_pngs() {
    tN2kMsg messageRapid;
    SetN2kPGN127488(messageRapid, 1, 6789);
    process_incoming_pgn(messageRapid);

    tN2kMsg messageDynamic;
    SetN2kPGN127489(messageDynamic, 1, N2kDoubleNA, 0.0, 0.0, 0.0, N2kDoubleNA, 6789.0);
    process_incoming_pgn(messageDynamic);
}

void set_initial_display_data() {
    displayData.rpm = displayData.hours = static_cast<int16_t>(N2kDoubleNA);
    displayData.chargerFailure = displayData.oilPressureFailure = displayData.coolingWaterTemperatureFailure = false;
}

static void flash_timer_callback(TimerHandle_t xTimer) {
    displayData.flashState = !displayData.flashState;
    engine_display_draw_screen("flash_timer_callback");
}

static void engine_idle_timer_callback(TimerHandle_t xTimer) {
    ESP_LOGW(TAG, "no engine pngs received in %d secs", CONFIG_ENGINE_DISPLAY_IDLE_TIMEOUT_SECS);
    set_initial_display_data();
    set_flash();
    engine_display_draw_screen("idle_timer_callback");
    xTimerStop(engineIdleTimer, portMAX_DELAY);
    xTimerStart(displayOffTimer, portMAX_DELAY);
}

static void stop_all_timers() {
    xTimerStop(engineIdleTimer, portMAX_DELAY);
    xTimerStop(displayLogoTimer, portMAX_DELAY);
    xTimerStop(displayOffTimer, portMAX_DELAY);
    xTimerStop(flashTimer, portMAX_DELAY);
}

static void off_timer_callback(TimerHandle_t xTimer) {
    stop_all_timers();
    engine_display_onoff(false);
    // TODO beep
}

static void logo_timer_callback(TimerHandle_t xTimer) {
    displayLogo = false;
    engine_display_draw_screen("logo_timer_callback");
    if (turningOff) {
        off_timer_callback(xTimer);
    } else {
        if (displayData.hasFailure) {
            xTimerStart(flashTimer, portMAX_DELAY);
        }
    }
}

static void key_ack_timer_callback(TimerHandle_t xTimer) {
    if (ackCounter != 0 && --ackCounter > 0) {
        send_key_png(false);
    } else {
        xTimerStop(keyAckTimer, portMAX_DELAY);
        ESP_LOGW(TAG, "No key ack");
    }
}

static void turn_display_onoff_with_logo(bool on) {
    if (on) {
        engine_display_onoff(true);
    } else {
        stop_all_timers();
    }
    turningOff = !on;
    engine_display_draw_logo();
    displayLogo = true;
    xTimerStart(displayLogoTimer, portMAX_DELAY);
}

static void send_key_png(bool initial) {
    tN2kMsg msg;
    SetN2kPGNVarilogEngineKeyPress(msg, lastDataSent);
    NMEA2000.SendMsg(msg);
    ESP_LOGI(TAG, "key png %ssent sid %02x", initial ? "" : "re", lastDataSent.sid);
}

static void gpio_changed(gpio_num_t io_num, int state) {
    ESP_LOGI(TAG, "gpio %d state %d", io_num, state);
    xTimerReset(displayOffTimer, portMAX_DELAY);
    state = !state;
    switch (io_num) {
        case PIN_INPUT_LIGHT:
            keysState.Keys.light = state;
            break;
        case PIN_INPUT_START:
            keysState.Keys.start = state;
            break;
        case PIN_INPUT_STOP:
            keysState.Keys.stop = state;
            break;
        case PIN_INPUT_ONOFF:
            keysState.Keys.main = state;
            if (!engine_display_is_on) {
                turn_display_onoff_with_logo(true);
            }
            break;
        default:
            break;
    }
    N2kVarilogEngineKeys keysChangedSinceLastAck;
    keysChangedSinceLastAck.ByteValue = keysState.ByteValue ^ keysStateAcked.ByteValue;
    lastDataSent.instanceId = 1;
    lastDataSent.sid = ++sid;
    lastDataSent.keysPressed = keysState;
    lastDataSent.keysChanged = keysChangedSinceLastAck;
    ackCounter = KEY_ACK_RETRY;
    send_key_png(true);
    xTimerReset(keyAckTimer, portMAX_DELAY);
}

static void setup_display(int iDev) {
    gpio_set_level(PIN_LCD_BACKLIGHT, false);
    gpio_set_level(PIN_BUTTON_BACKLIGHT, false);

    gpio_config_t gpioConfig;
    gpioConfig.pin_bit_mask = (1L << PIN_LCD_BACKLIGHT) | (1L << PIN_BUTTON_BACKLIGHT);
    gpioConfig.mode = GPIO_MODE_OUTPUT;
    gpioConfig.pull_up_en = GPIO_PULLUP_DISABLE;
    gpioConfig.pull_down_en = GPIO_PULLDOWN_ENABLE;
    gpioConfig.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&gpioConfig);

    myDeviceIndex = iDev;
    setup_n2k_device(iDev);

    set_initial_display_data();
    displayData.hasFailure = false;

    engine_display_setup_display();
}

static void setup_timers() {
    flashTimer = xTimerCreate(
        TAG,
        pdMS_TO_TICKS(500),
        true,
        nullptr,
        flash_timer_callback);
    engineIdleTimer = xTimerCreate(
        TAG,
        pdMS_TO_TICKS(CONFIG_ENGINE_DISPLAY_IDLE_TIMEOUT_SECS * 1000),
        true,
        nullptr,
        engine_idle_timer_callback);
    displayLogoTimer = xTimerCreate(
        TAG,
        pdMS_TO_TICKS(CONFIG_ENGINE_DISPLAY_LOGO_SECS * 1000),
        false,
        nullptr,
        logo_timer_callback);
    displayOffTimer = xTimerCreate(
        TAG,
        pdMS_TO_TICKS(CONFIG_ENGINE_DISPLAY_OFF_TIMEOUT_SECS * 1000),
        true,
        nullptr,
        off_timer_callback);
    keyAckTimer = xTimerCreate(
        TAG,
        pdMS_TO_TICKS(KEY_ACK_TIMEOUT),
        true,
        nullptr,
        key_ack_timer_callback);
}

static void define_input_pins(gpio_config_t *io_conf, void *user_context) {
    gpio_add_class(INPUTS, "buttons", 4, low_is_on);

    gpio_add_pin(INPUTS, BUTTONS_CLASS, PIN_INPUT_ONOFF,
                 low_is_on, &io_conf->pin_bit_mask);
    gpio_add_pin(INPUTS, BUTTONS_CLASS, PIN_INPUT_START,
                 low_is_on, &io_conf->pin_bit_mask);
    gpio_add_pin(INPUTS, BUTTONS_CLASS, PIN_INPUT_STOP,
                 low_is_on, &io_conf->pin_bit_mask);
    gpio_add_pin(INPUTS, BUTTONS_CLASS, PIN_INPUT_LIGHT,
                 low_is_on, &io_conf->pin_bit_mask);
}

void engine_display_main(int iDev) {
    setup_display(iDev);
    setup_timers();
    keysState.ByteValue = 0;
    keysStateAcked.ByteValue = 0;
    sid = sid_last_acked = 0;
    gpio_init(nullptr, define_input_pins, nullptr, gpio_changed);
}

void engine_display_test() {
    engine_display_setup_display();
    displayData.rpm = 6789;
    displayData.hours = 6789;
    displayData.chargerFailure = false;
    displayData.oilPressureFailure = true;
    displayData.coolingWaterTemperatureFailure = false;
    engine_display_draw_screen("engine_display_test");
}
