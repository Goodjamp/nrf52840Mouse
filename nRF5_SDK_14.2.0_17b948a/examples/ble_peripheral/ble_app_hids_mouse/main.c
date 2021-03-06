/**
 * Copyright (c) 2014 - 2017, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/** @file
 *
 * @defgroup ble_sdk_app_hids_mouse_main main.c
 * @{
 * @ingroup ble_sdk_app_hids_mouse
 * @brief HID Mouse Sample Application main file.
 *
 * This file contains is the source code for a sample application using the HID, Battery and Device
 * Information Service for implementing a simple mouse functionality. This application uses the
 * @ref app_scheduler.
 *
 * Also it would accept pairing requests from any peer device. This implementation of the
 * application will not know whether a connected central is a known device or not.
 */

#include <stdint.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf.h"
#include "nrf_sdm.h"
#include "app_error.h"
#include "ble.h"
#include "ble_err.h"
#include "ble_hci.h"
#include "ble_srv_common.h"
#include "ble_advdata.h"
#include "ble_hids.h"
#include "ble_bas.h"
#include "ble_dis.h"
#include "ble_conn_params.h"
#include "sensorsim.h"
#include "bsp_btn_ble.h"
#include "app_scheduler.h"
#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
#include "nrf_sdh_ble.h"
#include "app_timer.h"
#include "peer_manager.h"
#include "ble_advertising.h"
#include "fds.h"
#include "ble_conn_state.h"
#include "nrf_ble_gatt.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"


#define DEVICE_NAME                     "Nordic_Mouse"                              /**< Name of device. Will be included in the advertising data. */
#define MANUFACTURER_NAME               "NordicSemiconductor"                       /**< Manufacturer. Will be passed to Device Information Service. */

#define APP_BLE_OBSERVER_PRIO           3                                           /**< Application's BLE observer priority. You shouldn't need to modify this value. */
#define APP_BLE_CONN_CFG_TAG            1                                           /**< A tag identifying the SoftDevice BLE configuration. */

#define BATTERY_LEVEL_MEAS_INTERVAL     APP_TIMER_TICKS(2000)                       /**< Battery level measurement interval (ticks). */
#define MIN_BATTERY_LEVEL               81                                          /**< Minimum simulated battery level. */
#define MAX_BATTERY_LEVEL               100                                         /**< Maximum simulated battery level. */
#define BATTERY_LEVEL_INCREMENT         1                                           /**< Increment between each simulated battery level measurement. */

#define PNP_ID_VENDOR_ID_SOURCE         0x02                                        /**< Vendor ID Source. */
#define PNP_ID_VENDOR_ID                0x1915                                      /**< Vendor ID. */
#define PNP_ID_PRODUCT_ID               0xEEEE                                      /**< Product ID. */
#define PNP_ID_PRODUCT_VERSION          0x0001                                      /**< Product Version. */

/*lint -emacro(524, MIN_CONN_INTERVAL) // Loss of precision */
#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(7.5, UNIT_1_25_MS)            /**< Minimum connection interval (7.5 ms). */
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(15, UNIT_1_25_MS)             /**< Maximum connection interval (15 ms). */
#define SLAVE_LATENCY                   20                                          /**< Slave latency. */
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(3000, UNIT_10_MS)             /**< Connection supervisory timeout (3000 ms). */

#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000)                       /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000)                      /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAM_UPDATE_COUNT     3                                           /**< Number of attempts before giving up the connection parameter negotiation. */

#define SEC_PARAM_BOND                  1                                           /**< Perform bonding. */
#define SEC_PARAM_MITM                  0                                           /**< Man In The Middle protection not required. */
#define SEC_PARAM_LESC                  0                                           /**< LE Secure Connections not enabled. */
#define SEC_PARAM_KEYPRESS              0                                           /**< Keypress notifications not enabled. */
#define SEC_PARAM_IO_CAPABILITIES       BLE_GAP_IO_CAPS_NONE                        /**< No I/O capabilities. */
#define SEC_PARAM_OOB                   0                                           /**< Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE          7                                           /**< Minimum encryption key size. */
#define SEC_PARAM_MAX_KEY_SIZE          16                                          /**< Maximum encryption key size. */

#define MOVEMENT_SPEED                  5                                           /**< Number of pixels by which the cursor is moved each time a button is pushed. */
#define INPUT_REPORT_COUNT              3                                           /**< Number of input reports in this application. */
#define INPUT_REP_BUTTONS_LEN           3                                           /**< Length of Mouse Input Report containing button data. */
#define INPUT_REP_MOVEMENT_LEN          3                                           /**< Length of Mouse Input Report containing movement data. */
#define INPUT_REP_MEDIA_PLAYER_LEN      1                                           /**< Length of Mouse Input Report containing media player data. */
#define INPUT_REP_BUTTONS_INDEX         0                                           /**< Index of Mouse Input Report containing button data. */
#define INPUT_REP_MOVEMENT_INDEX        1                                           /**< Index of Mouse Input Report containing movement data. */
#define INPUT_REP_MPLAYER_INDEX         2                                           /**< Index of Mouse Input Report containing media player data. */
#define INPUT_REP_REF_BUTTONS_ID        1                                           /**< Id of reference to Mouse Input Report containing button data. */
#define INPUT_REP_REF_MOVEMENT_ID       2                                           /**< Id of reference to Mouse Input Report containing movement data. */
#define INPUT_REP_REF_MPLAYER_ID        3                                           /**< Id of reference to Mouse Input Report containing media player data. */

#define BASE_USB_HID_SPEC_VERSION       0x0101                                      /**< Version number of base USB HID Specification implemented by this application. */

#define SCHED_MAX_EVENT_DATA_SIZE       APP_TIMER_SCHED_EVENT_DATA_SIZE             /**< Maximum size of scheduler events. */
#ifdef SVCALL_AS_NORMAL_FUNCTION
#define SCHED_QUEUE_SIZE                20                                          /**< Maximum number of events in the scheduler queue. More is needed in case of Serialization. */
#else
#define SCHED_QUEUE_SIZE                10                                          /**< Maximum number of events in the scheduler queue. */
#endif

#define DEAD_BEEF                       0xDEADBEEF                                  /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

#define APP_FEATURE_NOT_SUPPORTED       BLE_GATT_STATUS_ATTERR_APP_BEGIN + 2        /**< Reply when unsupported features are requested. */

#define APP_ADV_FAST_INTERVAL           0x0028                                      /**< Fast advertising interval (in units of 0.625 ms. This value corresponds to 25 ms.). */
#define APP_ADV_SLOW_INTERVAL           0x0C80                                      /**< Slow advertising interval (in units of 0.625 ms. This value corrsponds to 2 seconds). */
#define APP_ADV_FAST_TIMEOUT            10                                          /**< The duration of the fast advertising period (in seconds). */
#define APP_ADV_SLOW_TIMEOUT            10                                         /**< The duration of the slow advertising period (in seconds). */


APP_TIMER_DEF(m_battery_timer_id);                                                  /**< Battery timer. */
BLE_BAS_DEF(m_bas);                                                                 /**< Battery service instance. */
BLE_HIDS_DEF(m_hids);                                                               /**< HID service instance. */
NRF_BLE_GATT_DEF(m_gatt);                                                           /**< GATT module instance. */
BLE_ADVERTISING_DEF(m_advertising);                                                 /**< Advertising module instance. */

static bool              m_in_boot_mode = false;                                    /**< Current protocol mode. */
static uint16_t          m_conn_handle  = BLE_CONN_HANDLE_INVALID;                  /**< Handle of the current connection. */
static pm_peer_id_t      m_peer_id;                                                 /**< Device reference handle to the current bonded central. */
static sensorsim_cfg_t   m_battery_sim_cfg;                                         /**< Battery Level sensor simulator configuration. */
static sensorsim_state_t m_battery_sim_state;                                       /**< Battery Level sensor simulator state. */
static pm_peer_id_t      m_whitelist_peers[BLE_GAP_WHITELIST_ADDR_MAX_COUNT];       /**< List of peers currently in the whitelist. */
static uint32_t          m_whitelist_peer_cnt;                                      /**< Number of peers currently in the whitelist. */
static ble_uuid_t        m_adv_uuids[] =                                            /**< Universally unique service identifiers. */
{
    {BLE_UUID_HUMAN_INTERFACE_DEVICE_SERVICE, BLE_UUID_TYPE_BLE}
};


static void on_hids_evt(ble_hids_t * p_hids, ble_hids_evt_t * p_evt);
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context);
static void pm_evt_handler(pm_evt_t const * p_evt);
static void on_adv_evt(ble_adv_evt_t ble_adv_evt);
static void ble_advertising_error_handler(uint32_t nrf_error);
static void peer_list_get(pm_peer_id_t * p_peers, uint32_t * p_size);


/**@brief Function for starting advertising for direct connect to device according order.
 *        This process include two phase: scanning, one item white list advertising
          - scanning:   high density advertising with rejection ALL input connection for detect device from white list around
          - connection: advertising with ONE WHITE LIST device and serially switch device on white list
          I should add state machine for global control advertising process: power on adv, normal adv and so on
          On processing of power On adv I should switch phase of adv.

 */











#include "fds.h"

#include "orderProcessing.h"
#include "systemTime.h"

#define DETECTED_DEV_FREE    0xFF
#define DIRECT_CONN_QUANTITY 0x3

#define FILE_ORDER                               0xBAAB  /* The ID of the file to write the records into. */
#define RECORD_KEY_ORDER                         0xABBA  /* A key for the second record. */

