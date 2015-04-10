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

/** @file
 *
 * @defgroup ble_sdk_app_template_main main.c
 * @{
 * @ingroup ble_sdk_app_template
 * @brief Template project main file.
 *
 * This file contains a template for creating a new application. It has the code necessary to wakeup
 * from button, advertise, get a connection restart advertising on disconnect and if no new
 * connection created go back to system-off mode.
 * It can easily be used as a starting point for creating a new application, the comments identified
 * with 'YOUR_JOB' indicates where and how you can customize.
 */

#include <stdint.h>
#include "main.h"
#include "nordic_common.h"
#include "nrf.h"
#include "app_error.h"
#include "nrf_gpio.h"
#include "nrf51_bitfields.h"
#include "ble.h"
#include "ble_hci.h"
//#include "ble_srv_common.h"
#include "ble_advdata.h"
#include "ble_conn_params.h"
#include "ble_display_service.h"
#include "app_scheduler.h"
#include "softdevice_handler.h"
#include "app_timer.h"
//#include "ble_error_log.h"
#include "app_gpiote.h"
#include "app_button.h"
#include "ble_debug_assert_handler.h"
#include "pstorage.h"
#include "display.h"
#include "graphics.h"

#define IS_SRVC_CHANGED_CHARACT_PRESENT 0                                           /**< Include or not the service_changed characteristic. if not enabled, the server's database cannot be changed for the lifetime of the device*/

#define WAKEUP_BUTTON_PIN               PIN_BTN                                    /**< Button used to wake up the application. */
// YOUR_JOB: Define any other buttons to be used by the applications:
// #define MY_BUTTON_PIN                   BUTTON_1

#define DEVICE_NAME                     "Dis"                           /**< Name of device. Will be included in the advertising data. */

#define APP_ADV_INTERVAL                64                                          /**< The advertising interval (in units of 0.625 ms. This value corresponds to 40 ms). */
#define APP_ADV_TIMEOUT_IN_SECONDS      180                                         /**< The advertising timeout (in units of seconds). */

// YOUR_JOB: Modify these according to requirements.
#define APP_TIMER_PRESCALER             0                                           /**< Value of the RTC1 PRESCALER register. */
#define APP_TIMER_MAX_TIMERS            3                                           /**< Maximum number of simultaneously created timers. */
#define APP_TIMER_OP_QUEUE_SIZE         4                                           /**< Size of timer operation queues. */

#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(7.5, UNIT_1_25_MS)            /**< Minimum acceptable connection interval (0.05 seconds). */
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(40, UNIT_1_25_MS)           /**< Maximum acceptable connection interval (0.1 second). */
#define SLAVE_LATENCY                   0                                           /**< Slave latency. */
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(4000, UNIT_10_MS)             /**< Connection supervisory timeout (4 seconds). */
#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000, APP_TIMER_PRESCALER)  /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000, APP_TIMER_PRESCALER) /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                           /**< Number of attempts before giving up the connection parameter negotiation. */

#define APP_GPIOTE_MAX_USERS            1                                           /**< Maximum number of users of the GPIOTE handler. */

#define BUTTON_DETECTION_DELAY          APP_TIMER_TICKS(50, APP_TIMER_PRESCALER)    /**< Delay from a GPIOTE event until a button is reported as pushed (in number of timer ticks). */

#define SEC_PARAM_TIMEOUT               30                                          /**< Timeout for Pairing Request or Security Request (in seconds). */
#define SEC_PARAM_BOND                  1                                           /**< Perform bonding. */
#define SEC_PARAM_MITM                  0                                           /**< Man In The Middle protection not required. */
#define SEC_PARAM_IO_CAPABILITIES       BLE_GAP_IO_CAPS_NONE                        /**< No I/O capabilities. */
#define SEC_PARAM_OOB                   0                                           /**< Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE          7                                           /**< Minimum encryption key size. */
#define SEC_PARAM_MAX_KEY_SIZE          16                                          /**< Maximum encryption key size. */

#define DEAD_BEEF                       0xDEADBEEF                                  /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

static ble_gap_sec_params_t m_sec_params; /**< Security requirements for this application. */
static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID; /**< Handle of the current connection. */

