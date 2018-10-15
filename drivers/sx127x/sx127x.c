/*
 * Copyright (C) 2016 Unwired Devices <info@unwds.com>
 *               2017 Inria Chile
 *               2017 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     drivers_sx127x
 * @{
 * @file
 * @brief       Basic functionality of sx127x driver
 *
 * @author      Eugene P. <ep@unwds.com>
 * @author      José Ignacio Alamos <jose.alamos@inria.cl>
 * @author      Alexandre Abadie <alexandre.abadie@inria.fr>
 * @}
 */
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "xtimer.h"
#include "thread.h"

#include "periph/gpio.h"
#include "periph/spi.h"

#include "sx127x.h"
#include "sx127x_internal.h"
#include "sx127x_registers.h"
#include "sx127x_netdev.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

/* Internal functions */
static void _init_isrs(sx127x_t *dev);
static void _init_timers(sx127x_t *dev);
static int _init_peripherals(sx127x_t *dev);
static void _on_tx_timeout(void *arg);
static void _on_rx_timeout(void *arg);

/* SX127X DIO interrupt handlers initialization */
#ifndef SX127X_USE_DIO_MULTI
static void sx127x_on_dio0_isr(void *arg);
static void sx127x_on_dio1_isr(void *arg);
static void sx127x_on_dio2_isr(void *arg);
static void sx127x_on_dio3_isr(void *arg);
#else
static void sx127x_on_dio_multi_isr(void *arg);
#endif

void sx127x_setup(sx127x_t *dev, const sx127x_params_t *params)
{
    netdev_t *netdev = (netdev_t*) dev;
    netdev->driver = &sx127x_driver;
    memcpy(&dev->params, params, sizeof(sx127x_params_t));
}

int sx127x_reset(const sx127x_t *dev)
{
    /*
     * This reset scheme complies with 7.2 chapter of the SX1272/1276 datasheet
     * See http://www.semtech.com/images/datasheet/sx1276.pdf for SX1276
     * See http://www.semtech.com/images/datasheet/sx1272.pdf for SX1272
     *
     * 1. Set NReset pin to LOW for at least 100 us
     * 2. Set NReset in Hi-Z state
     * 3. Wait at least 5 milliseconds
     */

    /* Check if the reset pin is defined */
    if (dev->params.reset_pin == GPIO_UNDEF) {
        DEBUG("[sx127x] error: No reset pin defined.\n");
        return -SX127X_ERR_GPIOS;
    }

    gpio_init(dev->params.reset_pin, GPIO_OUT);

    /* Set reset pin to 0 */
    gpio_clear(dev->params.reset_pin);

    /* Wait 1 ms */
    xtimer_usleep(1000);

    /* Put reset pin in High-Z */
    gpio_init(dev->params.reset_pin, GPIO_IN);

    /* Wait 10 ms */
    xtimer_usleep(1000 * 10);

    return 0;
}

int sx127x_init(sx127x_t *dev)
{
    /* Do internal initialization routines */
    if (!_init_peripherals(dev)) {
        return -SX127X_ERR_SPI;
    }

    /* Check presence of SX127X */
    if (!sx127x_test(dev)) {
        DEBUG("[Error] init : sx127x test failed\n");
        return -SX127X_ERR_TEST_FAILED;
    }

    _init_timers(dev);
    xtimer_usleep(1000); /* wait 1 millisecond */

    sx127x_reset(dev);

    sx127x_rx_chain_calibration(dev);
    sx127x_set_op_mode(dev, SX127X_RF_OPMODE_SLEEP);

    _init_isrs(dev);

    return SX127X_INIT_OK;
}

void sx127x_init_radio_settings(sx127x_t *dev)
{
    sx127x_set_freq_hop(dev, SX127X_FREQUENCY_HOPPING);
    sx127x_set_iq_invert(dev, SX127X_IQ_INVERSION);
    sx127x_set_rx_single(dev, SX127X_RX_SINGLE);
    sx127x_set_tx_timeout(dev, SX127X_TX_TIMEOUT_DEFAULT);
    sx127x_set_modem(dev, SX127X_MODEM_DEFAULT);
    sx127x_set_channel(dev, SX127X_CHANNEL_DEFAULT);
    sx127x_set_bandwidth(dev, SX127X_BW_DEFAULT);
    sx127x_set_spreading_factor(dev, SX127X_SF_DEFAULT);
    sx127x_set_coding_rate(dev, SX127X_CR_DEFAULT);

    sx127x_set_fixed_header_len_mode(dev, SX127X_FIXED_HEADER_LEN_MODE);
    sx127x_set_crc(dev, SX127X_PAYLOAD_CRC_ON);
    sx127x_set_symbol_timeout(dev, SX127X_SYMBOL_TIMEOUT);
    sx127x_set_preamble_length(dev, SX127X_PREAMBLE_LENGTH);
    sx127x_set_payload_length(dev, SX127X_PAYLOAD_LENGTH);
    sx127x_set_hop_period(dev, SX127X_FREQUENCY_HOPPING_PERIOD);

    sx127x_set_tx_power(dev, SX127X_RADIO_TX_POWER);
}