#define APP_ADV_GLOBAL_INTERVAL           0x0023      /**< Fast advertising interval (in units of 0.625 ms. This value corresponds to 21 ms.). */
#define APP_ADV_GLOBAL_TIMEOUT              60     /**< The duration of the fast advertising period (in seconds). */


#define APP_ADV_FAST_SCANING_INTERVAL           0x0023      /**< Fast advertising interval (in units of 0.625 ms. This value corresponds to 21 ms.). */
#define APP_ADV_FAST_SCANING_TIMEOUT                10     /**< The duration of the fast advertising period (in seconds). */

#define APP_ADV_FAST_CONNECT_INTERVAL           0x0023      /**< Fast advertising interval (in units of 0.625 ms. This value corresponds to 21 ms.). */
#define APP_ADV_FAST_CONNECT_TIMEOUT                10     /**< The duration of the fast advertising period (in seconds). */

#define APP_ADV_FAST_SEARCH_INTERVAL           0x0140      /**< Fast advertising interval (in units of 0.625 ms. This value corresponds to 200 ms.). */
#define APP_ADV_FAST_SEARCH_TIMEOUT                60     /**< The duration of the fast advertising period (in seconds). */

#define ORDER_FLASHE_PAGE                         254

#define ADV_RECONNECT_SCAN_TIMEOUT               10000

#define GET_PAGE_ADDRESS(X)  (uint32_t)(X*4096)


typedef enum
{
    ADV_PROC_START_CONNECT,
    ADV_PROC_STOP_ADV,
    ADV_PROC_CONNECT_PREV,
    ADV_PROC_CONNECT
}nrfBLEAdvEvT;

typedef enum
{
    DEV_SCANNING,
    DEV_CONNECTION,
}connPhase;

typedef enum
{
    ADV_IDLE,              // Mo advertising
    ADV_ADD_NEW,           // after press connect pushbutton
    ADV_RECONNECT_SCAN,    // after power on with bonds    OR after disconnect
    ADV_RECONNECT_CONNECT,
}advTypeT;

typedef enum
{
    CONNECTION_CONNECT          = 0x0,
    CONNECTION_START_DISCONNECT = 0x1,
    CONNECTION_DISCONNECT       = 0x2,
}connectionStateT;


struct
{
    connPhase phase;
    uint16_t  listDetectedDev[BLE_GAP_WHITELIST_ADDR_MAX_COUNT];
    uint8_t   quantityDetectedDev;
    uint8_t   connectCnt;
    uint8_t   loopCnt;
}advDirOrdConnState =
{
    .listDetectedDev = {[0 ... BLE_GAP_WHITELIST_ADDR_MAX_COUNT-1] = DETECTED_DEV_FREE}
};

orderT        deviceOrder;
timerCallbacT stopScanAdvTimerCallback;
timerCallbacT switchToConnAdvTimerCallback;
struct{
    bool             isDeleteBonds;
    bool             isRealAdv;
    bool             isAppAdv;
    bool             isPrevConn;
    uint16_t         connectionHandler;
    advTypeT         currentAdvType;
    connectionStateT connectState;
} appState =
{
    .isDeleteBonds    = false,                  // do we need clear bonds
    .isRealAdv        = false,                  // real state of advertising
    .isAppAdv         = false,                  // is advertising: true, false
    .isPrevConn       = false,                  // is current device was previous connected
    .connectionHandler = BLE_CONN_HANDLE_INVALID,
    .currentAdvType   = ADV_IDLE,               // current advertising state
    .connectState     = CONNECTION_DISCONNECT,  // current connection state
};
uint32_t scaningTimeout;

bool appAdvGetPrevConn(void)
{
    return appState.isPrevConn;
}


void appAdvSetPrevConn(bool prevConnSet)
{
    appState.isPrevConn = prevConnSet;
}


advTypeT appAdvGetCurrentType(void)
{
    return appState.currentAdvType;
}


bool appAdvGetState(void)
{
    return appState.isAppAdv;
}


void appAdvSetRealState(bool state)
{
    appState.isRealAdv = state;
}


void appConnectSetState(connectionStateT connState, uint16_t connectionHandler)
{
    appState.connectionHandler = connectionHandler;
    appState.connectState      = connState;
}


void appDisconnect()
{
    ret_code_t ret;
    if(appState.connectState != CONNECTION_CONNECT)
    {
        NRF_LOG_INFO("DISCONNECT: disconnected");
        return;
    }
    NRF_LOG_INFO("DISCONNECT: start disconnect");
    appState.connectState = CONNECTION_START_DISCONNECT;
    appState.isRealAdv = true;
    ret = sd_ble_gap_disconnect(appState.connectionHandler, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
    NRF_LOG_INFO("DISCONNECT: ret = %d", ret);
    APP_ERROR_CHECK(ret);
    NRF_LOG_INFO("DISCONNECT: ok");
}


void appAdvStop(void)
{
    ret_code_t ret;
    NRF_LOG_INFO("ADV STOP:");
    switch(appState.connectState)
    {
    case CONNECTION_CONNECT:
        appState.isAppAdv = false;
        NRF_LOG_INFO("ADV STOP: already stoped");
        break;
    case CONNECTION_START_DISCONNECT:
        appState.isAppAdv = false;
        NRF_LOG_INFO("ADV STOP: disconnected process");
        break;
    case CONNECTION_DISCONNECT:
        appState.isAppAdv = false;
        if(!appState.isRealAdv)
        {
            NRF_LOG_INFO("ADV STOP: current stop");
            return;
        }
        appState.isRealAdv = false;
        ret = sd_ble_gap_adv_stop();
        NRF_LOG_INFO("ADV STOP: ret = %d", ret);
        APP_ERROR_CHECK(ret);
        NRF_LOG_INFO("ADV STOP: ok");
        break;
    default:
        break;
    }
    NRF_LOG_INFO("ADV STOP: ok");

}


void appAdvStart(void)
{
    ret_code_t ret;
    switch(appState.connectState)
    {
        case CONNECTION_CONNECT:
            // after disconnect nrf automatically start advertising
            appState.isAppAdv = true;
            NRF_LOG_INFO("ADV START: disconnect connection");
            //ret = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            appDisconnect();
            break;
        case CONNECTION_START_DISCONNECT:
            appState.isAppAdv = true;
            NRF_LOG_INFO("ADV START: wait disconnect");
            break;
        case CONNECTION_DISCONNECT:
            if(appState.isRealAdv)
            {
                NRF_LOG_INFO("ADV START: running state");
                break;
            }
            appState.isAppAdv  = true;
            appState.isRealAdv = true;
            NRF_LOG_INFO("ADV START: start");
            ret = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);
            NRF_LOG_INFO("ADV START: ret = %d", ret);
            APP_ERROR_CHECK(ret);
            break;
        default: break;
    }
    NRF_LOG_INFO("ADV START: ok");
}


static void advertising_init(uint16_t intervalMSeconds, uint16_t periodSeconds, bool whitelistEnabled)
{
    ret_code_t err_code;
    uint8_t    adv_flags;
    ble_advertising_init_t init;

    memset(&init, 0, sizeof(init));

    adv_flags                            = BLE_GAP_ADV_FLAGS_LE_ONLY_LIMITED_DISC_MODE;
    init.advdata.name_type               = BLE_ADVDATA_FULL_NAME;
    init.advdata.include_appearance      = true;
    init.advdata.flags                   = adv_flags;
    init.advdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    init.advdata.uuids_complete.p_uuids  = m_adv_uuids;

    init.config.ble_adv_on_disconnect_disabled = false;
    init.config.ble_adv_whitelist_enabled      = whitelistEnabled;
    init.config.ble_adv_directed_enabled       = false;
    init.config.ble_adv_directed_slow_enabled  = false;
    init.config.ble_adv_directed_slow_interval = APP_ADV_FAST_INTERVAL;
    init.config.ble_adv_directed_slow_timeout  = APP_ADV_FAST_TIMEOUT;
    init.config.ble_adv_fast_enabled           = true;
    init.config.ble_adv_fast_interval          = intervalMSeconds;
    init.config.ble_adv_fast_timeout           = periodSeconds;
    init.config.ble_adv_slow_enabled           = false;
    init.config.ble_adv_slow_interval          = APP_ADV_SLOW_INTERVAL;
    init.config.ble_adv_slow_timeout           = APP_ADV_SLOW_TIMEOUT;

    init.evt_handler   = on_adv_evt;
    init.error_handler = ble_advertising_error_handler;

    err_code = ble_advertising_init(&m_advertising, &init);
    APP_ERROR_CHECK(err_code);

    ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);
}


bool appPeerGetIdInList(pm_peer_id_t peerIdIn)
{
    pm_peer_id_t peer_id;

    peer_id = pm_next_peer_id_get(PM_PEER_ID_INVALID);

    while (peer_id != PM_PEER_ID_INVALID )
    {
        if( peerIdIn == peer_id)
        {
            return true;
        }
        peer_id = pm_next_peer_id_get(peer_id);
    }
    return false;
}