#define DISPLAY_INTERVAL APP_TIMER_TICKS(1000, APP_TIMER_PRESCALER)
static app_timer_id_t m_display_timer_id;
static uint8_t blink = 0;
static char* txt[5][30];
static int txt_p = 0;
static uint32_t m_bat_voltage;
static ble_display_service_t m_display_service;
static uint8_t m_status = BLE_GAP_STATUS_DISCONNECTED;


// YOUR_JOB: Modify these according to requirements (e.g. if other event types are to pass through
//           the scheduler).
#define SCHED_MAX_EVENT_DATA_SIZE       sizeof(app_timer_event_t)                   /**< Maximum size of scheduler events. Note that scheduler BLE stack events do not contain any data, as the events are being pulled from the stack in the event handler. */
#define SCHED_QUEUE_SIZE                10                                          /**< Maximum number of events in the scheduler queue. */

// Persistent storage system event handler
void pstorage_sys_event_handler(uint32_t p_evt);

void debug(char *msg) {
	//strncpy(txt[txt_p],msg,20);

	//txt[txt_p] = msg;
	txt_p = txt_p + 1;
	if (txt_p == 5) {
		txt_p = 0;
	}
}

/**@brief Function for error handling, which is called when an error has occurred.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of error.
 *
 * @param[in] error_code  Error code supplied to the handler.
 * @param[in] line_num    Line number where the handler is called.
 * @param[in] p_file_name Pointer to the file name.
 */
void app_error_handler(uint32_t error_code, uint32_t line_num,
		const uint8_t * p_file_name) {
	//nrf_gpio_pin_set(ASSERT_LED_PIN_NO);
//TODO: Print
	// This call can be used for debug purposes during application development.
	// @note CAUTION: Activating this code will write the stack to flash on an error.
	//                This function should NOT be used in a final product.
	//                It is intended STRICTLY for development/debugging purposes.
	//                The flash write will happen EVEN if the radio is active, thus interrupting
	//                any communication.
	//                Use with care. Un-comment the line below to use.
	// ble_debug_assert_handler(error_code, line_num, p_file_name);

	// On assert, the system can only recover with a reset.
	NVIC_SystemReset();
}

/**@brief Callback function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in]   line_num   Line number of the failing ASSERT call.
 * @param[in]   file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name) {
	app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

static void sample() {
	uint32_t p_is_running = 0;

	sd_clock_hfclk_request();
	while (!p_is_running) {  				//wait for the hfclk to be available
		sd_clock_hfclk_is_running((&p_is_running));
	}
	NRF_ADC->TASKS_START = 1;							//Start ADC sampling
}

/**@brief Function for handling Service errors.
 *
 * @details A pointer to this function will be passed to each service which may need to inform the
 *          application about an error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
/*
 // YOUR_JOB: Uncomment this function and make it handle error situations sent back to your
 //           application by the services it uses.
 static void service_error_handler(uint32_t nrf_error)
 {
 APP_ERROR_HANDLER(nrf_error);
 } */

static void display_timer_timeout_handler(void * p_context) {
	UNUSED_PARAMETER(p_context);
	sample();

	//gFillRect(0, 0, 128, 96, 0);

	for (int i = 0; i < 5; i++) {
		gDrawString(0, i * 20, txt[i], 1);
	}

	switch (m_status) {
		case BLE_GAP_STATUS_ADVERTISING:
			gFillRect(0, 0, 5, 5, blink);
			break;
		case BLE_GAP_STATUS_DISCONNECTED:
			gFillRect(0, 0, 5, 5, 0);
			break;
		case BLE_GAP_STATUS_CONNECTED:
			gFillRect(0, 0, 5, 5, 1);
			break;
		default:
			break;
	}
	if (blink == 0) {
		blink = 1;
	} else {
		blink = 0;
	}

	nrf_gpio_cfg_input(PIN_BTN, NRF_GPIO_PIN_PULLUP);
	if (nrf_gpio_pin_read(PIN_BTN)) {
		gFillRect(6, 0, 5, 5, 0);
		gRect(6, 0, 5, 5, 1);
	} else {
		gFillRect(6, 0, 5, 5, 1);
	}

	//epdBegin();
	epdFrame(graphicsBuffer);
	//epdFrame(graphicsBuffer);
	//epdEnd();
}

