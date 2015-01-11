#include <stdint.h>
#include <limits.h>
#include <stdbool.h>
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "spi_master.h"
#include "app_error.h"
#include "app_util_platform.h"
#include "nrf_pwm.h"

#if !defined(DISPLAY_H)
#define DISPLAY_H 1

typedef struct
{
	uint8_t pinPanelOn;
	uint8_t pinBorder;
	uint8_t pinDischarge;
	uint8_t pinPWM;
	uint8_t pinReset;
	uint8_t pinBusy;


	spi_master_config_t spi_config;
	uint8_t pinCS;
	uint8_t pinMISO;
	uint8_t pinMOSI;
	uint8_t pinCLK;


	uint16_t stageTime;
	uint16_t linesPerDisplay;
	uint16_t dotsPerLine;
	uint16_t bytesPerLine;
	uint16_t bytesPerScan;
} epd_t;

static void epdSPIPut(uint8_t c);
static void spiEventHandler(spi_master_evt_t spi_master_evt);
static void epdSPIOn(epd_t *epd);
static void epdSPIOff() ;
static void epdSPISend(uint8_t cs_pin, const uint8_t *buffer, uint16_t length);
static void epdSPIBuffer(uint8_t c);
static void epdPWMStart(int pin);
static void epdPWMStop(int pin);
static void epdInit(epd_t* epd, uint8_t pinCS, uint8_t pinPanelOn, uint8_t pinBorder,
		uint8_t pinDischarge, uint8_t pinPWM, uint8_t pinReset, uint8_t pinBusy,
		uint8_t pinCLK, uint8_t pinMISO, uint8_t pinMOSI);
static void epdBegin(epd_t *epd);
static void epdEnd(epd_t *epd);
static void epdFrame(epd_t *epd, const uint8_t *image);
static void epdLine(epd_t *epd, uint16_t line, const uint8_t *data, uint8_t fixed_value);
#endif