static void appAdvSetStart(advTypeT advType, uint16_t peerId)
{
    ret_code_t ret;

    switch(advType)
    {
    case ADV_ADD_NEW: // advertising without white list, but REJECT connection all current peers
        NRF_LOG_INFO("ADV add new");
        appState.currentAdvType = ADV_ADD_NEW;

 /*      appAdvStop();  // use this stop adv for activate white list with new devices

        m_whitelist_peer_cnt = 0;
       ret = pm_whitelist_set(NULL, 0);
        APP_ERROR_CHECK(ret);
        ret = pm_device_identities_list_set(NULL, 0);
        if (ret != NRF_ERROR_NOT_SUPPORTED)
        {
            APP_ERROR_CHECK(ret);
        }


        if(m_conn_handle != BLE_CONN_HANDLE_INVALID)
        {
            NRF_LOG_INFO("----start adv DISCONNECT-----");
            appAdvStart();
        }
        else
        {
            NRF_LOG_INFO("----start adv ADV-----");
            appAdvStop();  // use this stop adv for activate white list with new devices
            appAdvStart();
            APP_ERROR_CHECK(ret);
        }
        return;
        break;
*/
        m_whitelist_peer_cnt = 0;

        break;
    case ADV_RECONNECT_SCAN:
        memset(m_whitelist_peers, PM_PEER_ID_INVALID, sizeof(m_whitelist_peers));
        m_whitelist_peer_cnt = (sizeof(m_whitelist_peers) / sizeof(pm_peer_id_t));
        peer_list_get(m_whitelist_peers, &m_whitelist_peer_cnt);

        NRF_LOG_INFO("Adv recon: %d", m_whitelist_peer_cnt);


        if(m_whitelist_peer_cnt == 0) // if no peer devices -
        {
            appState.currentAdvType = ADV_ADD_NEW;
        }
        else
        {
            appState.currentAdvType = ADV_RECONNECT_SCAN;
            timerRun(stopScanAdvTimerCallback, ADV_RECONNECT_SCAN_TIMEOUT);
            scaningTimeout = getTime();
        }
        break;
    case ADV_RECONNECT_CONNECT:
        appState.currentAdvType = ADV_RECONNECT_CONNECT;
        // set white list parameters
        NRF_LOG_INFO("Adv recon conn");
        m_whitelist_peers[0] = peerId;
        m_whitelist_peer_cnt = 1;
        break;
    default:
        break;
    }
    appAdvStop();  // use this stop adv for activate white list with new devices

    ret = pm_whitelist_set((m_whitelist_peer_cnt == 0 ) ? (NULL) : (m_whitelist_peers),
                           (m_whitelist_peer_cnt == 0 ) ? (0)    : (m_whitelist_peer_cnt));
    ret = pm_device_identities_list_set((m_whitelist_peer_cnt == 0 ) ? (NULL) : (m_whitelist_peers),
                                        (m_whitelist_peer_cnt == 0 ) ? (0)    : (m_whitelist_peer_cnt));
    NRF_LOG_INFO("ret = %d", ret);

    if (ret != NRF_ERROR_NOT_SUPPORTED)
    {
        APP_ERROR_CHECK(ret);
    }
    appAdvStart();
}


void appAdvAddNew(nrfBLEAdvEvT inEv, pm_peer_id_t peerId)
{
    switch(inEv)
    {
    case ADV_PROC_START_CONNECT:
        break;
    case ADV_PROC_STOP_ADV:
        break;
    case ADV_PROC_CONNECT_PREV:
        NRF_LOG_INFO("Disconnect old dev");
        appDisconnect();
        break;
    case ADV_PROC_CONNECT:

       // if(!appPeerGetIdInList(peerId))
       // {
            NRF_LOG_INFO("No device on the list");
            appAdvStop();
            break;
       // }


    }

}


void appAdvDirectProc(nrfBLEAdvEvT inEv, pm_peer_id_t peerId)
{
    ret_code_t err_code;
    switch(inEv)
    {
    case ADV_PROC_START_CONNECT: // 1
        NRF_LOG_INFO("CONNECT");
        switch(advDirOrdConnState.phase)
        {
        case DEV_SCANNING:  // add device to the list of device that was detected
        {
            uint8_t pos;
            // disconnect input connection
            NRF_LOG_INFO("_SCANNING_");

            uint8_t cnt = 0;
            for(; cnt < m_whitelist_peer_cnt; cnt++)
            {
                if(m_whitelist_peers[cnt] == peerId)
                {
                    break;
                }

            }
            if(cnt < m_whitelist_peer_cnt)
            {
                NRF_LOG_INFO("remove dev %d", m_whitelist_peers[cnt] );
                for(; cnt < (m_whitelist_peer_cnt - 1); cnt++)
                {
                    m_whitelist_peers[cnt] = m_whitelist_peers[cnt+1];//
                }
                m_whitelist_peer_cnt--;
                m_whitelist_peers[cnt] = PM_PEER_ID_INVALID;
                NRF_LOG_INFO("PDL quan %d", m_whitelist_peer_cnt);


                err_code = pm_whitelist_set((m_whitelist_peer_cnt == 0 ) ? (NULL) : (m_whitelist_peers),
                                       (m_whitelist_peer_cnt == 0 ) ? (0)    : (m_whitelist_peer_cnt));
                APP_ERROR_CHECK(err_code);
                // Setup the device identies list.
                // Some SoftDevices do not support this feature.
                err_code = pm_device_identities_list_set((m_whitelist_peer_cnt == 0 ) ? (NULL) : (m_whitelist_peers),
                                                    (m_whitelist_peer_cnt == 0 ) ? (0)    : (m_whitelist_peer_cnt));
                APP_ERROR_CHECK(err_code);
            }
            //disconnect current device
            appDisconnect();

            NRF_LOG_INFO("--------Dev in list ? %d", peerId);
            if(orderGetPos(deviceOrder, peerId, &pos))
            {
                NRF_LOG_INFO("Dev find %d  %d ", peerId, pos);
                if(advDirOrdConnState.listDetectedDev[pos] == DETECTED_DEV_FREE)
                {
                    NRF_LOG_INFO("Dev save");
                    advDirOrdConnState.listDetectedDev[pos] = peerId;
                    advDirOrdConnState.quantityDetectedDev++;
                }
            }
            if(advDirOrdConnState.quantityDetectedDev >= orderGetQuantity(deviceOrder))
            {
                NRF_LOG_INFO("---------SCANED TIME = %d", getTime() - scaningTimeout);
                appAdvStop();
            }
        }
        break;
        case DEV_CONNECTION:// DO NOTHING (continue connection processing)
            /*DISCONNECT CURRENT DEVICE IF IT PEER ID DON'T EQUAL CURRENT DEVICE FROM WHITE LIST !!!! */
            NRF_LOG_INFO("_CONNECTION_");
            appAdvStop();
            break;
        }
        break;
    case ADV_PROC_STOP_ADV:
        NRF_LOG_INFO("STOP_ADV");
        switch(advDirOrdConnState.phase)
        {
        case DEV_SCANNING: // stop adv for previous phase and start connection to detected devices
        {
            uint8_t cnt_1 = 0;
            uint8_t cnt_2 = 0;
            NRF_LOG_INFO("_SCANNING_");
            NRF_LOG_INFO("Find %d", advDirOrdConnState.quantityDetectedDev);
            // if device wasn't detected  start adv again
            if(advDirOrdConnState.quantityDetectedDev == 0)
            {
                appAdvSetStart(ADV_RECONNECT_SCAN , 0);
                break;
            }
            // shift all devices on list UP
            for(cnt_1 = 0; cnt_1 < BLE_GAP_WHITELIST_ADDR_MAX_COUNT - 1; cnt_1++)
            {
                for(cnt_2 = BLE_GAP_WHITELIST_ADDR_MAX_COUNT-1; cnt_2 > 0; cnt_2--)
                {
                    if(advDirOrdConnState.listDetectedDev[cnt_2 - 1] == DETECTED_DEV_FREE &&
                            advDirOrdConnState.listDetectedDev[cnt_2 ]    != DETECTED_DEV_FREE)
                    {
                        advDirOrdConnState.listDetectedDev[cnt_2 - 1] = advDirOrdConnState.listDetectedDev[cnt_2 ];
                        advDirOrdConnState.listDetectedDev[cnt_2 ]    = DETECTED_DEV_FREE;
                    }
                }
            }

            advDirOrdConnState.connectCnt = 0;
            NRF_LOG_INFO("+++++++++");
            appAdvSetStart(ADV_RECONNECT_CONNECT , advDirOrdConnState.listDetectedDev[advDirOrdConnState.connectCnt]);
            NRF_LOG_INFO("---------");
            advDirOrdConnState.phase = DEV_CONNECTION;
        }
        break;
        case DEV_CONNECTION:   /*
            on this point I can go in two cases:
            - current device from WL was not able connect
            - LAST DEVICE THAT WAS IN CONNECT-> DISACONNECT STATE BEFOR STOP SCANNIN WAS DISCONNECTED

            */
            NRF_LOG_INFO("_CONNECTION_");
            if(advDirOrdConnState.connectCnt >= advDirOrdConnState.quantityDetectedDev)
            {

                advDirOrdConnState.loopCnt++;

                if(advDirOrdConnState.loopCnt >= DIRECT_CONN_QUANTITY)
                {
                    // start general advertising
                    break;
                }
                // start scanning again
                memset(advDirOrdConnState.listDetectedDev, DETECTED_DEV_FREE, sizeof(advDirOrdConnState.listDetectedDev));
                advDirOrdConnState.connectCnt = 0;
                advDirOrdConnState.phase      = DEV_SCANNING;
                appAdvSetStart(ADV_RECONNECT_SCAN, 0);
            }
            // shift to next detected device
            advDirOrdConnState.connectCnt++;
            appAdvSetStart(ADV_RECONNECT_CONNECT, advDirOrdConnState.listDetectedDev[advDirOrdConnState.connectCnt]);
            break;
        }
        break;
    default:
        break;
    }
}


void appAdvProcessing(nrfBLEAdvEvT inEv, pm_peer_id_t peerId)
{
    switch(appState.currentAdvType)
    {
    case ADV_ADD_NEW:
        NRF_LOG_INFO("ADV_ADD_NEW");
        appAdvAddNew(inEv, peerId);
        break;
    case ADV_RECONNECT_SCAN:
        NRF_LOG_INFO("ADV_SCAN");
        appAdvDirectProc(inEv, peerId);
        break;
    case ADV_RECONNECT_CONNECT:
        NRF_LOG_INFO("ADV_CONNECT");
        appAdvDirectProc(inEv, peerId);
        break;
    default:
        break;
    }
}


