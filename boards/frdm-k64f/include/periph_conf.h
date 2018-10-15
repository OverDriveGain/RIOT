/*
 * Copyright (C) 2014 Freie Universität Berlin
 * Copyright (C) 2015 PHYTEC Messtechnik GmbH
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for more
 * details.
 */

/**
 * @ingroup     boards_frdm-k64f
 * @{
 *
 * @file
 * @name        Peripheral MCU configuration for the FRDM-K64F
 *
 * @author      Johann Fischer <j.fischer@phytec.de>
 */

#ifndef PERIPH_CONF_H
#define PERIPH_CONF_H

#include "periph_cpu.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @name Clock system configuration
 * @{
 */
static const clock_config_t clock_config = {
    /*
     * This configuration results in the system running from the PLL output with
     * the following clock frequencies:
     * Core:  60 MHz
     * Bus:   60 MHz
     * Flex:  20 MHz
     * Flash: 20 MHz
     */
    .clkdiv1 = SIM_CLKDIV1_OUTDIV1(0) | SIM_CLKDIV1_OUTDIV2(0) |
               SIM_CLKDIV1_OUTDIV3(2) | SIM_CLKDIV1_OUTDIV4(2),
    .default_mode = KINETIS_MCG_MODE_PEE,
    /* The board has an external RMII (Ethernet) clock which drives the ERC at 50 MHz */
    .erc_range = KINETIS_MCG_ERC_RANGE_VERY_HIGH,
    .fcrdiv = 0, /* Fast IRC divide by 1 => 4 MHz */
    .oscsel = 0, /* Use EXTAL for external clock */
    .clc = 0, /* External load caps on board */
    .fll_frdiv = 0b111, /* Divide by 1536 => FLL input 32252 Hz */
    .fll_factor_fei = KINETIS_MCG_FLL_FACTOR_1464, /* FLL freq = 48 MHz  */
    .fll_factor_fee = KINETIS_MCG_FLL_FACTOR_1920, /* FLL freq = 62.5 MHz */
    .pll_prdiv = 0b10011, /* Divide by 20 */
    .pll_vdiv = 0b00000, /* Multiply by 24 => PLL freq = 60 MHz */
    .enable_oscillator = false, /* Use EXTAL directly without OSC0 */
    .select_fast_irc = true,
    .enable_mcgirclk = false,
};
#define CLOCK_CORECLOCK              (60000000ul)
#define CLOCK_BUSCLOCK               (CLOCK_CORECLOCK / 1)
/** @} */

/**
 * @name Timer configuration
 * @{
 */
#define PIT_NUMOF               (2U)
#define PIT_CONFIG {                 \
        {                            \
            .prescaler_ch = 0,       \
            .count_ch = 1,           \
        },                           \
        {                            \
            .prescaler_ch = 2,       \
            .count_ch = 3,           \
        },                           \
    }
#define LPTMR_NUMOF             (0U)
#define LPTMR_CONFIG {}
#define TIMER_NUMOF             ((PIT_NUMOF) + (LPTMR_NUMOF))

#define PIT_BASECLOCK           (CLOCK_BUSCLOCK)
#define PIT_ISR_0               isr_pit1
#define PIT_ISR_1               isr_pit3
#define LPTMR_ISR_0             isr_lptmr0

/** @} */

/**
* @name UART configuration
* @{
*/
static const uart_conf_t uart_config[] = {
    {
        .dev    = UART0,
        .freq   = CLOCK_CORECLOCK,
        .pin_rx = GPIO_PIN(PORT_B, 16),
        .pin_tx = GPIO_PIN(PORT_B, 17),
        .pcr_rx = PORT_PCR_MUX(3),
        .pcr_tx = PORT_PCR_MUX(3),
        .irqn   = UART0_RX_TX_IRQn,
        .scgc_addr = &SIM->SCGC4,
        .scgc_bit = SIM_SCGC4_UART0_SHIFT,
        .mode   = UART_MODE_8N1
    },
};

#define UART_0_ISR          (isr_uart0_rx_tx)