/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module.
 */
static void timers_init(void) {
	// Initialize timer module, making it use the scheduler
	APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_MAX_TIMERS,
			APP_TIMER_OP_QUEUE_SIZE, true);
	uint32_t err_code;
	/* YOUR_JOB: Create any timers to be used by the application.
	 Below is an example of how to create a timer.
	 For every new timer needed, increase the value of the macro APP_TIMER_MAX_TIMERS by
	 one.*/
	err_code = app_timer_create(&m_display_timer_id, APP_TIMER_MODE_REPEATED,
			display_timer_timeout_handler);
	APP_ERROR_CHECK(err_code);
}

/** Configures and enables the ADC
 */
void ADC_init(void) {

	/* Enable interrupt on ADC sample ready event*/
		NRF_ADC->INTENSET = ADC_INTENSET_END_Msk;
		sd_nvic_SetPriority(ADC_IRQn, NRF_APP_PRIORITY_LOW);
		sd_nvic_EnableIRQ(ADC_IRQn);

		NRF_ADC->CONFIG	= (ADC_CONFIG_EXTREFSEL_None << ADC_CONFIG_EXTREFSEL_Pos) /* Bits 17..16 : ADC external reference pin selection. */
										| (ADC_CONFIG_PSEL_AnalogInput7 << ADC_CONFIG_PSEL_Pos)					/*!< Use analog input 2 as analog input. */
										| (ADC_CONFIG_REFSEL_VBG << ADC_CONFIG_REFSEL_Pos)							/*!< Use internal 1.2V bandgap voltage as reference for conversion. */
										| (ADC_CONFIG_INPSEL_AnalogInputNoPrescaling << ADC_CONFIG_INPSEL_Pos) /*!< Analog input specified by PSEL with no prescaling used as input for the conversion. */
										| (ADC_CONFIG_RES_10bit << ADC_CONFIG_RES_Pos);									/*!< 8bit ADC resolution. */

		/* Enable ADC*/
		NRF_ADC->ENABLE = ADC_ENABLE_ENABLE_Enabled;
}

void ADC_IRQHandler(void)
{
	/* Clear dataready event */
  NRF_ADC->EVENTS_END = 0;

  m_bat_voltage=NRF_ADC->RESULT;
  m_bat_voltage = (m_bat_voltage * 1200 / 1024);
  m_bat_voltage = m_bat_voltage * 122/22;


  //Use the STOP task to save current. Workaround for PAN_028 rev1.5 anomaly 1.
  NRF_ADC->TASKS_STOP = 1;

	//Release the external crystal
	sd_clock_hfclk_release();
}

/**@brief Function for the GAP initialization.
 *
 * @details This function sets up all the necessary GAP (Generic Access Profile) parameters of the
 *          device including the device name, appearance, and the preferred connection parameters.
 */
