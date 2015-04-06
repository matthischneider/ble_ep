/* Copyright (c) 2013 Nordic Semiconductor. All Rights Reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the license.txt file.
 */

#include "ble_display_service.h"
#include <string.h>
#include "nordic_common.h"
#include "ble_srv_common.h"
#include "app_util.h"


/**@brief Function for handling the Connect event.
 *
 * @param[in]   p_display_service       display Button Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_connect(ble_display_service_t * p_display_service, ble_evt_t * p_ble_evt)
{
    p_display_service->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
}


/**@brief Function for handling the Disconnect event.
 *
 * @param[in]   p_display_service       display Button Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_disconnect(ble_display_service_t * p_display_service, ble_evt_t * p_ble_evt)
{
    UNUSED_PARAMETER(p_ble_evt);
    p_display_service->conn_handle = BLE_CONN_HANDLE_INVALID;
}


/**@brief Function for handling the Write event.
 *
 * @param[in]   p_display_service       display Button Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_write(ble_display_service_t * p_display_service, ble_evt_t * p_ble_evt)
{
    ble_gatts_evt_write_t * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;
    uint8_t* data = p_evt_write->data;
    
   /* if ((p_evt_write->handle == p_display_service->display_char_handles.value_handle) && (p_display_service->write_handler != NULL))
    {
        p_display_service->write_handler(p_display_service, p_evt_write->data);
    }else if((p_evt_write->handle == p_display_service->pixel_char_handles.value_handle))
    {*/
        uint8_t x = data[0];
        uint8_t y = data[1];
        uint8_t w = data[2];
        if(w<=17*8){
        	for(int i = 0; i < w; i++){
        		int bytePos = (i / 8) + 3;
        		int shift = 7-(i % 8);
        		int mask = 1<<shift ;
        		gSetPixel(i+x,y,(data[bytePos] & mask)>>shift);
        	}
        }
    //}

}


void ble_display_service_on_ble_evt(ble_display_service_t * p_display_service, ble_evt_t * p_ble_evt)
{
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            on_connect(p_display_service, p_ble_evt);
            break;
            
        case BLE_GAP_EVT_DISCONNECTED:
            on_disconnect(p_display_service, p_ble_evt);
            break;
            
        case BLE_GATTS_EVT_WRITE:
            on_write(p_display_service, p_ble_evt);
            break;
            
        default:
            // No implementation needed.
            break;
    }
}


/**@brief Function for adding the display characteristic.
 *
 */
static uint32_t display_char_add(ble_display_service_t * p_display_service, const ble_display_service_init_t * p_display_service_init)
{
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_t    attr_char_value;
    ble_uuid_t          ble_uuid;
    ble_gatts_attr_md_t attr_md;

    memset(&char_md, 0, sizeof(char_md));
    
    char_md.char_props.read   = 1;
    char_md.char_props.write  = 1;
    char_md.p_char_user_desc  = "display";
    char_md.char_user_desc_size  = 7;
    char_md.char_user_desc_max_size = 7;

    char_md.p_char_pf         = NULL;
    char_md.p_user_desc_md    = NULL;
    char_md.p_cccd_md         = NULL;
    char_md.p_sccd_md         = NULL;
    
    ble_uuid.type = p_display_service->uuid_type;
    ble_uuid.uuid = DISPLAY_SERVICE_UUID_DISPLAY_CHAR;
    
    memset(&attr_md, 0, sizeof(attr_md));

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);
    attr_md.vloc       = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth    = 0;
    attr_md.wr_auth    = 0;
    attr_md.vlen       = 0;
    
    memset(&attr_char_value, 0, sizeof(attr_char_value));

    attr_char_value.p_uuid       = &ble_uuid;
    attr_char_value.p_attr_md    = &attr_md;
    attr_char_value.init_len     = 30; //sizeof(uint8_t);
    attr_char_value.init_offs    = 0;
    attr_char_value.max_len      = BLE_GATTS_FIX_ATTR_LEN_MAX; //sizeof(uint8_t);
    attr_char_value.p_value      = NULL;
    
    return sd_ble_gatts_characteristic_add(p_display_service->service_handle, &char_md,
                                               &attr_char_value,
                                               &p_display_service->display_char_handles);
}

/**@brief Function for adding the display characteristic.
 *
 */
