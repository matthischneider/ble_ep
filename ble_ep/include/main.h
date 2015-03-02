/* Copyright (c) 2012 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */
#ifndef NRF6310_H__
#define NRF6310_H__

#include "nrf_gpio.h"


#define BUTTON_START   0
#define BUTTON_0       0
#define BUTTON_1       1
#define BUTTON_2       2
#define BUTTON_3       3
#define BUTTON_4       4
#define BUTTON_5       5
#define BUTTON_6       6
#define BUTTON_7       7
#define BUTTON_STOP    7
#define BUTTON_PULL    NRF_GPIO_PIN_NOPULL

#define PIN_PANEL_ON   24
#define PIN_BORDER     25
#define PIN_DISCHARG   18
#define PIN_PWM        19
#define PIN_RESET      29
#define PIN_BUSY       30
#define PIN_CS         16
#define PIN_SCK        23
#define PIN_MOSI       21
#define PIN_MISO       22
#define PIN_BTN       0

#endif  // NRF6310_H__
