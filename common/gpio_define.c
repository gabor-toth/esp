#include "config_version.h"
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "gpio_define.h"
#include "gpio_json.h"
#include "gpio_task.h"
#include "nvs_main.h"
#include <string.h>

#include "portmacro.h"
#include "esp_private/gpio.h"

static const char *LOG_TAG = "gpio";

#define ESP_INTR_FLAG_DEFAULT 0

#define reached( X ) ((X)!=0)

#define PUMP_MAIN     0
#define PUMP_REFILL   1

#define MAX_PIN_CLASSES 4

#define NVS_KEY_PREFIX  "gpio."
#define NVS_KEY_VERSION  NVS_KEY_PREFIX "version"

static nvs_handle_t nvs_storage_handle;
static ConfigVersion version;

typedef struct {
    gpio_num_t pin;
    char *name;
    PinLevelType level_type;
    bool state;
    bool is_manual;
    int delay_ms_going_low;
    int delay_ms_going_high;
} Pin;

typedef struct {
    const char *name;
    int max_pin_count;
    int used_pin_count;
    Pin *pins;
    PinLevelType level_type;
} PinClass;

typedef struct {
    PinClass classes[ MAX_PIN_CLASSES ];
    int used_classes;
} PinClasses;

static PinClasses pin_definitions[ 2 ];

static void get_nvs_key(bool is_input, int class_id, int index, char *buffer, size_t buffer_size) {
    snprintf(buffer, buffer_size, "%s.%d", gpio_get_class_name(is_input, class_id), index + 1);
}

static bool is_valid_class(bool is_input, int class_id) {
    if (class_id >= 0 && class_id < pin_definitions[ is_input ].used_classes) {
        return true;
    }
    ESP_LOGE(LOG_TAG, "Wrong pin class_id %d/%d >= %d/%d",
             is_input, class_id,
             sizeof( pin_definitions ) / sizeof( pin_definitions[ 0 ] ), pin_definitions[ is_input ].used_classes);
    return false;
}

bool gpio_is_valid_index(bool is_input, int class_id, int index) {
    if (!is_valid_class(is_input, class_id)) {
        return false;
    }
    int max_pin_count = pin_definitions[ is_input ].classes[ class_id ].max_pin_count;
    if (index >= 0
        && index < max_pin_count) {
        return true;
    }
    ESP_LOGE(LOG_TAG, "Wrong pin index %d/%d/%d >= %d",
             is_input, class_id, index,
             max_pin_count
    );
    return false;
}

int gpio_add_class(bool is_input, const char *name, int max_pin_count, PinLevelType level_type) {
    PinClasses *pin_classes = &pin_definitions[ is_input ];
    ESP_LOGI(LOG_TAG, "Adding pin class_id %d/%d \"%s\"", is_input, pin_classes->used_classes, name);
    if (pin_classes->used_classes == MAX_PIN_CLASSES) {
        ESP_LOGE(LOG_TAG, "Too many pin classes on definition %d \"%s\"", is_input, name);
        return -1;
    }
    int class_id = pin_classes->used_classes++;
    PinClass *pin_class = &pin_classes->classes[ class_id ];
    pin_class->name = name;
    pin_class->max_pin_count = max_pin_count;
    pin_class->used_pin_count = 0;
    pin_class->level_type = level_type;
    pin_class->pins = malloc(sizeof(Pin) * max_pin_count);
    return class_id;
}

static void set_pin_state(Pin *output_pin, bool enabled) {
    output_pin->state = enabled;
    if (!enabled && (output_pin->level_type == high_is_on_float_off || output_pin->level_type == low_is_on_float_off)) {
        gpio_output_disable(output_pin->pin);
    } else {
        bool level =
                (enabled == (output_pin->level_type == high_is_on || output_pin->level_type == high_is_on_float_off)) ||
                (!enabled == (output_pin->level_type == low_is_on || output_pin->level_type == low_is_on_float_off));
        ESP_LOGI(LOG_TAG, "set pin %d to %d", output_pin->pin, level);
        gpio_set_level(output_pin->pin, level);
        gpio_output_enable(output_pin->pin);
    }
}