static void gap_params_init(void) {
	uint32_t err_code;
	ble_gap_conn_params_t gap_conn_params;
	ble_gap_conn_sec_mode_t sec_mode;

	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

	err_code = sd_ble_gap_device_name_set(&sec_mode,
			(const uint8_t *) DEVICE_NAME, strlen(DEVICE_NAME));
	APP_ERROR_CHECK(err_code);

	/* YOUR_JOB: Use an appearance value matching the application's use case.
	 err_code = sd_ble_gap_appearance_set(BLE_APPEARANCE_);
	 APP_ERROR_CHECK(err_code); */

	memset(&gap_conn_params, 0, sizeof(gap_conn_params));

	gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
	gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
	gap_conn_params.slave_latency = SLAVE_LATENCY;
	gap_conn_params.conn_sup_timeout = CONN_SUP_TIMEOUT;

	err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
	APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing the Advertising functionality.
 *
 * @details Encodes the required advertising data and passes it to the stack.
 *          Also builds a structure to be passed to the stack when starting advertising.
 */
static void advertising_init(void) {
	uint32_t err_code;
	ble_advdata_t advdata;
	uint8_t flags = BLE_GAP_ADV_FLAGS_LE_ONLY_LIMITED_DISC_MODE;

	// YOUR_JOB: Use UUIDs for service(s) used in your application.
	ble_uuid_t adv_uuids[] = {{DISPLAY_SERVICE_UUID_SERVICE, m_display_service.uuid_type}};

	// Build and set advertising data
	memset(&advdata, 0, sizeof(advdata));

	advdata.name_type = BLE_ADVDATA_FULL_NAME;
	advdata.include_appearance = true;
	advdata.flags.size = sizeof(flags);
	advdata.flags.p_data = &flags;
	advdata.uuids_complete.uuid_cnt = sizeof(adv_uuids) / sizeof(adv_uuids[0]);
	advdata.uuids_complete.p_uuids = adv_uuids;

	err_code = ble_advdata_set(&advdata, NULL);
	APP_ERROR_CHECK(err_code);
}

static void display_write_handler(ble_display_service_t * p_display_service, char *msg)
{
    debug(msg);
}

/**@brief Function for initializing services that will be used by the application.
 */
static void services_init(void) {
	uint32_t err_code;
	    ble_display_service_init_t init;

	    init.write_handler = display_write_handler;

	    err_code = ble_display_service_init(&m_display_service, &init);
	    APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing security parameters.
 */
static void sec_params_init(void) {
	m_sec_params.timeout = SEC_PARAM_TIMEOUT;
	m_sec_params.bond = SEC_PARAM_BOND;
	m_sec_params.mitm = SEC_PARAM_MITM;
	m_sec_params.io_caps = SEC_PARAM_IO_CAPABILITIES;
	m_sec_params.oob = SEC_PARAM_OOB;
	m_sec_params.min_key_size = SEC_PARAM_MIN_KEY_SIZE;
	m_sec_params.max_key_size = SEC_PARAM_MAX_KEY_SIZE;
}

/**@brief Function for handling the Connection Parameters Module.
 *
 * @details This function will be called for all events in the Connection Parameters Module which
 *          are passed to the application.
 *          @note All this function does is to disconnect. This could have been done by simply
 *                setting the disconnect_on_fail config parameter, but instead we use the event
 *                handler mechanism to demonstrate its use.
 *
 * @param[in]   p_evt   Event received from the Connection Parameters Module.
 */
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt) {
	uint32_t err_code;

	if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED) {
		err_code = sd_ble_gap_disconnect(m_conn_handle,
		BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
		APP_ERROR_CHECK(err_code);
	}
}

/**@brief Function for handling a Connection Parameters error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error) {
	APP_ERROR_HANDLER(nrf_error);
}

/**@brief Function for initializing the Connection Parameters module.
 */
static void conn_params_init(void) {
	uint32_t err_code;
	ble_conn_params_init_t cp_init;

	memset(&cp_init, 0, sizeof(cp_init));

	cp_init.p_conn_params = NULL;
	cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
	cp_init.next_conn_params_update_delay = NEXT_CONN_PARAMS_UPDATE_DELAY;
	cp_init.max_conn_params_update_count = MAX_CONN_PARAMS_UPDATE_COUNT;
	cp_init.start_on_notify_cccd_handle = BLE_GATT_HANDLE_INVALID;
	cp_init.disconnect_on_fail = false;
	cp_init.evt_handler = on_conn_params_evt;
	cp_init.error_handler = conn_params_error_handler;

	err_code = ble_conn_params_init(&cp_init);
	APP_ERROR_CHECK(err_code);
}

/**@brief Function for starting timers.
 */
static void timers_start(void) {
	uint32_t err_code;
	/* YOUR_JOB: Start your timers. below is an example of how to start a timer.
	 uint32_t err_code;
	 */
	err_code = app_timer_start(m_display_timer_id, DISPLAY_INTERVAL, NULL);
	APP_ERROR_CHECK(err_code);
}

/**@brief Function for starting advertising.
 */
static void advertising_start(void) {
	uint32_t err_code;
	ble_gap_adv_params_t adv_params;

	// Start advertising
	memset(&adv_params, 0, sizeof(adv_params));

	adv_params.type = BLE_GAP_ADV_TYPE_ADV_IND;
	adv_params.p_peer_addr = NULL;
	adv_params.fp = BLE_GAP_ADV_FP_ANY;
	adv_params.interval = APP_ADV_INTERVAL;
	adv_params.timeout = APP_ADV_TIMEOUT_IN_SECONDS;

	err_code = sd_ble_gap_adv_start(&adv_params);
	APP_ERROR_CHECK(err_code);
	m_status = BLE_GAP_STATUS_ADVERTISING;
}

/**@brief Function for handling the Application's BLE Stack events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 */
static void on_ble_evt(ble_evt_t * p_ble_evt) {
	uint32_t err_code;
	static ble_gap_evt_auth_status_t m_auth_status;
	ble_gap_enc_info_t * p_enc_info;
	switch (p_ble_evt->header.evt_id) {
	case BLE_GAP_EVT_CONNECTED:
		m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
		/* YOUR_JOB: Uncomment this part if you are using the app_button module to handle button
		 events (assuming that the button events are only needed in connected
		 state). If this is uncommented out here,
		 1. Make sure that app_button_disable() is called when handling
		 BLE_GAP_EVT_DISCONNECTED below.
		 2. Make sure the app_button module is initialized.
		 err_code = app_button_enable();
		 APP_ERROR_CHECK(err_code);
		 */
		m_status = BLE_GAP_STATUS_CONNECTED;
		break;

	case BLE_GAP_EVT_DISCONNECTED:
		//TODO: Print
		m_conn_handle = BLE_CONN_HANDLE_INVALID;

		/* YOUR_JOB: Uncomment this part if you are using the app_button module to handle button
		 events. This should be done to save power when not connected
		 to a peer.
		 err_code = app_button_disable();
		 APP_ERROR_CHECK(err_code);
		 */
		m_status = BLE_GAP_STATUS_DISCONNECTED;
		advertising_start();
		break;

	case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
		err_code = sd_ble_gap_sec_params_reply(m_conn_handle,
		BLE_GAP_SEC_STATUS_SUCCESS, &m_sec_params);
		APP_ERROR_CHECK(err_code);
		break;

	case BLE_GATTS_EVT_SYS_ATTR_MISSING:
		err_code = sd_ble_gatts_sys_attr_set(m_conn_handle, NULL, 0);
		APP_ERROR_CHECK(err_code);
		break;

	case BLE_GAP_EVT_AUTH_STATUS:
		m_auth_status = p_ble_evt->evt.gap_evt.params.auth_status;
		break;

	case BLE_GAP_EVT_SEC_INFO_REQUEST:
		p_enc_info = &m_auth_status.periph_keys.enc_info;
		if (p_enc_info->div
				== p_ble_evt->evt.gap_evt.params.sec_info_request.div) {
			err_code = sd_ble_gap_sec_info_reply(m_conn_handle, p_enc_info,
			NULL);
			APP_ERROR_CHECK(err_code);
		} else {
			// No keys found for this device
			err_code = sd_ble_gap_sec_info_reply(m_conn_handle, NULL, NULL);
			APP_ERROR_CHECK(err_code);
		}
		break;

	case BLE_GAP_EVT_TIMEOUT:

		if (p_ble_evt->evt.gap_evt.params.timeout.src
				== BLE_GAP_TIMEOUT_SRC_ADVERTISEMENT) {
			m_status = BLE_GAP_STATUS_DISCONNECTED;

			// Configure buttons with sense level low as wakeup source.
			nrf_gpio_cfg_sense_input(WAKEUP_BUTTON_PIN,	BUTTON_PULL, NRF_GPIO_PIN_SENSE_LOW);

			// Go to system-off mode (this function will not return; wakeup will cause a reset)
			epdEnd();
			err_code = sd_power_system_off();
			APP_ERROR_CHECK(err_code);

		}
		break;

	default:
		// No implementation needed.
		break;
	}
}

/**@brief Function for dispatching a BLE stack event to all modules with a BLE stack event handler.
 *
 * @details This function is called from the scheduler in the main loop after a BLE stack
 *          event has been received.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 */
static void ble_evt_dispatch(ble_evt_t * p_ble_evt) {
	on_ble_evt(p_ble_evt);
	ble_conn_params_on_ble_evt(p_ble_evt);
	ble_display_service_on_ble_evt(&m_display_service, p_ble_evt);
}

/**@brief Function for dispatching a system event to interested modules.
 *
 * @details This function is called from the System event interrupt handler after a system
 *          event has been received.
 *
 * @param[in]   sys_evt   System stack event.
 */
static void sys_evt_dispatch(uint32_t sys_evt) {
	pstorage_sys_event_handler(sys_evt);
}

/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void) {
	uint32_t err_code;

	// Initialize the SoftDevice handler module.
	SOFTDEVICE_HANDLER_INIT(NRF_CLOCK_LFCLKSRC_XTAL_20_PPM, false);

	// Enable BLE stack
	ble_enable_params_t ble_enable_params;
	memset(&ble_enable_params, 0, sizeof(ble_enable_params));
	ble_enable_params.gatts_enable_params.service_changed =
	IS_SRVC_CHANGED_CHARACT_PRESENT;
	err_code = sd_ble_enable(&ble_enable_params);
	APP_ERROR_CHECK(err_code);

	// Register with the SoftDevice handler module for BLE events.
	err_code = softdevice_ble_evt_handler_set(ble_evt_dispatch);
	APP_ERROR_CHECK(err_code);

	// Register with the SoftDevice handler module for BLE events.
	err_code = softdevice_sys_evt_handler_set(sys_evt_dispatch);
	APP_ERROR_CHECK(err_code);
}

