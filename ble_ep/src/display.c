#include "display.h"

// inline arrays
#define ARRAY(type, ...) ((type[]){__VA_ARGS__})
#define CU8(...) (ARRAY(const uint8_t, __VA_ARGS__))
#define SPI_MASTER_HW SPI_MASTER_0

#define TX_RX_BUF_LENGTH    100u     /**< SPI transaction buffer length. */

//Data buffers.
uint8_t m_tx_data[TX_RX_BUF_LENGTH] = { 0 }; /**< A buffer with data to transfer. */
uint8_t m_rx_data[TX_RX_BUF_LENGTH] = { 0 }; /**< A buffer for incoming data. */
int cursor = 0;

void epdSPIPut(uint8_t c) {
	m_tx_data[0] = c;
	spi_master_send_recv(SPI_MASTER_HW, m_tx_data, 1, m_rx_data, 0);
}

void spiEventHandler(spi_master_evt_t spi_master_evt) {
	switch (spi_master_evt.evt_type) {
	case SPI_MASTER_EVT_TRANSFER_COMPLETED:
		nrf_delay_us(10);
		break;
	default:
		nrf_delay_us(10);
		break;
	}
}

void epdSPIOn() {

	spi_master_open(SPI_MASTER_HW, &(epd.spi_config));
	spi_master_evt_handler_reg(SPI_MASTER_HW, spiEventHandler);
	epdSPIPut(0x00);
	epdSPIPut(0x00);
	nrf_delay_us(10);
}

void epdSPIOff() {
	epdSPIPut(0x00);
	epdSPIPut(0x00);
	nrf_delay_us(10);
	spi_master_close(SPI_MASTER_HW);
}


void epdSPIBuffer(uint8_t c) {
	m_tx_data[cursor] = c;
	cursor++;
}

void epdSPISend(uint8_t cs_pin, const uint8_t *buffer, uint16_t length) {
	// CS 0
	nrf_gpio_pin_write(cs_pin, 0);

	for (uint16_t i = 0; i < length; ++i) {
		m_tx_data[i] = (*buffer++);
	}

	spi_master_send_recv(SPI_MASTER_HW, m_tx_data, length, m_rx_data, 2);

	// CS 1
	nrf_gpio_pin_write(cs_pin, 1);
}

void epdPWMStart(int pin) {
	nrf_pwm_set_value(0, 50);
	nrf_pwm_set_enabled(1);
}

void epdPWMStop(int pin) {
	nrf_pwm_set_value(0, 0);
}

void epdInit(uint8_t pinCS, uint8_t pinPanelOn, uint8_t pinBorder,
		uint8_t pinDischarge, uint8_t pinPWM, uint8_t pinReset, uint8_t pinBusy,
		uint8_t pinCLK, uint8_t pinMISO, uint8_t pinMOSI) {

	epd.pinPanelOn = pinPanelOn;
	epd.pinBorder = pinBorder;
	epd.pinDischarge = pinDischarge;
	epd.pinPWM = pinPWM;
	epd.pinReset = pinReset;
	epd.pinCS = pinCS;

	nrf_gpio_cfg_output(epd.pinPanelOn);
	nrf_gpio_cfg_output(epd.pinBorder);
	nrf_gpio_cfg_output(epd.pinDischarge);
	nrf_gpio_cfg_output(epd.pinPWM);
	nrf_gpio_cfg_output(epd.pinReset);
	nrf_gpio_cfg_output(epd.pinCS);

	nrf_gpio_pin_clear(epd.pinPanelOn);
	nrf_gpio_pin_clear(epd.pinBorder);
	nrf_gpio_pin_clear(epd.pinDischarge);
	nrf_gpio_pin_clear(epd.pinPWM);
	nrf_gpio_pin_clear(epd.pinReset);

	epd.pinBusy = pinBusy;
	nrf_gpio_cfg_input(epd.pinBusy, NRF_GPIO_PIN_NOPULL);


	epd.pinCLK = pinCLK;
	epd.pinMISO = pinMISO;
	epd.pinMOSI = pinMOSI;


	epd.stageTime = 480;
	epd.linesPerDisplay = 96;
	epd.dotsPerLine = 128;
	epd.bytesPerLine = 16;
	epd.bytesPerScan = 24;

	epd.spi_config.SPI_Freq = SPI_FREQUENCY_FREQUENCY_M8; /**< SPI master frequency */
	epd.spi_config.SPI_Pin_SCK = epd.pinCLK; /**< SCK pin number. */
	epd.spi_config.SPI_Pin_MISO = epd.pinMISO; /**< MISO pin number. */
	epd.spi_config.SPI_Pin_MOSI = epd.pinMOSI; /**< MOSI pin number .*/
	epd.spi_config.SPI_Pin_SS = SPI_PIN_DISCONNECTED;
	epd.spi_config.SPI_PriorityIRQ = APP_IRQ_PRIORITY_LOW;
	epd.spi_config.SPI_DisableAllIRQ = 0;


	epd.spi_config.SPI_CONFIG_ORDER = SPI_CONFIG_ORDER_MsbFirst; /**< Bytes order LSB or MSB shifted out first. */
	epd.spi_config.SPI_CONFIG_CPOL = SPI_CONFIG_CPOL_ActiveHigh; /**< Serial clock polarity ACTIVEHIGH or ACTIVELOW. */
	epd.spi_config.SPI_CONFIG_CPHA = SPI_CONFIG_CPHA_Leading; /**< Serial clock phase LEADING or TRAILING. */

	nrf_pwm_config_t pwm_config = PWM_DEFAULT_CONFIG	;
	pwm_config.mode = PWM_MODE_BUZZER_100;
	pwm_config.num_channels = 1;
	pwm_config.gpio_num[0] = pinPWM;

	nrf_pwm_init(&pwm_config);

}