uint32_t sx127x_random(sx127x_t *dev)
{
    uint32_t rnd = 0;

    sx127x_set_modem(dev, SX127X_MODEM_LORA); /* Set LoRa modem ON */

    /* Disable LoRa modem interrupts */
    sx127x_reg_write(dev, SX127X_REG_LR_IRQFLAGSMASK, SX127X_RF_LORA_IRQFLAGS_RXTIMEOUT |
                     SX127X_RF_LORA_IRQFLAGS_RXDONE |
                     SX127X_RF_LORA_IRQFLAGS_PAYLOADCRCERROR |
                     SX127X_RF_LORA_IRQFLAGS_VALIDHEADER |
                     SX127X_RF_LORA_IRQFLAGS_TXDONE |
                     SX127X_RF_LORA_IRQFLAGS_CADDONE |
                     SX127X_RF_LORA_IRQFLAGS_FHSSCHANGEDCHANNEL |
                     SX127X_RF_LORA_IRQFLAGS_CADDETECTED);

    /* Set radio in continuous reception */
    sx127x_set_op_mode(dev, SX127X_RF_OPMODE_RECEIVER);

    for (unsigned i = 0; i < 32; i++) {
        xtimer_usleep(1000); /* wait for the chaos */

        /* Non-filtered RSSI value reading. Only takes the LSB value */
        rnd |= ((uint32_t) sx127x_reg_read(dev, SX127X_REG_LR_RSSIWIDEBAND) & 0x01) << i;
    }

    sx127x_set_sleep(dev);

    return rnd;
}

/**
 * IRQ handlers
 */
void sx127x_isr(netdev_t *dev)
{
    if (dev->event_callback) {
        dev->event_callback(dev, NETDEV_EVENT_ISR);
    }
}

static void sx127x_on_dio_isr(sx127x_t *dev, sx127x_flags_t flag)
{
    dev->irq |= flag;
    sx127x_isr((netdev_t *)dev);
}

#ifndef SX127X_USE_DIO_MULTI
static void sx127x_on_dio0_isr(void *arg)
{
    sx127x_on_dio_isr((sx127x_t*) arg, SX127X_IRQ_DIO0);
}

static void sx127x_on_dio1_isr(void *arg)
{
    sx127x_on_dio_isr((sx127x_t*) arg, SX127X_IRQ_DIO1);
}

static void sx127x_on_dio2_isr(void *arg)
{
    sx127x_on_dio_isr((sx127x_t*) arg, SX127X_IRQ_DIO2);
}

static void sx127x_on_dio3_isr(void *arg)
{
    sx127x_on_dio_isr((sx127x_t*) arg, SX127X_IRQ_DIO3);
}
#else
static void sx127x_on_dio_multi_isr(void *arg)
{
    sx127x_on_dio_isr((sx127x_t*) arg, SX127X_IRQ_DIO_MULTI);
}
#endif

