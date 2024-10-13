#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "hardware/gpio.h"
#include "pwm.pio.h"

#define TRIG_PIN 15

const uint32_t lookup_table[340] = {
    18,    37,    56,    75,    94,   113,   132,
   151,   170,   188,   207,   226,   245,   264,   282,
   301,   320,   339,   357,   376,   394,   413,   431,
   450,   468,   487,   505,   523,   542,   560,   578,
   596,   614,   632,   650,   668,   686,   704,   722,
   739,   757,   774,   792,   809,   827,   844,   861,
   878,   895,   912,   929,   946,   963,   979,   996,
  1013,  1029,  1045,  1061,  1078,  1094,  1110,  1125,
  1141,  1157,  1172,  1188,  1203,  1219,  1234,  1249,
  1264,  1279,  1293,  1308,  1322,  1337,  1351,  1365,
  1379,  1393,  1407,  1421,  1434,  1448,  1461,  1474,
  1487,  1500,  1513,  1526,  1538,  1551,  1563,  1575,
  1587,  1599,  1611,  1622,  1634,  1645,  1656,  1667,
  1678,  1689,  1700,  1710,  1721,  1731,  1741,  1751,
  1760,  1770,  1779,  1789,  1798,  1807,  1816,  1824,
  1833,  1841,  1849,  1857,  1865,  1873,  1881,  1888,
  1895,  1902,  1909,  1916,  1923,  1929,  1935,  1941,
  1947,  1953,  1959,  1964,  1969,  1974,  1979,  1984,
  1989,  1993,  1997,  2001,  2005,  2009,  2013,  2016,
  2019,  2022,  2025,  2028,  2030,  2033,  2035,  2037,
  2039,  2040,  2042,  2043,  2044,  2045,  2046,  2047,
  2047,  2047,  2048,  2047,  2047,  2047,  2046,  2045,
  2044,  2043,  2042,  2040,  2039,  2037,  2035,  2033,
  2030,  2028,  2025,  2022,  2019,  2016,  2013,  2009,
  2005,  2001,  1997,  1993,  1989,  1984,  1979,  1974,
  1969,  1964,  1959,  1953,  1947,  1941,  1935,  1929,
  1923,  1916,  1909,  1902,  1895,  1888,  1881,  1873,
  1865,  1857,  1849,  1841,  1833,  1824,  1816,  1807,
  1798,  1789,  1779,  1770,  1760,  1751,  1741,  1731,
  1721,  1710,  1700,  1689,  1678,  1667,  1656,  1645,
  1634,  1622,  1611,  1599,  1587,  1575,  1563,  1551,
  1538,  1526,  1513,  1500,  1487,  1474,  1461,  1448,
  1434,  1421,  1407,  1393,  1379,  1365,  1351,  1337,
  1322,  1308,  1293,  1279,  1264,  1249,  1234,  1219,
  1203,  1188,  1172,  1157,  1141,  1125,  1110,  1094,
  1078,  1061,  1045,  1029,  1013,   996,   979,   963,
   946,   929,   912,   895,   878,   861,   844,   827,
   809,   792,   774,   757,   739,   722,   704,   686,
   668,   650,   632,   614,   596,   578,   560,   542,
   523,   505,   487,   468,   450,   431,   413,   394,
   376,   357,   339,   320,   301,   282,   264,   245,
   226,   207,   188,   170,   151,   132,   113,    94,
    75,    56,    37,    18, 0};

bool pos = 0;

uint low_sm;
uint high_sm;
int low_pwm_dma_chan;
int high_pwm_dma_chan;

void dma_handler(){
    // Clear the interrupt request.
    dma_channel_set_read_addr(low_pwm_dma_chan, &lookup_table[low_sm], true);
    dma_channel_set_read_addr(high_pwm_dma_chan, &lookup_table[low_sm], false);
    dma_hw->ints0 = 1u << high_pwm_dma_chan;
    gpio_put(TRIG_PIN, pos);
    pos = !pos;
    //restart transfer
}