void epdBegin() {

	// power up sequence
	nrf_gpio_pin_write(epd.pinReset, 0);
	nrf_gpio_pin_write(epd.pinPanelOn, 0);
	nrf_gpio_pin_write(epd.pinDischarge, 0);
	nrf_gpio_pin_write(epd.pinBorder, 0);
	nrf_gpio_pin_write(epd.pinCS, 0);
	nrf_gpio_pin_write(epd.pinPWM, 0);
	nrf_gpio_pin_write(epd.pinPWM, 1);
	nrf_gpio_pin_write(epd.pinPWM, 0);
	nrf_gpio_pin_write(epd.pinPWM, 1);

	epdSPIOn();

	epdPWMStart(epd.pinPWM);
	nrf_delay_ms(5);                       // Al0 time for PWN to start up
	nrf_gpio_pin_write(epd.pinPanelOn, 1);
	nrf_delay_ms(10);

	nrf_gpio_pin_write(epd.pinReset, 1);
	nrf_gpio_pin_write(epd.pinCS, 1);
	nrf_delay_ms(5);

	nrf_gpio_pin_write(epd.pinReset, 0);
	nrf_delay_ms(5);

	nrf_gpio_pin_write(epd.pinReset, 1);
	nrf_delay_ms(5);

	// wait for COG to become ready
	while (1 == nrf_gpio_pin_read(epd.pinBusy)) {
	}

	// read the COG ID
	epdSPISend(epd.pinCS, CU8(0x71, 0x00), 2);

	// channel select
	nrf_delay_ms(10);
	epdSPISend(epd.pinCS, CU8(0x70, 0x01), 2);
	nrf_delay_ms(10);
	epdSPISend(epd.pinCS,
			CU8(0x72, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0x00), 9);

	// DC/DC frequency
	nrf_delay_ms(10);
	epdSPISend(epd.pinCS, CU8(0x70, 0x06), 2);
	nrf_delay_ms(10);
	epdSPISend(epd.pinCS, CU8(0x72, 0xff), 2);

	// 1 power mode osc
	nrf_delay_ms(10);
	epdSPISend(epd.pinCS, CU8(0x70, 0x07), 2);
	nrf_delay_ms(10);
	epdSPISend(epd.pinCS, CU8(0x72, 0x9d), 2);

	// disable ADC
	nrf_delay_ms(10);
	epdSPISend(epd.pinCS, CU8(0x70, 0x08), 2);
	nrf_delay_ms(10);
	epdSPISend(epd.pinCS, CU8(0x72, 0x00), 2);

	// Vcom level
	nrf_delay_ms(10);
	epdSPISend(epd.pinCS, CU8(0x70, 0x09), 2);
	nrf_delay_ms(10);
	epdSPISend(epd.pinCS, CU8(0x72, 0xd0, 0x00), 3);

	// gate and source voltage levels
	nrf_delay_ms(10);
	epdSPISend(epd.pinCS, CU8(0x70, 0x04), 2);
	nrf_delay_ms(10);
	epdSPISend(epd.pinCS, CU8(0x72, 0x03), 2);

	nrf_delay_ms(5);  //???

	// driver latch on
	nrf_delay_ms(10);
	epdSPISend(epd.pinCS, CU8(0x70, 0x03), 2);
	nrf_delay_ms(10);
	epdSPISend(epd.pinCS, CU8(0x72, 0x01), 2);

	// driver latch off
	nrf_delay_ms(10);
	epdSPISend(epd.pinCS, CU8(0x70, 0x03), 2);
	nrf_delay_ms(10);
	epdSPISend(epd.pinCS, CU8(0x72, 0x00), 2);

	nrf_delay_ms(5);

	// charge pump positive voltage on
	nrf_delay_ms(10);
	epdSPISend(epd.pinCS, CU8(0x70, 0x05), 2);
	nrf_delay_ms(10);
	epdSPISend(epd.pinCS, CU8(0x72, 0x01), 2);

	// final delay before PWM off
	nrf_delay_ms(30);
	epdPWMStop(epd.pinPWM);

	// charge pump negative voltage on
	nrf_delay_ms(10);
	epdSPISend(epd.pinCS, CU8(0x70, 0x05), 2);
	nrf_delay_ms(10);
	epdSPISend(epd.pinCS, CU8(0x72, 0x03), 2);

	nrf_delay_ms(30);

	// Vcom driver on
	nrf_delay_ms(10);
	epdSPISend(epd.pinCS, CU8(0x70, 0x05), 2);
	nrf_delay_ms(10);
	epdSPISend(epd.pinCS, CU8(0x72, 0x0f), 2);

	nrf_delay_ms(30);

	// output enable to disable
	nrf_delay_ms(10);
	epdSPISend(epd.pinCS, CU8(0x70, 0x02), 2);
	nrf_delay_ms(10);
	epdSPISend(epd.pinCS, CU8(0x72, 0x24), 2);

	epdSPIOff();
}