/* Internal event handlers */
static void _init_isrs(sx127x_t *dev)
{
//MZTODO COMPILE PROBLEMS
	//<<<<<<< HEAD
    int res;

#ifndef SX127X_USE_DIO_MULTI
    /* Check if DIO0 pin is defined */
    if (dev->params.dio0_pin != GPIO_UNDEF) {
        res = gpio_init_int(dev->params.dio0_pin, GPIO_IN, GPIO_RISING,
                                sx127x_on_dio0_isr, dev);
        if (res < 0) {
            DEBUG("[sx127x] error: failed to initialize DIO0 pin\n");
            return res;
        }
    }
    else {
        DEBUG("[sx127x] error: no DIO0 pin defined\n");
        DEBUG("[sx127x] error: at least one interrupt should be defined\n");
        return SX127X_ERR_GPIOS;
    }

    /* Check if DIO1 pin is defined */
    if (dev->params.dio1_pin != GPIO_UNDEF) {
        res = gpio_init_int(dev->params.dio1_pin, GPIO_IN, GPIO_RISING,
                                sx127x_on_dio1_isr, dev);
        if (res < 0) {
            DEBUG("[sx127x] error: failed to initialize DIO1 pin\n");
            return res;
        }
    }

    /* check if DIO2 pin is defined */
    if (dev->params.dio2_pin != GPIO_UNDEF) {
        res = gpio_init_int(dev->params.dio2_pin, GPIO_IN, GPIO_RISING,
                            sx127x_on_dio2_isr, dev);
        if (res < 0) {
            DEBUG("[sx127x] error: failed to initialize DIO2 pin\n");
            return res;
        }
    }

    /* check if DIO3 pin is defined */
    if (dev->params.dio3_pin != GPIO_UNDEF) {
        res = gpio_init_int(dev->params.dio3_pin, GPIO_IN, GPIO_RISING,
                            sx127x_on_dio3_isr, dev);
        if (res < 0) {
            DEBUG("[sx127x] error: failed to initialize DIO3 pin\n");
            return res;
        }
    }
#else
    if (dev->params.dio_multi_pin != GPIO_UNDEF) {
        DEBUG("[sx127x] info: Trying to initialize DIO MULTI pin\n");
        res = gpio_init_int(dev->params.dio_multi_pin, GPIO_IN, GPIO_RISING,
                                sx127x_on_dio_multi_isr, dev);
        if (res < 0) {
            DEBUG("[sx127x] error: failed to initialize DIO MULTI pin\n");
            return res;
        }

        DEBUG("[sx127x] info: DIO MULTI pin initialized successfully\n");
    }
    else {
        DEBUG("[sx127x] error: no DIO MULTI pin defined\n");
        DEBUG("[sx127x] error at least one interrupt should be defined\n");
        return SX127X_ERR_GPIOS;
    }
#endif

    return res;
//=======
//    if (gpio_init_int(dev->params.dio0_pin, GPIO_IN, GPIO_RISING, sx127x_on_dio0_isr, dev) < 0) {
//        DEBUG("Error: cannot initialize DIO0 pin\n");
//    }
//
 //   if (gpio_init_int(dev->params.dio1_pin, GPIO_IN, GPIO_RISING, sx127x_on_dio1_isr, dev) < 0) {
 //       DEBUG("Error: cannot initialize DIO1 pin\n");
 ///   }
//
   // if (gpio_init_int(dev->params.dio2_pin, GPIO_IN, GPIO_RISING, sx127x_on_dio2_isr, dev) < 0) {
   //     DEBUG("Error: cannot initialize DIO2 pin\n");
   // }
//
 //   if (gpio_init_int(dev->params.dio3_pin, GPIO_IN, GPIO_RISING, sx127x_on_dio3_isr, dev) < 0) {
  //      DEBUG("Error: cannot initialize DIO3 pin\n");
  //  }
//>>>>>>> Fix dependencies, make javascript example run
}

static void _on_tx_timeout(void *arg)
{
    netdev_t *dev = (netdev_t *) arg;

    dev->event_callback(dev, NETDEV_EVENT_TX_TIMEOUT);
}

static void _on_rx_timeout(void *arg)
{
    netdev_t *dev = (netdev_t *) arg;

    dev->event_callback(dev, NETDEV_EVENT_RX_TIMEOUT);
}

static void _init_timers(sx127x_t *dev)
{
    dev->_internal.tx_timeout_timer.arg = dev;
    dev->_internal.tx_timeout_timer.callback = _on_tx_timeout;

    dev->_internal.rx_timeout_timer.arg = dev;
    dev->_internal.rx_timeout_timer.callback = _on_rx_timeout;
}

static int _init_peripherals(sx127x_t *dev)
{
    int res;

    /* Setup SPI for SX127X */
    res = spi_init_cs(dev->params.spi, dev->params.nss_pin);

    if (res != SPI_OK) {
        DEBUG("sx127x: error initializing SPI_%i device (code %i)\n",
                  dev->params.spi, res);
        return 0;
    }

    res = gpio_init(dev->params.nss_pin, GPIO_OUT);
    if (res < 0) {
        DEBUG("sx127x: error initializing GPIO_%ld as CS line (code %i)\n",
                  (long)dev->params.nss_pin, res);
        return 0;
    }

    gpio_set(dev->params.nss_pin);

    DEBUG("sx127x: peripherals initialized with success\n");
    return 1;
}