void pio_pwm_set_period(PIO pio, uint sm, uint32_t period) {
    pio_sm_set_enabled(pio, sm, false);
    pio_sm_put_blocking(pio, sm, period);
    pio_sm_exec(pio, sm, pio_encode_pull(false, false));
    pio_sm_exec(pio, sm, pio_encode_out(pio_isr, 32));
    pio_sm_set_enabled(pio, sm, true);
}

int main()
{
    gpio_init(TRIG_PIN);
    gpio_set_dir(TRIG_PIN, GPIO_OUT);
    gpio_put(TRIG_PIN, 0);
    // ============= INIT PIO =============
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &pwm_program);

    // Find a free state machine on our chosen PIO (erroring if there are
    // none). Configure it to run our program, and start it, using the
    // helper function we included in our .pio file.
    low_sm = pio_claim_unused_sm(pio, true);

    pwm_program_init(pio, low_sm, offset, 0);
    pio_pwm_set_period(pio, low_sm, (2048-1));

    high_sm = pio_claim_unused_sm(pio, true);

    pwm_program_init(pio, high_sm, offset, 2);
    pio_pwm_set_period(pio, high_sm, (2048-1));

    // Reserve DMA channel to drive the PWM
    low_pwm_dma_chan = dma_claim_unused_channel(true);
    high_pwm_dma_chan = dma_claim_unused_channel(true);

    // ============= LOW PWM DMA =============
    dma_channel_config low_pwm_dma_chan_config = dma_channel_get_default_config(low_pwm_dma_chan);
    // Transfers 32-bits at a time, increment read address so we pick up a new fade value each
    // time, don't increment writes address so we always transfer to the same PWM register.
    channel_config_set_transfer_data_size(&low_pwm_dma_chan_config, DMA_SIZE_32);
    channel_config_set_read_increment(&low_pwm_dma_chan_config, true);
    channel_config_set_write_increment(&low_pwm_dma_chan_config, false);
    //start high when finished
    channel_config_set_chain_to(&low_pwm_dma_chan_config, high_pwm_dma_chan),
    // Transfer when PWM slice that is connected to the LED asks for a new value
    channel_config_set_dreq(&low_pwm_dma_chan_config, DREQ_PIO0_TX0 + low_sm);

    dma_channel_configure(
        low_pwm_dma_chan,
        &low_pwm_dma_chan_config,
        &pio0_hw->txf[low_sm], // Write to PWM counter compare
        &lookup_table[0], // Read values from fade buffer
        340,
        false // Start immediately.
    );

    // ============= HIGH PWM DMA =============

    dma_channel_config high_pwm_dma_chan_config = dma_channel_get_default_config(high_pwm_dma_chan);
    // Transfers 32-bits at a time, increment read address so we pick up a new fade value each
    // time, don't increment writes address so we always transfer to the same PWM register.
    channel_config_set_transfer_data_size(&high_pwm_dma_chan_config, DMA_SIZE_32);
    channel_config_set_read_increment(&high_pwm_dma_chan_config, true);
    channel_config_set_write_increment(&high_pwm_dma_chan_config, false);
    //start high when finished
    //channel_config_set_chain_to(&high_pwm_dma_chan_config, low_pwm_dma_chan),
    // Transfer when PWM slice that is connected to the LED asks for a new value
    channel_config_set_dreq(&high_pwm_dma_chan_config, DREQ_PIO0_TX0 + high_sm);

    dma_channel_configure(
        high_pwm_dma_chan,
        &high_pwm_dma_chan_config,
        &pio0_hw->txf[high_sm], // Write to PWM counter compare
        &lookup_table[0], // Read values from fade buffer
        340,
        false // Start immediately.
    );

    dma_channel_set_irq0_enabled(high_pwm_dma_chan, true);

    // Configure the processor to run dma_handler() when DMA IRQ 0 is asserted
    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);

    irq_set_enabled(DMA_IRQ_0, true);

    dma_channel_start(low_pwm_dma_chan);

    while(true) {
        tight_loop_contents();
    }
}