void epdEnd() {
	epdSPIOn();

	// dummy frame
	//EPD_frame_fixed(0x55, EPD_normal);
	epdLine(0x7fffu, 0, 0x55);


	// latch reset turn on
	nrf_delay_ms(10);
	epdSPISend(epd.pinCS, CU8(0x70, 0x03), 2);
	nrf_delay_ms(10);
	epdSPISend(epd.pinCS, CU8(0x72, 0x01), 2);

	// output enable off
	nrf_delay_ms(10);
	epdSPISend(epd.pinCS, CU8(0x70, 0x02), 2);
	nrf_delay_ms(10);
	epdSPISend(epd.pinCS, CU8(0x72, 0x05), 2);

	// Vcom power off
	nrf_delay_ms(10);
	epdSPISend(epd.pinCS, CU8(0x70, 0x05), 2);
	nrf_delay_ms(10);
	epdSPISend(epd.pinCS, CU8(0x72, 0x0e), 2);

	// power off negative charge pump
	nrf_delay_ms(10);
	epdSPISend(epd.pinCS, CU8(0x70, 0x05), 2);
	nrf_delay_ms(10);
	epdSPISend(epd.pinCS, CU8(0x72, 0x02), 2);

	// Discharge
	nrf_delay_ms(10);
	epdSPISend(epd.pinCS, CU8(0x70, 0x04), 2);
	nrf_delay_ms(10);
	epdSPISend(epd.pinCS, CU8(0x72, 0x0c), 2);

	nrf_delay_ms(120);

	// all charge pumps off
	nrf_delay_ms(10);
	epdSPISend(epd.pinCS, CU8(0x70, 0x05), 2);
	nrf_delay_ms(10);
	epdSPISend(epd.pinCS, CU8(0x72, 0x00), 2);

	// turn of osc
	nrf_delay_ms(10);
	epdSPISend(epd.pinCS, CU8(0x70, 0x07), 2);
	nrf_delay_ms(10);
	epdSPISend(epd.pinCS, CU8(0x72, 0x0d), 2);

	// Discharge internal - 1
	nrf_delay_ms(10);
	epdSPISend(epd.pinCS, CU8(0x70, 0x04), 2);
	nrf_delay_ms(10);
	epdSPISend(epd.pinCS, CU8(0x72, 0x50), 2);

	nrf_delay_ms(40);

	// Discharge internal - 2
	nrf_delay_ms(10);
	epdSPISend(epd.pinCS, CU8(0x70, 0x04), 2);
	nrf_delay_ms(10);
	epdSPISend(epd.pinCS, CU8(0x72, 0xA0), 2);

	nrf_delay_ms(40);

	// Discharge internal - 3
	nrf_delay_ms(10);
	epdSPISend(epd.pinCS, CU8(0x70, 0x04), 2);
	nrf_delay_ms(10);
	epdSPISend(epd.pinCS, CU8(0x72, 0x00), 2);
	nrf_delay_ms(10);

	// turn of power and all signals
	nrf_gpio_pin_write(epd.pinReset, 0);
	nrf_gpio_pin_write(epd.pinPanelOn, 0);
	nrf_gpio_pin_write(epd.pinBorder, 0);

	// ensure SPI MOSI and CLOCK are 0 before CS 0
	epdSPIOff();
	nrf_gpio_pin_write(epd.pinCS, 0);

	// Discharge pulse
	nrf_gpio_pin_write(epd.pinDischarge, 1);
	nrf_delay_ms(150);
	nrf_gpio_pin_write(epd.pinDischarge, 0);
}