void appAdvScanStopCB(void)
{
    NRF_LOG_INFO("Stop Scan");
    appAdvStop();
    appAdvProcessing(ADV_PROC_STOP_ADV, 0);
}


/**@brief Clear bond information from persistent storage.
 */
static void delete_bonds(void)
{
    ret_code_t err_code;

    NRF_LOG_INFO("Erase bonds!");

    err_code = pm_peers_delete();
    NRF_LOG_INFO("DELETE ERR %d", err_code);
    APP_ERROR_CHECK(err_code);
}


void appProcessing(void)
{
    // start/stop adv processing
    if(appState.isAppAdv != appState.isRealAdv)
    {
        if(appState.isAppAdv)
        {
            appAdvStart();
        }
        else
        {
            appAdvStop();
        }
    }

    //delete bonds
    if(appState.isDeleteBonds &&  appState.connectState == CONNECTION_DISCONNECT)
    {
        appState.isDeleteBonds = false;
        NRF_LOG_INFO("Clear b_start");
        delete_bonds();
        orderClean(deviceOrder);
        orderWriteFlash(deviceOrder, GET_PAGE_ADDRESS(ORDER_FLASHE_PAGE));
    }
}


void flashMemWriteBytes(uint32_t flashAddress, uint8_t buffer[], uint32_t bufferSize)
{
    fds_record_t        record;
    fds_record_desc_t   record_desc;
    // Set up record.
    record.file_id           = FILE_ORDER;
    record.key               = RECORD_KEY_ORDER;
    record.data.p_data       = buffer;
    record.data.length_words = (bufferSize+ 3) / 4;
    ret_code_t ret;
    fds_find_token_t    ftok;

    /* It is required to zero the token before first use. */
    memset(&ftok, 0x00, sizeof(fds_find_token_t));

    // delete all previous records with the same KEY and ID
    while (fds_record_find(FILE_ORDER, RECORD_KEY_ORDER, &record_desc, &ftok) == FDS_SUCCESS)
    {
        ret = fds_record_delete(&record_desc);
        if (ret != FDS_SUCCESS)
        {
        /* Error. */
        }
    }
    ret = fds_record_write(&record_desc, &record);
    if (ret != FDS_SUCCESS)
    {
        /* Handle error. */
    }
}


void flashMemReadBytes(uint32_t flashAddress, uint8_t buffer[], uint32_t bufferSize)
{
    fds_flash_record_t  flash_record;
    fds_record_desc_t   record_desc;
    fds_find_token_t    ftok;

    /* It is required to zero the token before first use. */
    memset(&ftok, 0x00, sizeof(fds_find_token_t));
    /* Loop until all records with the given key and file ID have been found. */
    // while (fds_record_find(FILE_ORDER, RECORD_KEY_ORDER, &record_desc, &ftok) == FDS_SUCCESS)
    while (fds_record_find(FILE_ORDER, RECORD_KEY_ORDER, &record_desc, &ftok) == FDS_SUCCESS)
    {
        NRF_LOG_INFO("Read start");
        if (fds_record_open(&record_desc, &flash_record) != FDS_SUCCESS)
        {
            /* Handle error. */
        }

        memcpy(buffer, (uint8_t*)flash_record.p_data, bufferSize);

        /* Access the record through the flash_record structure. */
        /* Close the record when done. */
        if (fds_record_close(&record_desc) != FDS_SUCCESS)
        {
            /* Handle error. */
        }
    }
    NRF_LOG_INFO("Read End");
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
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}


/**@brief Fetch the list of peer manager peer IDs.
 *
 * @param[inout] p_peers   The buffer where to store the list of peer IDs.
 * @param[inout] p_size    In: The size of the @p p_peers buffer.
 *                         Out: The number of peers copied in the buffer.
 */
static void peer_list_get(pm_peer_id_t * p_peers, uint32_t * p_size)
{
    pm_peer_id_t peer_id;
    uint32_t     peers_to_copy;

    peers_to_copy = (*p_size < BLE_GAP_WHITELIST_ADDR_MAX_COUNT) ?
                     *p_size : BLE_GAP_WHITELIST_ADDR_MAX_COUNT;

    peer_id = pm_next_peer_id_get(PM_PEER_ID_INVALID);
    *p_size = 0;

    while ((peer_id != PM_PEER_ID_INVALID) && (peers_to_copy--))
    {
        p_peers[(*p_size)++] = peer_id;
        peer_id = pm_next_peer_id_get(peer_id);
    }
}



/**@brief Function for starting advertising.
 */
 /*
static void advertising_start(bool erase_bonds)
{
    if (erase_bonds == true)
    {
        delete_bonds();
        // Advertising is started by PM_EVT_PEERS_DELETE_SUCCEEDED event.
    }
    else
    {
        ret_code_t ret;

        memset(m_whitelist_peers, PM_PEER_ID_INVALID, sizeof(m_whitelist_peers));
        m_whitelist_peer_cnt = (sizeof(m_whitelist_peers) / sizeof(pm_peer_id_t));

        peer_list_get(m_whitelist_peers, &m_whitelist_peer_cnt);

         NRF_LOG_INFO("Number Peer %d", m_whitelist_peer_cnt);

        ret = pm_whitelist_set(m_whitelist_peers, m_whitelist_peer_cnt);
        APP_ERROR_CHECK(ret);

        // Setup the device identies list.
        // Some SoftDevices do not support this feature.
        ret = pm_device_identities_list_set(m_whitelist_peers, m_whitelist_peer_cnt);
        if (ret != NRF_ERROR_NOT_SUPPORTED)
        {
            APP_ERROR_CHECK(ret);
        }

        NRF_LOG_INFO("------start adv6-------");
        ret = ble_advertising_start(&m_advertising, BLE_ADV_MODE_DIRECTED);
        APP_ERROR_CHECK(ret);
    }
}
*/



/**@brief Function for handling Service errors.
 *
 * @details A pointer to this function will be passed to each service which may need to inform the
 *          application about an error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void service_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for handling advertising errors.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void ble_advertising_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for performing a battery measurement, and update the Battery Level characteristic in the Battery Service.
 */
static void battery_level_update(void)
{
    ret_code_t err_code;
    uint8_t  battery_level;

    battery_level = (uint8_t)sensorsim_measure(&m_battery_sim_state, &m_battery_sim_cfg);

    err_code = ble_bas_battery_level_update(&m_bas, battery_level);
    if ((err_code != NRF_SUCCESS) &&
        (err_code != NRF_ERROR_BUSY) &&
        (err_code != NRF_ERROR_RESOURCES) &&
        (err_code != NRF_ERROR_FORBIDDEN) &&
        (err_code != NRF_ERROR_INVALID_STATE) &&
        (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
       )
    {
        APP_ERROR_HANDLER(err_code);
    }
}


/**@brief Function for handling the Battery measurement timer timeout.
 *
 * @details This function will be called each time the battery level measurement timer expires.
 *
 * @param[in]   p_context   Pointer used for passing some arbitrary information (context) from the
 *                          app_start_timer() call to the timeout handler.
 */
static void battery_level_meas_timeout_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);
    battery_level_update();
}


/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module.
 */
static void timers_init(void)
{
    ret_code_t err_code;

    err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);

    // Create battery timer.
    err_code = app_timer_create(&m_battery_timer_id,
                                APP_TIMER_MODE_REPEATED,
                                battery_level_meas_timeout_handler);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for the GAP initialization.
 *
 * @details This function sets up all the necessary GAP (Generic Access Profile) parameters of the
 *          device including the device name, appearance, and the preferred connection parameters.
 */
static void gap_params_init(void)
{
    ret_code_t              err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *)DEVICE_NAME,
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    err_code = sd_ble_gap_appearance_set(BLE_APPEARANCE_HID_MOUSE);
    APP_ERROR_CHECK(err_code);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the GATT module.
 */
static void gatt_init(void)
{
    ret_code_t err_code = nrf_ble_gatt_init(&m_gatt, NULL);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing Device Information Service.
 */
static void dis_init(void)
{
    ret_code_t       err_code;
    ble_dis_init_t   dis_init_obj;
    ble_dis_pnp_id_t pnp_id;

    pnp_id.vendor_id_source = PNP_ID_VENDOR_ID_SOURCE;
    pnp_id.vendor_id        = PNP_ID_VENDOR_ID;
    pnp_id.product_id       = PNP_ID_PRODUCT_ID;
    pnp_id.product_version  = PNP_ID_PRODUCT_VERSION;

    memset(&dis_init_obj, 0, sizeof(dis_init_obj));

    ble_srv_ascii_to_utf8(&dis_init_obj.manufact_name_str, MANUFACTURER_NAME);
    dis_init_obj.p_pnp_id = &pnp_id;

    BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&dis_init_obj.dis_attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&dis_init_obj.dis_attr_md.write_perm);

    err_code = ble_dis_init(&dis_init_obj);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing Battery Service.
 */
static void bas_init(void)
{
    ret_code_t     err_code;
    ble_bas_init_t bas_init_obj;

    memset(&bas_init_obj, 0, sizeof(bas_init_obj));

    bas_init_obj.evt_handler          = NULL;
    bas_init_obj.support_notification = true;
    bas_init_obj.p_report_ref         = NULL;
    bas_init_obj.initial_batt_level   = 100;

    BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&bas_init_obj.battery_level_char_attr_md.cccd_write_perm);
    BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&bas_init_obj.battery_level_char_attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&bas_init_obj.battery_level_char_attr_md.write_perm);

    BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&bas_init_obj.battery_level_report_read_perm);

    err_code = ble_bas_init(&m_bas, &bas_init_obj);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing HID Service.
 */