#define UART_NUMOF          (sizeof(uart_config) / sizeof(uart_config[0]))
/** @} */

/**
 * @name    ADC configuration
 * @{
 */
static const adc_conf_t adc_config[] = {
//<<<<<head
    [ 0] = { .dev = ADC0, .pin = GPIO_PIN(PORT_B,  2), .chan = 12, .avg = ADC_AVG_MAX }, /* PTB2 (Arduino A0) */
    [ 1] = { .dev = ADC0, .pin = GPIO_PIN(PORT_B,  3), .chan = 13, .avg = ADC_AVG_MAX }, /* PTB3 (Arduino A1) */
    [ 2] = { .dev = ADC1, .pin = GPIO_PIN(PORT_B, 10), .chan = 14, .avg = ADC_AVG_MAX }, /* PTB10 (Arduino A2) */
    [ 3] = { .dev = ADC1, .pin = GPIO_PIN(PORT_B, 11), .chan = 15, .avg = ADC_AVG_MAX }, /* PTB11 (Arduino A3) */
    [ 4] = { .dev = ADC1, .pin = GPIO_PIN(PORT_C, 11), .chan =  7, .avg = ADC_AVG_MAX }, /* PTC11 (Arduino A4) */
    [ 5] = { .dev = ADC1, .pin = GPIO_PIN(PORT_C, 10), .chan =  6, .avg = ADC_AVG_MAX }, /* PTC10 (Arduino A5) */
    [ 6] = { .dev = ADC0, .pin = GPIO_UNDEF          , .chan =  0, .avg = ADC_AVG_MAX }, /* ADC0_DP0 */
    [ 7] = { .dev = ADC0, .pin = GPIO_UNDEF          , .chan = 19, .avg = ADC_AVG_MAX }, /* ADC0_DM0 */
    [ 8] = { .dev = ADC0, .pin = GPIO_UNDEF          , .chan = (0 | ADC_SC1_DIFF_MASK), .avg = ADC_AVG_MAX }, /* ADC0_DP0 - ADC0_DM0 */
    [ 9] = { .dev = ADC1, .pin = GPIO_UNDEF          , .chan =  0, .avg = ADC_AVG_MAX }, /* ADC1_DP0 */
    [10] = { .dev = ADC1, .pin = GPIO_UNDEF          , .chan = 19, .avg = ADC_AVG_MAX }, /* ADC1_DM0 */
    [11] = { .dev = ADC1, .pin = GPIO_UNDEF          , .chan = (0 | ADC_SC1_DIFF_MASK), .avg = ADC_AVG_MAX }, /* ADC1_DP0 - ADC1_DM0 */
    [12] = { .dev = ADC0, .pin = GPIO_UNDEF          , .chan =  1, .avg = ADC_AVG_MAX }, /* ADC0_DP1 */
    [13] = { .dev = ADC0, .pin = GPIO_UNDEF          , .chan = 20, .avg = ADC_AVG_MAX }, /* ADC0_DM1 */
    [14] = { .dev = ADC0, .pin = GPIO_UNDEF          , .chan = (1 | ADC_SC1_DIFF_MASK), .avg = ADC_AVG_MAX }, /* ADC0_DP1 - ADC0_DM1 */
    [15] = { .dev = ADC1, .pin = GPIO_UNDEF          , .chan =  1, .avg = ADC_AVG_MAX }, /* ADC1_DP1 */
    [16] = { .dev = ADC1, .pin = GPIO_UNDEF          , .chan = 20, .avg = ADC_AVG_MAX }, /* ADC1_DM1 */
    [17] = { .dev = ADC1, .pin = GPIO_UNDEF          , .chan = (1 | ADC_SC1_DIFF_MASK), .avg = ADC_AVG_MAX }, /* ADC1_DP1 - ADC1_DM1 */
    /* internal: temperature sensor */
    /* The temperature sensor has a very high output impedance, it must not be
     * sampled using hardware averaging, or the sampled values will be garbage */
    [18] = { .dev = ADC0, .pin = GPIO_UNDEF, .chan = 26, .avg = ADC_AVG_NONE },
    /* internal: band gap */
    /* Note: the band gap buffer uses a bit of current and is turned off by default,
     * Set PMC->REGSC |= PMC_REGSC_BGBE_MASK before reading or the input will be floating */
    [19] = { .dev = ADC0, .pin = GPIO_UNDEF, .chan = 27, .avg = ADC_AVG_MAX },
//MZTODO What is the adc config . in which commit has this changed to the following
//=====
//    { .dev = ADC0, .pin = GPIO_PIN(PORT_B, 10), .chan = 14 },
//    { .dev = ADC0, .pin = GPIO_PIN(PORT_B, 11), .chan = 15 },
//    { .dev = ADC0, .pin = GPIO_PIN(PORT_C, 11), .chan =  7 },
//    { .dev = ADC0, .pin = GPIO_PIN(PORT_C, 10), .chan =  6 },
//    { .dev = ADC0, .pin = GPIO_PIN(PORT_C,  8), .chan =  4 },
//    { .dev = ADC0, .pin = GPIO_PIN(PORT_C,  9), .chan =  5 }
//>>>>>>> Fix dependencies, make javascript example run
};