void epdFrame(const uint8_t *image) {
	epdSPIOn();
	for (uint8_t line = 0; line < epd.linesPerDisplay; ++line) {
		epdLine(line, &image[line * epd.bytesPerLine], 0);
	}
	epdSPIOff();
}

void epdLine(uint16_t line, const uint8_t *data,
		uint8_t fixed_value) {

	int delay = 10;
	cursor = 0;
	//epdSPIon();

	// gate and source voltage levels
	nrf_delay_us(delay);
	epdSPISend(epd.pinCS, CU8(0x70, 0x04), 2);
	nrf_delay_us(delay);
	epdSPISend(epd.pinCS, CU8(0x72, 0x03), 2);

	// send data
	nrf_delay_us(delay);
	epdSPISend(epd.pinCS, CU8(0x70, 0x0a), 2);
	nrf_delay_us(delay);

	// CS 0
	nrf_gpio_pin_write(epd.pinCS, 0);
	epdSPIBuffer(0x72);
	epdSPIBuffer(0x00);

	// even pixels
	for (uint16_t b = epd.bytesPerLine; b > 0; --b) {
		if (0 != data) {
			// AVR has multiple memory spaces
			uint8_t pixels;
			pixels = data[b - 1] & 0xaa;
			pixels = 0xaa | (pixels >> 1);
			epdSPIBuffer(pixels);
		} else {
			epdSPIBuffer(fixed_value);
		}
	}

	// scan line
	for (uint16_t b = 0; b < epd.bytesPerScan; ++b) {
		if (line / 4 == b) {
			epdSPIBuffer(0xc0 >> (2 * (line & 0x03)));
		} else {
			epdSPIBuffer(0x00);
		}
	}

	// odd pixels
	for (uint16_t b = 0; b < epd.bytesPerLine; ++b) {
		if (0 != data) {
			// AVR has multiple memory spaces
			uint8_t pixels;

			pixels = data[b] & 0x55;
			pixels = 0xaa | pixels;

			uint8_t p1 = (pixels >> 6) & 0x03;
			uint8_t p2 = (pixels >> 4) & 0x03;
			uint8_t p3 = (pixels >> 2) & 0x03;
			uint8_t p4 = (pixels >> 0) & 0x03;
			pixels = (p1 << 0) | (p2 << 2) | (p3 << 4) | (p4 << 6);
			epdSPIBuffer(pixels);
		} else {
			epdSPIBuffer(fixed_value);
		}
	}

	spi_master_send_recv(SPI_MASTER_HW, m_tx_data, cursor, m_rx_data, 2);

	// CS 1
	nrf_gpio_pin_write(epd.pinCS, 1);

	// output data to panel
	nrf_delay_us(delay);
	epdSPISend(epd.pinCS, CU8(0x70, 0x02), 2);
	nrf_delay_us(delay);
	epdSPISend(epd.pinCS, CU8(0x72, 0x2f), 2);

	//epdSPIoff();
}