static uint32_t pixel_char_add(ble_display_service_t * p_display_service, const ble_display_service_init_t * p_display_service_init)
{
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_t    attr_char_value;
    ble_uuid_t          ble_uuid;
    ble_gatts_attr_md_t attr_md;

    memset(&char_md, 0, sizeof(char_md));

    char_md.char_props.read   = 1;
    char_md.char_props.write  = 1;
    char_md.p_char_user_desc  = NULL;

    char_md.p_char_pf         = NULL;
    char_md.p_user_desc_md    = NULL;
    char_md.p_cccd_md         = NULL;
    char_md.p_sccd_md         = NULL;

    ble_uuid.type = p_display_service->uuid_type;
    ble_uuid.uuid = DISPLAY_SERVICE_UUID_PIXEL_CHAR;

    memset(&attr_md, 0, sizeof(attr_md));

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);
    attr_md.vloc       = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth    = 0;
    attr_md.wr_auth    = 0;
    attr_md.vlen       = 0;

    memset(&attr_char_value, 0, sizeof(attr_char_value));

    attr_char_value.p_uuid       = &ble_uuid;
    attr_char_value.p_attr_md    = &attr_md;
    attr_char_value.init_len     = 20; //sizeof(uint8_t);
    attr_char_value.init_offs    = 0;
    attr_char_value.max_len      = 20; //sizeof(uint8_t);
    attr_char_value.p_value      = NULL;

    return sd_ble_gatts_characteristic_add(p_display_service->service_handle, &char_md,
                                               &attr_char_value,
                                               &p_display_service->pixel_char_handles);
}



/**@brief Function for adding the Button characteristic.
 *
 */
static uint32_t button_char_add(ble_display_service_t * p_display_service, const ble_display_service_init_t * p_display_service_init)
{
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_md_t cccd_md;
    ble_gatts_attr_t    attr_char_value;
    ble_uuid_t          ble_uuid;
    ble_gatts_attr_md_t attr_md;

    memset(&cccd_md, 0, sizeof(cccd_md));

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);
    cccd_md.vloc = BLE_GATTS_VLOC_STACK;
    
    memset(&char_md, 0, sizeof(char_md));
    
    char_md.char_props.read   = 1;
    char_md.char_props.notify = 1;
    char_md.p_char_user_desc  = NULL;
    char_md.p_char_pf         = NULL;
    char_md.p_user_desc_md    = NULL;
    char_md.p_cccd_md         = &cccd_md;
    char_md.p_sccd_md         = NULL;
    
    ble_uuid.type = p_display_service->uuid_type;
    ble_uuid.uuid = DISPLAY_SERVICE_UUID_BUTTON_CHAR;
    
    memset(&attr_md, 0, sizeof(attr_md));

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&attr_md.write_perm);
    attr_md.vloc       = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth    = 0;
    attr_md.wr_auth    = 0;
    attr_md.vlen       = 0;
    
    memset(&attr_char_value, 0, sizeof(attr_char_value));

    attr_char_value.p_uuid       = &ble_uuid;
    attr_char_value.p_attr_md    = &attr_md;
    attr_char_value.init_len     = sizeof(uint8_t);
    attr_char_value.init_offs    = 0;
    attr_char_value.max_len      = sizeof(uint8_t);
    attr_char_value.p_value      = NULL;
    
    return sd_ble_gatts_characteristic_add(p_display_service->service_handle, &char_md,
                                               &attr_char_value,
                                               &p_display_service->button_char_handles);
}

uint32_t ble_display_service_init(ble_display_service_t * p_display_service, const ble_display_service_init_t * p_display_service_init)
{
    uint32_t   err_code;
    ble_uuid_t ble_uuid;

    // Initialize service structure
    p_display_service->conn_handle       = BLE_CONN_HANDLE_INVALID;
    p_display_service->write_handler = p_display_service_init->write_handler;
    
    // Add service
    ble_uuid128_t base_uuid = {DISPLAY_SERVICE_UUID_BASE};
    err_code = sd_ble_uuid_vs_add(&base_uuid, &p_display_service->uuid_type);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }
    
    ble_uuid.type = p_display_service->uuid_type;
    ble_uuid.uuid = DISPLAY_SERVICE_UUID_SERVICE;

    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &p_display_service->service_handle);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }
    
   /* err_code = button_char_add(p_display_service, p_display_service_init);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }*/
    
    err_code = display_char_add(p_display_service, p_display_service_init);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }
    
    err_code = pixel_char_add(p_display_service, p_display_service_init);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    return NRF_SUCCESS;
}

uint32_t ble_display_service_on_button_change(ble_display_service_t * p_display_service, uint8_t button_state)
{
    ble_gatts_hvx_params_t params;
    uint16_t len = sizeof(button_state);
    
    memset(&params, 0, sizeof(params));
    params.type = BLE_GATT_HVX_NOTIFICATION;
    params.handle = p_display_service->button_char_handles.value_handle;
    params.p_data = &button_state;
    params.p_len = &len;
    
    return sd_ble_gatts_hvx(p_display_service->conn_handle, &params);
}
