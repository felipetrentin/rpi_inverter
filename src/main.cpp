#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/pwm.h"
#include "hardware/dma.h"

#define MYPIN 1

int pwm_dma_chan;

const uint32_t lookup_table[25] = {
0x80000000, 0x9fd511f9, 0xbdaa1ab9, 0xd79f3d53, 0xec12effd, 0xf9bc384c, 0xffbf56ff, 0xfdbb96b0,
0xf3d15f72, 0xe2a02d8d, 0xcb3c8c11, 0xaf1eb491, 0x900aeb5d, 0x6ff514a2, 0x50e14b6e, 0x34c373ee,
0x1d5fd272, 0xc2ea08d, 0x244694f, 0x40a900, 0x643c7b3, 0x13ed1002, 0x2860c2ac, 0x4255e546,
0x602aee06};

void dma_handler(){
    // Clear the interrupt request.
    dma_channel_set_read_addr(pwm_dma_chan, &lookup_table[0], true);
    dma_hw->ints0 = 1u << pwm_dma_chan;
    //restart transfer
    
}

int main()
{
    gpio_set_function(MYPIN, GPIO_FUNC_PWM);

    int led_pwm_slice_num = pwm_gpio_to_slice_num(MYPIN);

    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv_int_frac(&config, 1, 1);
    pwm_config_set_clkdiv_mode(&config, PWM_DIV_FREE_RUNNING);
    pwm_init(led_pwm_slice_num, &config, true);

    // Setup DMA channel to drive the PWM
    pwm_dma_chan = dma_claim_unused_channel(true);

    dma_channel_config pwm_dma_chan_config = dma_channel_get_default_config(pwm_dma_chan);
    // Transfers 32-bits at a time, increment read address so we pick up a new fade value each
    // time, don't increment writes address so we always transfer to the same PWM register.
    channel_config_set_transfer_data_size(&pwm_dma_chan_config, DMA_SIZE_32);
    channel_config_set_read_increment(&pwm_dma_chan_config, true);
    channel_config_set_write_increment(&pwm_dma_chan_config, false);
    // Transfer when PWM slice that is connected to the LED asks for a new value
    channel_config_set_dreq(&pwm_dma_chan_config, DREQ_PWM_WRAP0 + led_pwm_slice_num);

    // Setup the channel and set it going
    dma_channel_configure(
        pwm_dma_chan,
        &pwm_dma_chan_config,
        &pwm_hw->slice[led_pwm_slice_num].cc, // Write to PWM counter compare
        lookup_table, // Read values from fade buffer
        25,
        true // Start immediately.
    );

    dma_channel_set_irq0_enabled(pwm_dma_chan, true);

    // Configure the processor to run dma_handler() when DMA IRQ 0 is asserted
    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);

    while(true) {
        tight_loop_contents();
    }
}