static void hids_init(void)
{
    ret_code_t                err_code;
    ble_hids_init_t           hids_init_obj;
    ble_hids_inp_rep_init_t   inp_rep_array[INPUT_REPORT_COUNT];
    ble_hids_inp_rep_init_t * p_input_report;
    uint8_t                   hid_info_flags;

    static uint8_t rep_map_data[] =
    {
        0x05, 0x01, // Usage Page (Generic Desktop)
        0x09, 0x02, // Usage (Mouse)

        0xA1, 0x01, // Collection (Application)

        // Report ID 1: Mouse buttons + scroll/pan
        0x85, 0x01,       // Report Id 1
        0x09, 0x01,       // Usage (Pointer)
        0xA1, 0x00,       // Collection (Physical)
        0x95, 0x05,       // Report Count (3)
        0x75, 0x01,       // Report Size (1)
        0x05, 0x09,       // Usage Page (Buttons)
        0x19, 0x01,       // Usage Minimum (01)
        0x29, 0x05,       // Usage Maximum (05)
        0x15, 0x00,       // Logical Minimum (0)
        0x25, 0x01,       // Logical Maximum (1)
        0x81, 0x02,       // Input (Data, Variable, Absolute)
        0x95, 0x01,       // Report Count (1)
        0x75, 0x03,       // Report Size (3)
        0x81, 0x01,       // Input (Constant) for padding
        0x75, 0x08,       // Report Size (8)
        0x95, 0x01,       // Report Count (1)
        0x05, 0x01,       // Usage Page (Generic Desktop)
        0x09, 0x38,       // Usage (Wheel)
        0x15, 0x81,       // Logical Minimum (-127)
        0x25, 0x7F,       // Logical Maximum (127)
        0x81, 0x06,       // Input (Data, Variable, Relative)
        0x05, 0x0C,       // Usage Page (Consumer)
        0x0A, 0x38, 0x02, // Usage (AC Pan)
        0x95, 0x01,       // Report Count (1)
        0x81, 0x06,       // Input (Data,Value,Relative,Bit Field)
        0xC0,             // End Collection (Physical)

        // Report ID 2: Mouse motion
        0x85, 0x02,       // Report Id 2
        0x09, 0x01,       // Usage (Pointer)
        0xA1, 0x00,       // Collection (Physical)
        0x75, 0x0C,       // Report Size (12)
        0x95, 0x02,       // Report Count (2)
        0x05, 0x01,       // Usage Page (Generic Desktop)
        0x09, 0x30,       // Usage (X)
        0x09, 0x31,       // Usage (Y)
        0x16, 0x01, 0xF8, // Logical maximum (2047)
        0x26, 0xFF, 0x07, // Logical minimum (-2047)
        0x81, 0x06,       // Input (Data, Variable, Relative)
        0xC0,             // End Collection (Physical)
        0xC0,             // End Collection (Application)

        // Report ID 3: Advanced buttons
        0x05, 0x0C,       // Usage Page (Consumer)
        0x09, 0x01,       // Usage (Consumer Control)
        0xA1, 0x01,       // Collection (Application)
        0x85, 0x03,       // Report Id (3)
        0x15, 0x00,       // Logical minimum (0)
        0x25, 0x01,       // Logical maximum (1)
        0x75, 0x01,       // Report Size (1)
        0x95, 0x01,       // Report Count (1)

        0x09, 0xCD,       // Usage (Play/Pause)
        0x81, 0x06,       // Input (Data,Value,Relative,Bit Field)
        0x0A, 0x83, 0x01, // Usage (AL Consumer Control Configuration)
        0x81, 0x06,       // Input (Data,Value,Relative,Bit Field)
        0x09, 0xB5,       // Usage (Scan Next Track)
        0x81, 0x06,       // Input (Data,Value,Relative,Bit Field)
        0x09, 0xB6,       // Usage (Scan Previous Track)
        0x81, 0x06,       // Input (Data,Value,Relative,Bit Field)

        0x09, 0xEA,       // Usage (Volume Down)
        0x81, 0x06,       // Input (Data,Value,Relative,Bit Field)
        0x09, 0xE9,       // Usage (Volume Up)
        0x81, 0x06,       // Input (Data,Value,Relative,Bit Field)
        0x0A, 0x25, 0x02, // Usage (AC Forward)
        0x81, 0x06,       // Input (Data,Value,Relative,Bit Field)
        0x0A, 0x24, 0x02, // Usage (AC Back)
        0x81, 0x06,       // Input (Data,Value,Relative,Bit Field)
        0xC0              // End Collection
    };

    memset(inp_rep_array, 0, sizeof(inp_rep_array));
    // Initialize HID Service.
    p_input_report                      = &inp_rep_array[INPUT_REP_BUTTONS_INDEX];
    p_input_report->max_len             = INPUT_REP_BUTTONS_LEN;
    p_input_report->rep_ref.report_id   = INPUT_REP_REF_BUTTONS_ID;
    p_input_report->rep_ref.report_type = BLE_HIDS_REP_TYPE_INPUT;

    BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&p_input_report->security_mode.cccd_write_perm);
    BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&p_input_report->security_mode.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&p_input_report->security_mode.write_perm);

    p_input_report                      = &inp_rep_array[INPUT_REP_MOVEMENT_INDEX];
    p_input_report->max_len             = INPUT_REP_MOVEMENT_LEN;
    p_input_report->rep_ref.report_id   = INPUT_REP_REF_MOVEMENT_ID;
    p_input_report->rep_ref.report_type = BLE_HIDS_REP_TYPE_INPUT;

    BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&p_input_report->security_mode.cccd_write_perm);
    BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&p_input_report->security_mode.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&p_input_report->security_mode.write_perm);

    p_input_report                      = &inp_rep_array[INPUT_REP_MPLAYER_INDEX];
    p_input_report->max_len             = INPUT_REP_MEDIA_PLAYER_LEN;
    p_input_report->rep_ref.report_id   = INPUT_REP_REF_MPLAYER_ID;
    p_input_report->rep_ref.report_type = BLE_HIDS_REP_TYPE_INPUT;

    BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&p_input_report->security_mode.cccd_write_perm);
    BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&p_input_report->security_mode.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&p_input_report->security_mode.write_perm);

    hid_info_flags = HID_INFO_FLAG_REMOTE_WAKE_MSK | HID_INFO_FLAG_NORMALLY_CONNECTABLE_MSK;

    memset(&hids_init_obj, 0, sizeof(hids_init_obj));

    hids_init_obj.evt_handler                    = on_hids_evt;
    hids_init_obj.error_handler                  = service_error_handler;
    hids_init_obj.is_kb                          = false;
    hids_init_obj.is_mouse                       = true;
    hids_init_obj.inp_rep_count                  = INPUT_REPORT_COUNT;
    hids_init_obj.p_inp_rep_array                = inp_rep_array;
    hids_init_obj.outp_rep_count                 = 0;
    hids_init_obj.p_outp_rep_array               = NULL;
    hids_init_obj.feature_rep_count              = 0;
    hids_init_obj.p_feature_rep_array            = NULL;
    hids_init_obj.rep_map.data_len               = sizeof(rep_map_data);
    hids_init_obj.rep_map.p_data                 = rep_map_data;
    hids_init_obj.hid_information.bcd_hid        = BASE_USB_HID_SPEC_VERSION;
    hids_init_obj.hid_information.b_country_code = 0;
    hids_init_obj.hid_information.flags          = hid_info_flags;
    hids_init_obj.included_services_count        = 0;
    hids_init_obj.p_included_services_array      = NULL;

    BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&hids_init_obj.rep_map.security_mode.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&hids_init_obj.rep_map.security_mode.write_perm);
    BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&hids_init_obj.hid_information.security_mode.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&hids_init_obj.hid_information.security_mode.write_perm);

    BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(
        &hids_init_obj.security_mode_boot_mouse_inp_rep.cccd_write_perm);
    BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(
        &hids_init_obj.security_mode_boot_mouse_inp_rep.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(
        &hids_init_obj.security_mode_boot_mouse_inp_rep.write_perm);

    BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&hids_init_obj.security_mode_protocol.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&hids_init_obj.security_mode_protocol.write_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&hids_init_obj.security_mode_ctrl_point.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&hids_init_obj.security_mode_ctrl_point.write_perm);

    err_code = ble_hids_init(&m_hids, &hids_init_obj);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing services that will be used by the application.
 */
static void services_init(void)
{
    dis_init();
    bas_init();
    hids_init();
}


/**@brief Function for initializing the battery sensor simulator.
 */
static void sensor_simulator_init(void)
{
    m_battery_sim_cfg.min          = MIN_BATTERY_LEVEL;
    m_battery_sim_cfg.max          = MAX_BATTERY_LEVEL;
    m_battery_sim_cfg.incr         = BATTERY_LEVEL_INCREMENT;
    m_battery_sim_cfg.start_at_max = true;

    sensorsim_init(&m_battery_sim_state, &m_battery_sim_cfg);
}


/**@brief Function for handling a Connection Parameters error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for initializing the Connection Parameters module.
 */