static void read_nvs_or_default(bool is_input, int class_id, int index, PinData *pin_data) {
    char nvs_key[ 256 ];

    memset(pin_data, 0, sizeof(PinData));
    get_nvs_key(is_input, class_id, index, nvs_key, sizeof(nvs_key));
    char *json_string = nvs_read_string(nvs_storage_handle, nvs_key);
    if (json_string == NULL) {
        pin_data->name = strdup(nvs_key);
        return;
    }

    gpio_data_from_json_string(json_string, pin_data);
    free(json_string);
}

int gpio_add_pin(bool is_input, int class_id, gpio_num_t gpio_pin, PinLevelType level_type, uint64_t *pin_bit_mask) {
    if (!is_valid_class(is_input, class_id)) {
        ESP_LOGE(LOG_TAG, "Adding pin %d/%d %d", is_input, class_id, gpio_pin);
        return -1;
    }

    PinClass *pin_class = &pin_definitions[ is_input ].classes[ class_id ];

    ESP_LOGI(LOG_TAG, "Adding pin %d/%d/%d %d", is_input, class_id, pin_class->used_pin_count, gpio_pin);
    if (pin_class->used_pin_count == pin_class->max_pin_count) {
        ESP_LOGE(LOG_TAG, "Too many pins on class_id %d/%d", is_input, class_id);
        return -1;
    }
    int index = pin_class->used_pin_count++;

    PinData pin_data;
    read_nvs_or_default(is_input, class_id, index, &pin_data);

    Pin *pin = &pin_class->pins[ index ];
    memset(pin, 0, sizeof(Pin));
    pin->name = pin_data.name;
    pin->is_manual = pin_data.is_manual;
    pin->pin = gpio_pin;
    pin->level_type = level_type != inherit ? level_type : pin_class->level_type;
    pin->delay_ms_going_high = pin->delay_ms_going_low = PIN_SETTLE_DEFAULT_TIMEOUT_MS;

    *pin_bit_mask |= (1ULL << gpio_pin);
    if (!is_input) {
        set_pin_state(pin, false);
    }
    return index;
}

static void add_input_pins(void *user_context, gpio_define_pins_callback_t define_input_pins,
                           gpio_changed_callback_t gpio_changed_callback) {
    if (define_input_pins == NULL) {
        return;
    }
    //zero-initialize the config structure.
    gpio_config_t io_conf = {};

    define_input_pins(&io_conf, user_context);

    if (io_conf.pin_bit_mask == 0) {
        ESP_LOGW(LOG_TAG, "No input pins added");
        return;
    }

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    // io_conf.pin_bit_mask = set above
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    io_conf.mode = GPIO_MODE_INPUT;

    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);

    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);

    gpio_task_init(gpio_changed_callback);
    PinClasses *input_classes = &pin_definitions[ INPUTS ];
    for (int input_class_index = 0; input_class_index < input_classes->used_classes; input_class_index++) {
        PinClass *input_class = &input_classes->classes[ input_class_index ];
        for (int input_pin = 0; input_pin < input_class->used_pin_count; input_pin++) {
            Pin *pin = &input_class->pins[ input_pin ];
            gpio_task_add(pin->pin, pin->delay_ms_going_low, pin->delay_ms_going_high);
        }
    }
}

static void add_output_pins(void *user_context, gpio_define_pins_callback_t define_output_pins) {
    if (define_output_pins == NULL) {
        return;
    }

    gpio_config_t io_conf = {};

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_down_en = PIN_DISABLED;
    io_conf.pull_up_en = PIN_DISABLED;

    /* does not work to get pin 26 work as output
    // Disable DAC1
    REG_CLR_BIT( RTC_IO_PAD_DAC1_REG, RTC_IO_PDAC1_XPD_DAC );
    REG_SET_BIT( RTC_IO_PAD_DAC1_REG, RTC_IO_PDAC1_DAC_XPD_FORCE );
    // Disable DAC2
    REG_CLR_BIT( RTC_IO_PAD_DAC2_REG, RTC_IO_PDAC2_XPD_DAC );
    REG_SET_BIT( RTC_IO_PAD_DAC2_REG, RTC_IO_PDAC2_DAC_XPD_FORCE );
     */

    define_output_pins(&io_conf, user_context);

    if (io_conf.pin_bit_mask == 0) {
        ESP_LOGI(LOG_TAG, "No output pins added");
        return;
    }
    gpio_config(&io_conf);
}