#define ADC_NUMOF           (sizeof(adc_config) / sizeof(adc_config[0]))
/** @} */

/**
 * @name    PWM configuration
 * @{
 */
static const pwm_conf_t pwm_config[] = {
    {
        .ftm        = FTM0,
        .chan       = {
            { .pin = GPIO_PIN(PORT_A, 4), .af = 3, .ftm_chan = 6 },
            { .pin = GPIO_PIN(PORT_A, 2), .af = 3, .ftm_chan = 7 },
            { .pin = GPIO_PIN(PORT_C, 2), .af = 4, .ftm_chan = 1 },
            { .pin = GPIO_PIN(PORT_C, 3), .af = 4, .ftm_chan = 2 }
        },
        .chan_numof = 4,
        .ftm_num    = 0
    }
};

#define PWM_NUMOF           (sizeof(pwm_config) / sizeof(pwm_config[0]))
/** @} */


/**
 * @name   SPI configuration
 *
 * Clock configuration values based on the configured 30Mhz module clock.
 *
 * Auto-generated by:
 * cpu/kinetis_common/dist/calc_spi_scalers/calc_spi_scalers.c
 *
* @{
*/
static const uint32_t spi_clk_config[] = {
    (
        SPI_CTAR_PBR(2) | SPI_CTAR_BR(6) |          /* -> 93750Hz */
        SPI_CTAR_PCSSCK(2) | SPI_CTAR_CSSCK(5) |
        SPI_CTAR_PASC(2) | SPI_CTAR_ASC(5) |
        SPI_CTAR_PDT(2) | SPI_CTAR_DT(5)
    ),
    (
        SPI_CTAR_PBR(2) | SPI_CTAR_BR(4) |          /* -> 375000Hz */
        SPI_CTAR_PCSSCK(2) | SPI_CTAR_CSSCK(3) |
        SPI_CTAR_PASC(2) | SPI_CTAR_ASC(3) |
        SPI_CTAR_PDT(2) | SPI_CTAR_DT(3)
    ),
    (
        SPI_CTAR_PBR(2) | SPI_CTAR_BR(2) |          /* -> 1000000Hz */
        SPI_CTAR_PCSSCK(0) | SPI_CTAR_CSSCK(4) |
        SPI_CTAR_PASC(0) | SPI_CTAR_ASC(4) |
        SPI_CTAR_PDT(0) | SPI_CTAR_DT(4)
    ),
    (
        SPI_CTAR_PBR(1) | SPI_CTAR_BR(0) |          /* -> 5000000Hz */
        SPI_CTAR_PCSSCK(1) | SPI_CTAR_CSSCK(0) |
        SPI_CTAR_PASC(1) | SPI_CTAR_ASC(0) |
        SPI_CTAR_PDT(1) | SPI_CTAR_DT(0)
    ),
    (
        SPI_CTAR_PBR(0) | SPI_CTAR_BR(0) |          /* -> 7500000Hz */
        SPI_CTAR_PCSSCK(0) | SPI_CTAR_CSSCK(1) |
        SPI_CTAR_PASC(0) | SPI_CTAR_ASC(1) |
        SPI_CTAR_PDT(0) | SPI_CTAR_DT(1)
    )
};

