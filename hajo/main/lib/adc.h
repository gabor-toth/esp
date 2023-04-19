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
    int channel;
    const char *name;
    uint32_t raw_value;
    uint32_t display_value;
    void *user_data;
} adc_channel_value_t;

typedef void (*adc_value_converter)( uint32_t raw_value,
                                     uint32_t *display_value,
                                     uint32_t *correction );

extern esp_err_t adc_get_channel_value( int index,
                                        adc_channel_value_t *channel_value );

extern void *adc_get_channel_user_data( int index );

extern esp_err_t adc_add_channel( uint8_t adc_channel,
                                  const char *name,
                                  void *user_data,
                                  size_t user_data_bytes,
                                  adc_value_converter converter );

extern void adc_read_all();

#ifdef __cplusplus
}
#endif

#endif //HAJO_ADC_H
