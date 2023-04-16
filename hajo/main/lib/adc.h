#ifndef HAJO_ADC_H
#define HAJO_ADC_H

#include <stdint.h>
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void adc_main( bool start_timer );

extern int adc_number_of_channels();

typedef struct {
    uint32_t value;
    uint32_t max_value;
    uint8_t instance;
    uint8_t type;
} adc_channel_value_t;

typedef void (*adc_value_converter)( uint32_t raw_value,
                                     uint32_t *display_value,
                                     uint32_t *correction );

extern esp_err_t adc_get_channel_value( int index,
                                        adc_channel_value_t *channel_value );

extern esp_err_t adc_add_channel( uint8_t adc_channel,
                                  const char *name,
                                  uint8_t instance,
                                  uint8_t type,
                                  uint32_t max_value,
                                  adc_value_converter converter );

extern void adc_read_all();

#ifdef __cplusplus
}
#endif

#endif //HAJO_ADC_H