static void conn_params_init(void)
{
    ret_code_t             err_code;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAM_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = NULL;
    cp_init.error_handler                  = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for starting timers.
 */
static void timers_start(void)
{
    ret_code_t err_code;

    err_code = app_timer_start(m_battery_timer_id, BATTERY_LEVEL_MEAS_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for putting the chip into sleep mode.
 *
 * @note This function will not return.
 */
static void sleep_mode_enter(void)
{
    ret_code_t err_code;

    err_code = bsp_indication_set(BSP_INDICATE_IDLE);
    APP_ERROR_CHECK(err_code);

    // Prepare wakeup buttons.
    err_code = bsp_btn_ble_sleep_mode_prepare();
    APP_ERROR_CHECK(err_code);

    // Go to system-off mode (this function will not return; wakeup will cause a reset).
    err_code = sd_power_system_off();
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling HID events.
 *
 * @details This function will be called for all HID events which are passed to the application.
 *
 * @param[in]   p_hids  HID service structure.
 * @param[in]   p_evt   Event received from the HID service.
 */
static void on_hids_evt(ble_hids_t * p_hids, ble_hids_evt_t * p_evt)
{
    switch (p_evt->evt_type)
    {
        case BLE_HIDS_EVT_BOOT_MODE_ENTERED:
            m_in_boot_mode = true;
            break;

        case BLE_HIDS_EVT_REPORT_MODE_ENTERED:
            m_in_boot_mode = false;
            break;

        case BLE_HIDS_EVT_NOTIF_ENABLED:
            break;

        default:
            // No implementation needed.
            break;
    }
}



/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
    ret_code_t err_code;

    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    // Enable BLE stack.
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    // Register a handler for BLE events.
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
}


/**@brief Function for the Peer Manager initialization.
 */
static void peer_manager_init(void)
{
    ble_gap_sec_params_t sec_param;
    ret_code_t           err_code;

    err_code = pm_init();
    APP_ERROR_CHECK(err_code);

    memset(&sec_param, 0, sizeof(ble_gap_sec_params_t));

    // Security parameters to be used for all security procedures.
    sec_param.bond           = SEC_PARAM_BOND;
    sec_param.mitm           = SEC_PARAM_MITM;
    sec_param.lesc           = SEC_PARAM_LESC;
    sec_param.keypress       = SEC_PARAM_KEYPRESS;
    sec_param.io_caps        = SEC_PARAM_IO_CAPABILITIES;
    sec_param.oob            = SEC_PARAM_OOB;
    sec_param.min_key_size   = SEC_PARAM_MIN_KEY_SIZE;
    sec_param.max_key_size   = SEC_PARAM_MAX_KEY_SIZE;
    sec_param.kdist_own.enc  = 1;
    sec_param.kdist_own.id   = 1;
    sec_param.kdist_peer.enc = 1;
    sec_param.kdist_peer.id  = 1;

    err_code = pm_sec_params_set(&sec_param);
    APP_ERROR_CHECK(err_code);

    err_code = pm_register(pm_evt_handler);
    APP_ERROR_CHECK(err_code);
}





/**@brief Function for the Event Scheduler initialization.
 */
static void scheduler_init(void)
{
    APP_SCHED_INIT(SCHED_MAX_EVENT_DATA_SIZE, SCHED_QUEUE_SIZE);
}


/**@brief Function for sending a Mouse Movement.
 *
 * @param[in]   x_delta   Horizontal movement.
 * @param[in]   y_delta   Vertical movement.
 */
static void mouse_movement_send(int16_t x_delta, int16_t y_delta)
{
    ret_code_t err_code;

    if (m_in_boot_mode)
    {
        x_delta = MIN(x_delta, 0x00ff);
        y_delta = MIN(y_delta, 0x00ff);

        err_code = ble_hids_boot_mouse_inp_rep_send(&m_hids,
                                                    0x00,
                                                    (int8_t)x_delta,
                                                    (int8_t)y_delta,
                                                    0,
                                                    NULL);
    }
    else
    {
        uint8_t buffer[INPUT_REP_MOVEMENT_LEN];

        APP_ERROR_CHECK_BOOL(INPUT_REP_MOVEMENT_LEN == 3);

        x_delta = MIN(x_delta, 0x0fff);
        y_delta = MIN(y_delta, 0x0fff);

        buffer[0] = x_delta & 0x00ff;
        buffer[1] = ((y_delta & 0x000f) << 4) | ((x_delta & 0x0f00) >> 8);
        buffer[2] = (y_delta & 0x0ff0) >> 4;

        err_code = ble_hids_inp_rep_send(&m_hids,
                                         INPUT_REP_MOVEMENT_INDEX,
                                         INPUT_REP_MOVEMENT_LEN,
                                         buffer);
    }

    NRF_LOG_INFO("HID ERROR %d", err_code);

    if ((err_code != NRF_SUCCESS) &&
        (err_code != NRF_ERROR_INVALID_STATE) &&
        (err_code != NRF_ERROR_RESOURCES) &&
        (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
       )
    {
        APP_ERROR_HANDLER(err_code);
    }
}



bool genisAppAdv = false;

/**@brief Function for handling events from the BSP module.
 *
 * @param[in]   event   Event generated by button press.
 */
static void bsp_event_handler(bsp_event_t event)
{
    ret_code_t err_code;

    switch (event)
    {
        case BSP_EVENT_SLEEP:
            sleep_mode_enter();
            break;

        case BSP_EVENT_DISCONNECT:
            err_code = sd_ble_gap_disconnect(m_conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            if (err_code != NRF_ERROR_INVALID_STATE)
            {
                APP_ERROR_CHECK(err_code);
            }
            break;

        case BSP_EVENT_WHITELIST_OFF:
            if (m_conn_handle == BLE_CONN_HANDLE_INVALID)
            {
                err_code = ble_advertising_restart_without_whitelist(&m_advertising);
                if (err_code != NRF_ERROR_INVALID_STATE)
                {
                    APP_ERROR_CHECK(err_code);
                }
            }
            break;

        case BSP_EVENT_KEY_0:
            NRF_LOG_INFO("KEY_0");
            if (m_conn_handle != BLE_CONN_HANDLE_INVALID)
            {
                NRF_LOG_INFO("OK");
                mouse_movement_send(-MOVEMENT_SPEED, 0);
            }
            break;

        case BSP_EVENT_KEY_1:
            NRF_LOG_INFO("KEY_1");
            if (m_conn_handle != BLE_CONN_HANDLE_INVALID)
            {
                NRF_LOG_INFO("OK");
                mouse_movement_send(0, -MOVEMENT_SPEED);
            }
            break;

        case BSP_EVENT_KEY_2:
            NRF_LOG_INFO("Clear all bonds");
            appState.isDeleteBonds = true;

            break;

        case BSP_EVENT_KEY_3:
           NRF_LOG_INFO("ADD_NEW adv start");
           appAdvSetStart(ADV_ADD_NEW, 0);
           break;

        default:
            break;
    }
}


/**@brief Function for initializing buttons and leds.
 *
 * @param[out] p_erase_bonds  Will be true if the clear bonding button was pressed to wake the application up.
 */
static void buttons_leds_init(bool * p_erase_bonds)
{
    ret_code_t err_code;
    bsp_event_t startup_event;

    err_code = bsp_init(BSP_INIT_LED | BSP_INIT_BUTTONS, bsp_event_handler);

    APP_ERROR_CHECK(err_code);

    err_code = bsp_btn_ble_init(NULL, &startup_event);
    APP_ERROR_CHECK(err_code);

    *p_erase_bonds = (startup_event == BSP_EVENT_CLEAR_BONDING_DATA);
}


/**@brief Function for initializing the nrf log module.
 */
static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}


/**@brief Function for the Power manager.
 */
static void power_manage(void)
{
    ret_code_t err_code = sd_app_evt_wait();
    APP_ERROR_CHECK(err_code);
}


// Simple event handler to handle errors during initialization.
static void fds_evt_handler(fds_evt_t const * p_fds_evt)
{
    switch(p_fds_evt->id)
    {
        case  FDS_EVT_INIT:       NRF_LOG_INFO("FDS_EVT_INIT");        break;
        case  FDS_EVT_WRITE:      NRF_LOG_INFO("FDS_EVT_WRITE");       break;
        case  FDS_EVT_UPDATE:     NRF_LOG_INFO("FDS_EVT_UPDATE");      break;
        case  FDS_EVT_DEL_RECORD: NRF_LOG_INFO("FDS_EVT_DEL_RECORD");  break;
        case  FDS_EVT_DEL_FILE:   NRF_LOG_INFO("FDS_EVT_DEL_FILE");    break;
        case  FDS_EVT_GC:         NRF_LOG_INFO("FDS_EVT_GC");          break;
    }

    switch (p_fds_evt->id)
    {
        case FDS_EVT_INIT:
            if (p_fds_evt->result != FDS_SUCCESS)
            {
                // Initialization failed.
            }
            break;
        default:
            break;
    }
}


/**@brief Function for application main entry.
 */
int main(void)
{
    bool erase_bonds;

    // Initialize.
    log_init();
    NRF_LOG_INFO("Gerasimchuk started.");

    initUserTimer();
    stopScanAdvTimerCallback = timerGetCallback(appAdvScanStopCB);
    deviceOrder              = orderMalloc();

    ret_code_t ret           = fds_register(fds_evt_handler);
    if (ret != FDS_SUCCESS)
    {
    // Registering of the FDS event handler has failed.
    }
    ret = fds_init();
    if (ret != FDS_SUCCESS)
    {
    // Handle error.
    }
    //orderWriteFlash(deviceOrder, GET_PAGE_ADDRESS(ORDER_FLASHE_PAGE));
    orderReadFlash(deviceOrder, GET_PAGE_ADDRESS(ORDER_FLASHE_PAGE));
    NRF_LOG_INFO("Order items = %d",  orderGetQuantity(deviceOrder));
    for(uint8_t cnt = 0; cnt < ORDER_ITEM_QUANTITY; cnt++ )
    {
        NRF_LOG_INFO("item = %d \n", orderGetItem(deviceOrder, cnt));
    }

    timers_init();
    buttons_leds_init(&erase_bonds);
    ble_stack_init();
    scheduler_init();
    gap_params_init();
    gatt_init();
    services_init();
    sensor_simulator_init();
    conn_params_init();
    peer_manager_init();
    // Start execution.

    timers_start();
    advertising_init(APP_ADV_GLOBAL_INTERVAL, APP_ADV_GLOBAL_TIMEOUT, true);
    appAdvSetStart(ADV_RECONNECT_SCAN, 0);

    // Enter main loop.
    for (;;)
    {
        app_sched_execute();
        /******app processing*****************/
        appProcessing();
        userProcessingTimerCallbackFun();
        /*************************************/

        if (NRF_LOG_PROCESS() == false)
        {
            power_manage();
        }
    }
}



/**
 * @}
 */


/**@brief Function for handling BLE events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 * @param[in]   p_context   Unused.
 */
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
    ret_code_t err_code;

    switch(p_ble_evt->header.evt_id)
    {
         case BLE_GAP_EVT_CONNECTED:                  NRF_LOG_INFO("CONNECTED");                  break;
         case BLE_GAP_EVT_DISCONNECTED:               NRF_LOG_INFO("DISCONNECTED");               break;
         case BLE_GAP_EVT_CONN_PARAM_UPDATE:          NRF_LOG_INFO("CONN_PARAM_UPDATE");          break;
         case BLE_GAP_EVT_SEC_PARAMS_REQUEST:         NRF_LOG_INFO("SEC_PARAMS_REQUEST");         break;
         case BLE_GAP_EVT_SEC_INFO_REQUEST:           NRF_LOG_INFO("SEC_INFO_REQUEST");           break;
         case BLE_GAP_EVT_PASSKEY_DISPLAY:            NRF_LOG_INFO("PASSKEY_DISPLAY");            break;
         case BLE_GAP_EVT_KEY_PRESSED:                NRF_LOG_INFO("KEY_PRESSED");                break;
         case BLE_GAP_EVT_AUTH_KEY_REQUEST:           NRF_LOG_INFO("AUTH_KEY_REQUEST");           break;
         case BLE_GAP_EVT_LESC_DHKEY_REQUEST:         NRF_LOG_INFO("LESC_DHKEY_REQUEST");         break;
         case BLE_GAP_EVT_AUTH_STATUS:                NRF_LOG_INFO("AUTH_STATUS");                break;
         case BLE_GAP_EVT_CONN_SEC_UPDATE:            NRF_LOG_INFO("CONN_SEC_UPDATE");            break;
         case BLE_GAP_EVT_TIMEOUT:                    NRF_LOG_INFO("TIMEOUT");                    break;
         case BLE_GAP_EVT_RSSI_CHANGED:               NRF_LOG_INFO("RSSI_CHANGED");               break;
         case BLE_GAP_EVT_ADV_REPORT:                 NRF_LOG_INFO("ADV_REPORT");                 break;
         case BLE_GAP_EVT_SEC_REQUEST:                NRF_LOG_INFO("SEC_REQUEST");                break;
         case BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST:  NRF_LOG_INFO("CONN_PARAM_UPDATE_REQUEST");  break;
         case BLE_GAP_EVT_SCAN_REQ_REPORT:            NRF_LOG_INFO("SCAN_REQ_REPORT");            break;
         case BLE_GAP_EVT_PHY_UPDATE:                 NRF_LOG_INFO("PHY_UPDATE");                 break;
         case BLE_GAP_EVT_DATA_LENGTH_UPDATE_REQUEST: NRF_LOG_INFO("DATA_LENGTH_UPDATE_REQUEST"); break;
         case BLE_GAP_EVT_DATA_LENGTH_UPDATE:         NRF_LOG_INFO("DATA_LENGTH_UPDATE");         break;
    }

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:

            err_code = bsp_indication_set(BSP_INDICATE_CONNECTED);
            APP_ERROR_CHECK(err_code);
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            if(appAdvGetPrevConn())
            {
                break;
            }
            /******app processing*****************/
            appAdvSetRealState(false);
            appConnectSetState(CONNECTION_CONNECT, m_conn_handle);
            appAdvProcessing(ADV_PROC_CONNECT,0);
            /*************************************/
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            //NRF_LOG_INFO("Disconnected");

            /******app processing*****************/
            appAdvSetPrevConn(false);
            appConnectSetState(CONNECTION_DISCONNECT, BLE_CONN_HANDLE_INVALID);
            /*************************************/

            m_conn_handle = BLE_CONN_HANDLE_INVALID;
            break;

#ifndef S140
        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
            NRF_LOG_DEBUG("PHY update request.");
            ble_gap_phys_t const phys =
            {
                .rx_phys = BLE_GAP_PHY_AUTO,
                .tx_phys = BLE_GAP_PHY_AUTO,
            };
            err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
            APP_ERROR_CHECK(err_code);
        } break;
#endif

        case BLE_GATTC_EVT_TIMEOUT:
            // Disconnect on GATT Client timeout event.
            NRF_LOG_DEBUG("GATT Client Timeout.");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_TIMEOUT:
            // Disconnect on GATT Server timeout event.
            NRF_LOG_DEBUG("GATT Server Timeout.");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_EVT_USER_MEM_REQUEST:
            err_code = sd_ble_user_mem_reply(m_conn_handle, NULL);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST:
        {
            ble_gatts_evt_rw_authorize_request_t  req;
            ble_gatts_rw_authorize_reply_params_t auth_reply;

            req = p_ble_evt->evt.gatts_evt.params.authorize_request;

            if (req.type != BLE_GATTS_AUTHORIZE_TYPE_INVALID)
            {
                if ((req.request.write.op == BLE_GATTS_OP_PREP_WRITE_REQ)     ||
                    (req.request.write.op == BLE_GATTS_OP_EXEC_WRITE_REQ_NOW) ||
                    (req.request.write.op == BLE_GATTS_OP_EXEC_WRITE_REQ_CANCEL))
                {
                    if (req.type == BLE_GATTS_AUTHORIZE_TYPE_WRITE)
                    {
                        auth_reply.type = BLE_GATTS_AUTHORIZE_TYPE_WRITE;
                    }
                    else
                    {
                        auth_reply.type = BLE_GATTS_AUTHORIZE_TYPE_READ;
                    }
                    auth_reply.params.write.gatt_status = APP_FEATURE_NOT_SUPPORTED;
                    err_code = sd_ble_gatts_rw_authorize_reply(p_ble_evt->evt.gatts_evt.conn_handle,
                                                               &auth_reply);
                    APP_ERROR_CHECK(err_code);
                }
            }
        } break; // BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST

        default:
            // No implementation needed.
            break;
    }
}




/**@brief Function for handling Peer Manager events.
 *
 * @param[in] p_evt  Peer Manager event.
 */
static void pm_evt_handler(pm_evt_t const * p_evt)
{
    ret_code_t err_code;

    switch(p_evt->evt_id)
    {
        case PM_EVT_BONDED_PEER_CONNECTED:         NRF_LOG_INFO("PM_EVT_BONDED_PEER_CONNECTED");         break;
        case PM_EVT_CONN_SEC_START:                NRF_LOG_INFO("PM_EVT_CONN_SEC_START");                break;
        case PM_EVT_CONN_SEC_SUCCEEDED:            NRF_LOG_INFO("PM_EVT_CONN_SEC_SUCCEEDED");            break;
        case PM_EVT_CONN_SEC_FAILED:               NRF_LOG_INFO("PM_EVT_CONN_SEC_FAILED");               break;
        case PM_EVT_CONN_SEC_CONFIG_REQ:           NRF_LOG_INFO("PM_EVT_CONN_SEC_CONFIG_REQ");           break;
        case PM_EVT_CONN_SEC_PARAMS_REQ:           NRF_LOG_INFO("PM_EVT_CONN_SEC_PARAMS_REQ");           break;
        case PM_EVT_STORAGE_FULL:                  NRF_LOG_INFO("PM_EVT_STORAGE_FULL");                  break;
        case PM_EVT_ERROR_UNEXPECTED:              NRF_LOG_INFO("PM_EVT_ERROR_UNEXPECTED");              break;
        case PM_EVT_PEER_DATA_UPDATE_SUCCEEDED:    NRF_LOG_INFO("PM_EVT_PEER_DATA_UPDATE_SUCCEEDED");    break;
        case PM_EVT_PEER_DATA_UPDATE_FAILED:       NRF_LOG_INFO("PM_EVT_PEER_DATA_UPDATE_FAILED");       break;
        case PM_EVT_PEER_DELETE_SUCCEEDED:         NRF_LOG_INFO("PM_EVT_PEER_DELETE_SUCCEEDED");         break;
        case PM_EVT_PEER_DELETE_FAILED:            NRF_LOG_INFO("PM_EVT_PEER_DELETE_FAILED");            break;
        case PM_EVT_PEERS_DELETE_SUCCEEDED:        NRF_LOG_INFO("PM_EVT_PEERS_DELETE_SUCCEEDED");        break;
        case PM_EVT_PEERS_DELETE_FAILED:           NRF_LOG_INFO("PM_EVT_PEERS_DELETE_FAILED");           break;
        case PM_EVT_LOCAL_DB_CACHE_APPLIED:        NRF_LOG_INFO("PM_EVT_LOCAL_DB_CACHE_APPLIED");        break;
        case PM_EVT_LOCAL_DB_CACHE_APPLY_FAILED:   NRF_LOG_INFO("PM_EVT_LOCAL_DB_CACHE_APPLY_FAILED");   break;
        case PM_EVT_SERVICE_CHANGED_IND_SENT:      NRF_LOG_INFO("PM_EVT_SERVICE_CHANGED_IND_SENT");      break;
        case PM_EVT_SERVICE_CHANGED_IND_CONFIRMED: NRF_LOG_INFO("PM_EVT_SERVICE_CHANGED_IND_CONFIRMED"); break;
        case PM_EVT_SLAVE_SECURITY_REQ:            NRF_LOG_INFO("PM_EVT_SLAVE_SECURITY_REQ");            break;
        case PM_EVT_FLASH_GARBAGE_COLLECTED:       NRF_LOG_INFO("PM_EVT_FLASH_GARBAGE_COLLECTED");       break;
    }

    switch (p_evt->evt_id)
    {

        case PM_EVT_BONDED_PEER_CONNECTED:
        {
            NRF_LOG_INFO("Prev con: %d", p_evt->peer_id);
            /******app processing*****************/
            appAdvSetPrevConn(true);
            appAdvSetRealState(false);
            appConnectSetState(CONNECTION_CONNECT, p_evt->conn_handle);
            appAdvProcessing(ADV_PROC_START_CONNECT, p_evt->peer_id);
            /****** *******************************/

        } break;

        case PM_EVT_CONN_SEC_SUCCEEDED:
        {
            NRF_LOG_INFO("Connection secured: role: %d, conn_handle: 0x%x, procedure: %d.",
                         ble_conn_state_role(p_evt->conn_handle),
                         p_evt->conn_handle,
                         p_evt->params.conn_sec_succeeded.procedure);

            /* -AddOrder- Added new device to the order*/
            orderSetFirst(deviceOrder, p_evt->peer_id);
            orderWriteFlash(deviceOrder, GET_PAGE_ADDRESS(ORDER_FLASHE_PAGE));

            m_peer_id = p_evt->peer_id;

        } break;

        case PM_EVT_CONN_SEC_FAILED:
        {
            appDisconnect();
            /* Often, when securing fails, it shouldn't be restarted, for security reasons.
             * Other times, it can be restarted directly.
             * Sometimes it can be restarted, but only after changing some Security Parameters.
             * Sometimes, it cannot be restarted until the link is disconnected and reconnected.
             * Sometimes it is impossible, to secure the link, or the peer device does not support it.
             * How to handle this error is highly application dependent. */
        } break;

        case PM_EVT_CONN_SEC_CONFIG_REQ:
        {
            // Reject pairing request from an already bonded peer.
            pm_conn_sec_config_t conn_sec_config = {.allow_repairing = false};
            pm_conn_sec_config_reply(p_evt->conn_handle, &conn_sec_config);
        } break;

        case PM_EVT_STORAGE_FULL:
        {
            // Run garbage collection on the flash.
            err_code = fds_gc();
            if (err_code == FDS_ERR_BUSY || err_code == FDS_ERR_NO_SPACE_IN_QUEUES)
            {
                // Retry.
            }
            else
            {
                APP_ERROR_CHECK(err_code);
            }
        } break;

        case PM_EVT_PEERS_DELETE_SUCCEEDED:
        {
            //advertising_start(false);
        } break;

        case PM_EVT_LOCAL_DB_CACHE_APPLY_FAILED:
        {
            // The local database has likely changed, send service changed indications.
            pm_local_database_has_changed();
        } break;

        case PM_EVT_PEER_DATA_UPDATE_SUCCEEDED:
        {
            if (     p_evt->params.peer_data_update_succeeded.flash_changed
                 && (p_evt->params.peer_data_update_succeeded.data_id == PM_PEER_DATA_ID_BONDING))
            {

                /* -AddOrder- Added new device to the order*/
                orderSetFirst(deviceOrder, p_evt->peer_id);
                orderWriteFlash(deviceOrder, GET_PAGE_ADDRESS(ORDER_FLASHE_PAGE));


                NRF_LOG_INFO("New Bond, add the peer to the whitelist if possible");
                NRF_LOG_INFO("\tm_whitelist_peer_cnt %d, MAX_PEERS_WLIST %d",
                               m_whitelist_peer_cnt + 1,
                               BLE_GAP_WHITELIST_ADDR_MAX_COUNT);
                // Note: You should check on what kind of white list policy your application should use.

                if (m_whitelist_peer_cnt < BLE_GAP_WHITELIST_ADDR_MAX_COUNT)
                {
                    // Bonded to a new peer, add it to the whitelist.
                    m_whitelist_peers[m_whitelist_peer_cnt++] = m_peer_id;

                    // The whitelist has been modified, update it in the Peer Manager.
                    err_code = pm_whitelist_set(m_whitelist_peers, m_whitelist_peer_cnt);
                    APP_ERROR_CHECK(err_code);

                    err_code = pm_device_identities_list_set(m_whitelist_peers, m_whitelist_peer_cnt);
                    if (err_code != NRF_ERROR_NOT_SUPPORTED)
                    {
                        APP_ERROR_CHECK(err_code);
                    }
                }
            }
        } break;

        case PM_EVT_PEER_DATA_UPDATE_FAILED:
        {
            // Assert.
            APP_ERROR_CHECK(p_evt->params.peer_data_update_failed.error);
        } break;

        case PM_EVT_PEER_DELETE_FAILED:
        {
            // Assert.
            APP_ERROR_CHECK(p_evt->params.peer_delete_failed.error);
        } break;

        case PM_EVT_PEERS_DELETE_FAILED:
        {
            // Assert.
            APP_ERROR_CHECK(p_evt->params.peers_delete_failed_evt.error);
        } break;

        case PM_EVT_ERROR_UNEXPECTED:
        {
            // Assert.
            APP_ERROR_CHECK(p_evt->params.error_unexpected.error);
        } break;

        case PM_EVT_CONN_SEC_START:
        case PM_EVT_PEER_DELETE_SUCCEEDED:
        case PM_EVT_LOCAL_DB_CACHE_APPLIED:
        case PM_EVT_SERVICE_CHANGED_IND_SENT:
        case PM_EVT_SERVICE_CHANGED_IND_CONFIRMED:
        default:
            break;
    }
}



/**@brief Function for handling advertising events.
 *
 * @details This function will be called for advertising events which are passed to the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    ret_code_t err_code;

    NRF_LOG_INFO("ADV_EV %d", ble_adv_evt);

    if( (appAdvGetState() == false) && (ble_adv_evt != BLE_ADV_EVT_IDLE))
    {
        NRF_LOG_INFO("Stop Adv");
        appAdvStop();
        return;
    }

    switch (ble_adv_evt)
    {
        case BLE_ADV_EVT_DIRECTED:
            NRF_LOG_INFO("Directed advertising.");
            err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING_DIRECTED);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_ADV_EVT_FAST:
            //NRF_LOG_INFO("Fast advertising.");
            err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_ADV_EVT_SLOW:
            NRF_LOG_INFO("Slow advertising.");
            err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING_SLOW);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_ADV_EVT_FAST_WHITELIST:
           // NRF_LOG_INFO("Fast advertising with whitelist.");
            err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING_WHITELIST);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_ADV_EVT_SLOW_WHITELIST:
            NRF_LOG_INFO("Slow advertising with whitelist.");
            err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING_WHITELIST);
            APP_ERROR_CHECK(err_code);
            err_code = ble_advertising_restart_without_whitelist(&m_advertising);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_ADV_EVT_IDLE:
            err_code = bsp_indication_set(BSP_INDICATE_IDLE);
            APP_ERROR_CHECK(err_code);

            /******processing adv event************/
            appAdvSetRealState(false);
            /****** *******************************/
            //sleep_mode_enter();
            break;

        case BLE_ADV_EVT_WHITELIST_REQUEST:
        {
            ble_gap_addr_t whitelist_addrs[BLE_GAP_WHITELIST_ADDR_MAX_COUNT];
            ble_gap_irk_t  whitelist_irks[BLE_GAP_WHITELIST_ADDR_MAX_COUNT];
            uint32_t       addr_cnt = BLE_GAP_WHITELIST_ADDR_MAX_COUNT;
            uint32_t       irk_cnt  = BLE_GAP_WHITELIST_ADDR_MAX_COUNT;

            err_code = pm_whitelist_get(whitelist_addrs, &addr_cnt,
                                        whitelist_irks,  &irk_cnt);
            APP_ERROR_CHECK(err_code);
            NRF_LOG_INFO("pm_whitelist_get returns %d addr in whitelist and %d irk whitelist",
                           addr_cnt,
                           irk_cnt);

            // Apply the whitelist.
            err_code = ble_advertising_whitelist_reply(&m_advertising,
                                                       whitelist_addrs,
                                                       addr_cnt,
                                                       whitelist_irks,
                                                       irk_cnt);
            APP_ERROR_CHECK(err_code);
        }
        break;

        case BLE_ADV_EVT_PEER_ADDR_REQUEST:
        {
            pm_peer_data_bonding_t peer_bonding_data;
            NRF_LOG_INFO("Dir_next");

            // Only Give peer address if we have a handle to the bonded peer.
            if (m_peer_id != PM_PEER_ID_INVALID)
            {

                err_code = pm_peer_data_bonding_load(m_peer_id, &peer_bonding_data);
                if (err_code != NRF_ERROR_NOT_FOUND)
                {
                    APP_ERROR_CHECK(err_code);

                    ble_gap_addr_t * p_peer_addr = &(peer_bonding_data.peer_ble_id.id_addr_info);
                    err_code = ble_advertising_peer_addr_reply(&m_advertising, p_peer_addr);
                    APP_ERROR_CHECK(err_code);
                }

            }
            break;
        }

        default:
            break;
    }
}


