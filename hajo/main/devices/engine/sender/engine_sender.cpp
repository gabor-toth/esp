#include "esp_log.h"
#include "esp_timer.h"
#include "engine_sender.h"
#include "n2k/n2k_sender.h"
#include "n2k/n2k_receiver.h"
#include "n2k/N2kVarilog.h"
#include "nvs_main.h"

static const char *TAG = "sender";

static int myDeviceIndex;
static uint32_t engineMinutes;
static double engineSpeed;
static bool chargerFailure;
static bool oilPressureFailure;
static bool coolingWaterTemperatureFailure;
static bool hasFailure;

static void process_incoming_pgn(const tN2kMsg &message);

static void setup_n2k_device(int iDev) {
    static constexpr unsigned long TransmitMessages[ ] = {
        N2K_PGN_ENGINE_PARAMETERS_RAPID_UPDATE,
        N2K_PGN_ENGINE_PARAMETERS_DYNAMIC,
        N2K_PGN_VARILOG_ENGINE_KEY_PRESS_ACK,
        0
    };

    static constexpr unsigned long ReceiveMessages[ ] = {
        N2K_PGN_VARILOG_ENGINE_KEY_PRESS,
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
    if (index > 0) {
        return false;
    }
    SetN2kPGN127488(message,
                    index + 1,
                    engineSpeed);
    return true;
}

static double get_failure_value(bool failure) {
    return failure ? N2kDoubleNA : 0.0;
}

static bool send_dynamic(int index, tN2kMsg &message, int &deviceIndex) {
    deviceIndex = myDeviceIndex;
    if (index > 0) {
        return false;
    }
    SetN2kPGN127489(message,
                    index + 1,
                    get_failure_value(oilPressureFailure),
                    N2kDoubleNA,
                    get_failure_value(coolingWaterTemperatureFailure),
                    get_failure_value(chargerFailure),
                    N2kDoubleNA,
                    (double) engineMinutes / 60.0);
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
    nvs_close_storage(nvs_handle);

    engineSpeed = 0.0;
    chargerFailure = false;
    coolingWaterTemperatureFailure = false;
    oilPressureFailure = false;
}

static void init_test_data() {
    engineMinutes = 6789 * 60;
    engineSpeed = 6789.0;
    chargerFailure = false;
    coolingWaterTemperatureFailure = false;
    oilPressureFailure = true;
}

static void process_engine_key_press(const tN2kMsg &N2kMsg) {
    N2kPGNVarilogEngineKeyPress data;
    if (!ParseN2kPGNVarilogEngineKeyPress(N2kMsg, data)) {
        return;
    }
    ESP_LOGI(TAG, "key pressed sid=%02x pressed=%02x changed=%02x",
             data.sid, data.keysPressed.ByteValue, data.keysChanged.ByteValue);
    tN2kMsg ackMsg;
    SetN2kPGNVarilogEngineKeyPressAck(ackMsg, data.instanceId, data.sid);
    NMEA2000.SendMsg(ackMsg);
    // TODO implement logic
}

static void process_incoming_pgn(const tN2kMsg &message) {
    switch (message.PGN) {
        case N2K_PGN_VARILOG_ENGINE_KEY_PRESS:
            process_engine_key_press(message);
            break;
        default:
            break;
    }
}

void engine_sender_main(int iDev) {
    myDeviceIndex = iDev;
    setup_n2k_device(iDev);

    nk2_register_sender(send_rapid_update, "engine_rapid_update", N2K_PGN_ENGINE_PARAMETERS_RAPID_UPDATE_INTERVAL_MS,
                        75, true);
    nk2_register_sender(send_dynamic, "engine_dynamic", N2K_PGN_ENGINE_PARAMETERS_DYNAMIC_INTERVAL_MS, 80, true);

    init_data();
    init_test_data();
}