void gpio_init(void *user_context,
               gpio_define_pins_callback_t define_input_pins,
               gpio_define_pins_callback_t define_output_pins,
               gpio_changed_callback_t gpio_changed_callback) {
    nvs_storage_handle = nvs_open_storage();
    config_version_read(nvs_storage_handle, NVS_KEY_VERSION, &version);
    add_input_pins(user_context, define_input_pins, gpio_changed_callback);
    add_output_pins(user_context, define_output_pins);
    nvs_close_storage(nvs_storage_handle);
    nvs_storage_handle = 0;
}

int gpio_get_number_of_classes(bool is_input) {
    return pin_definitions[ is_input ].used_classes;
}

const char *gpio_get_class_name(bool is_input, int class_id) {
    if (!is_valid_class(is_input, class_id)) {
        return "wrong class_id";
    }
    return pin_definitions[ is_input ].classes[ class_id ].name;
}

int gpio_get_number_of_pins(bool is_input, int class_id) {
    if (!is_valid_class(is_input, class_id)) {
        return -1;
    }
    return pin_definitions[ is_input ].classes[ class_id ].max_pin_count;
}

bool gpio_get_pin_state(bool is_input, int class_id, int index) {
    if (!gpio_is_valid_index(is_input, class_id, index)) {
        return -1;
    }
    Pin *pin = &pin_definitions[ is_input ].classes[ class_id ].pins[ index ];
    if (!is_input) {
        return pin->state;
    }
    int state = gpio_get_level(pin->pin);
    return (pin->level_type == high_is_on) == state;
}

bool gpio_get_pin_data(bool is_input, int class_id, int index, PinData *pin_data) {
    if (!gpio_is_valid_index(is_input, class_id, index)) {
        return false;
    }
    Pin *pin = &pin_definitions[ is_input ].classes[ class_id ].pins[ index ];
    pin_data->name = pin->name;
    pin_data->is_manual = pin->is_manual;
    pin_data->state = gpio_get_pin_state(is_input, class_id, index);
    return true;
}

void gpio_set_pin_state(bool is_input, int class_id, int index, bool state) {
    if (is_input || !gpio_is_valid_index(is_input, class_id, index)) {
        return;
    }

    Pin *pin = &pin_definitions[ is_input ].classes[ class_id ].pins[ index ];
    if (!pin->is_manual) {
        set_pin_state(pin, state);
    }
}

void gpio_set_pin_state_forced(bool is_input, int class_id, int index, bool state) {
    if (is_input || !gpio_is_valid_index(is_input, class_id, index)) {
        return;
    }
    set_pin_state(&pin_definitions[ is_input ].classes[ class_id ].pins[ index ], state);
}

bool gpio_set_pin_data(bool is_input, int class_id, int index, PinData *pin_data) {
    if (!gpio_is_valid_index(is_input, class_id, index)) {
        return false;
    }
    Pin *pin = &pin_definitions[ is_input ].classes[ class_id ].pins[ index ];
    bool write_to_nvs = false;
    if (pin_data->name != NULL && pin_data->name != pin->name) {
        if (pin->name != NULL) {
            free(pin->name);
        }
        pin->name = strdup(pin_data->name);
        write_to_nvs = true;
    }
    if (pin_data->is_manual != pin->is_manual) {
        pin->is_manual = pin_data->is_manual;
        write_to_nvs = true;
    }
    if (write_to_nvs) {
        char *json_string = gpio_data_to_json_string(pin_data);
        char nvs_key[ 256 ];
        get_nvs_key(is_input, class_id, index, nvs_key, sizeof nvs_key);

        nvs_handle_t nvs_handle = nvs_open_storage();
        nvs_write_string(nvs_handle, nvs_key, json_string);
        config_version_set_and_write(nvs_handle, &version);
        nvs_close_storage(nvs_handle);

        free(json_string);
    }
    return true;
}

void gpio_set_delays(bool is_input, int class_id, int index, int delay_ms_going_low, int delay_ms_going_high) {
    if (!gpio_is_valid_index(is_input, class_id, index)) {
        return;
    }
    Pin *pin = &pin_definitions[ is_input ].classes[ class_id ].pins[ index ];
    pin->delay_ms_going_high = delay_ms_going_high;
    pin->delay_ms_going_low = delay_ms_going_low;
}

const char *gpio_get_version() {
    return version.string;
}