static const spi_conf_t spi_config[] = {
    {
        .dev      = SPI0,
        .pin_miso = GPIO_PIN(PORT_D, 3),
        .pin_mosi = GPIO_PIN(PORT_D, 2),
        .pin_clk  = GPIO_PIN(PORT_D, 1),
        .pin_cs   = {
            GPIO_PIN(PORT_D, 0),
            GPIO_UNDEF,
            GPIO_UNDEF,
            GPIO_UNDEF,
            GPIO_UNDEF
        },
        .pcr      = GPIO_AF_2,
        .simmask  = SIM_SCGC6_SPI0_MASK
    }
};

#define SPI_NUMOF           (sizeof(spi_config) / sizeof(spi_config[0]))
/** @} */


/**
* @name I2C configuration
* @{
*/
#define I2C_NUMOF                    (1U)
#define I2C_0_EN                     1
/* Low (10 kHz): MUL = 4, SCL divider = 1536, total: 6144 */
#define KINETIS_I2C_F_ICR_LOW        (0x36)
#define KINETIS_I2C_F_MULT_LOW       (2)
/* Normal (100 kHz): MUL = 2, SCL divider = 320, total: 640 */
#define KINETIS_I2C_F_ICR_NORMAL     (0x25)
#define KINETIS_I2C_F_MULT_NORMAL    (1)
/* Fast (400 kHz): MUL = 1, SCL divider = 160, total: 160 */
#define KINETIS_I2C_F_ICR_FAST       (0x1D)
#define KINETIS_I2C_F_MULT_FAST      (0)
/* Fast plus (1000 kHz): MUL = 1, SCL divider = 64, total: 64 */
#define KINETIS_I2C_F_ICR_FAST_PLUS  (0x12)
#define KINETIS_I2C_F_MULT_FAST_PLUS (0)

/* I2C 0 device configuration */
#define I2C_0_DEV                    I2C0
#define I2C_0_CLKEN()                (SIM->SCGC4 |= (SIM_SCGC4_I2C0_MASK))
#define I2C_0_CLKDIS()               (SIM->SCGC4 &= ~(SIM_SCGC4_I2C0_MASK))
#define I2C_0_IRQ                    I2C0_IRQn
#define I2C_0_IRQ_HANDLER            isr_i2c0
/* I2C 0 pin configuration */
#define I2C_0_PORT                   PORTE
#define I2C_0_PORT_CLKEN()           (SIM->SCGC5 |= (SIM_SCGC5_PORTE_MASK))
#define I2C_0_PIN_AF                 5
#define I2C_0_SDA_PIN                25
#define I2C_0_SCL_PIN                24
#define I2C_0_PORT_CFG               (PORT_PCR_MUX(I2C_0_PIN_AF) | PORT_PCR_ODE_MASK)
/** @} */

/**
* @name RTT and RTC configuration
* @{
*/
#define RTT_NUMOF                    (1U)
#define RTC_NUMOF                    (1U)
#define RTT_DEV                      RTC
#define RTT_IRQ                      RTC_IRQn
#define RTT_IRQ_PRIO                 10
#define RTT_UNLOCK()                 (SIM->SCGC6 |= (SIM_SCGC6_RTC_MASK))
#define RTT_ISR                      isr_rtc
#define RTT_FREQUENCY                (1)
#define RTT_MAX_VALUE                (0xffffffff)
/** @} */

/**
 * @name Random Number Generator configuration
 * @{
 */
#define KINETIS_RNGA                RNG
#define HWRNG_CLKEN()               (SIM->SCGC6 |= (1 << 9))
#define HWRNG_CLKDIS()              (SIM->SCGC6 &= ~(1 << 9))
/** @} */

#ifdef __cplusplus
}
#endif

#endif /* PERIPH_CONF_H */
/** @} */
