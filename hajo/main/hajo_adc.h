//
// Created by tothg on 2023.01.11..
//

#ifndef HAJO_HAJO_ADC_H
#define HAJO_HAJO_ADC_H

extern void adc_main();

extern int adc_number_of_channels();

typedef struct {
    uint32_t value;
    uint8_t instance;
} adc_channel_value_t;

extern uint32_t adc_get_channel_value( int index, adc_channel_value_t *channel_value );

#endif //HAJO_HAJO_ADC_H