/**@brief Function for the Event Scheduler initialization.
 */
static void scheduler_init(void) {
	APP_SCHED_INIT(SCHED_MAX_EVENT_DATA_SIZE, SCHED_QUEUE_SIZE);
}

/**@brief Function for handling a button event.
 *
 * @param[in]   pin_no         Pin that had an event happen.
 * @param[in]   button_event   APP_BUTTON_PUSH or APP_BUTTON_RELEASE.
 */
/* YOUR_JOB: Uncomment this function if you need to handle button events.
 static void button_event_handler(uint8_t pin_no, uint8_t button_event)
 {
 if (button_action == APP_BUTTON_PUSH)
 {
 switch (pin_no)
 {
 case MY_BUTTON_PIN:
 // Code to handle MY_BUTTON keypresses
 break;

 // Handle any other buttons

 default:
 APP_ERROR_HANDLER(pin_no);
 break;
 }
 }
 }
 */

/**@brief Function for initializing the GPIOTE handler module.
 */
static void gpiote_init(void) {
	APP_GPIOTE_INIT(APP_GPIOTE_MAX_USERS);
}

/**@brief Function for initializing the button handler module.
 */
static void buttons_init(void) {
	// Note: Array must be static because a pointer to it will be saved in the Button handler
	//       module.
	static app_button_cfg_t buttons[] = { { WAKEUP_BUTTON_PIN,
	APP_BUTTON_ACTIVE_LOW, BUTTON_PULL, NULL },
	// YOUR_JOB: Add other buttons to be used:
	// {MY_BUTTON_PIN,     false, BUTTON_PULL, button_event_handler}
			};

	APP_BUTTON_INIT(buttons, sizeof(buttons) / sizeof(buttons[0]),
			BUTTON_DETECTION_DELAY, true);

	// Note: If the only use of buttons is to wake up, the app_button module can be omitted, and
	//       the wakeup button can be configured by
	// GPIO_WAKEUP_BUTTON_CONFIG(WAKEUP_BUTTON_PIN);
}

/**@brief Function for the Power manager.
 */
static void power_manage(void) {
	uint32_t err_code = sd_app_evt_wait();
	APP_ERROR_CHECK(err_code);
}

static void display_init(void) {
	epdInit(PIN_CS, PIN_PANEL_ON, PIN_BORDER, PIN_DISCHARG, PIN_PWM,
	PIN_RESET, PIN_BUSY, PIN_SCK, PIN_MISO, PIN_MOSI);
}

/**@brief Function for application main entry.
 */
int main(void) {
	// Initialize
	display_init();
	timers_init();
	gpiote_init();
	buttons_init();
	ble_stack_init();
	scheduler_init();
	gap_params_init();
	services_init();
	advertising_init();
	conn_params_init();
	sec_params_init();
	ADC_init();
	epdBegin();
	extern uint32_t __HeapLimit;
	int i = __HeapLimit;
	// Start execution
	timers_start();
	advertising_start();

	// Enter main loop

	for (;;) {
		app_sched_execute();
		power_manage();
	}
	//epdEnd();
}



/**
 * @}
 */
