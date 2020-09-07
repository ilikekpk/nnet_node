#include "app_nnet.h"
//#include "sdk_config.h"
#include "ble_advertising.h"
#include "nrf_sdh_ble.h"
#include "app_error.h"
//#include "ble_gap.h"
#include "app_util_platform.h" // for breakpoint

#define NRF_BLE_SCAN_BUFFER 31 // from sdk config
#define APP_BLE_OBSERVER_PRIO 3 

static uint8_t scan_report_buffer[NRF_BLE_SCAN_BUFFER] = {0};
static ble_data_t ble_data =
{
    .p_data = scan_report_buffer,
    .len = NRF_BLE_SCAN_BUFFER
};
static ble_gap_scan_params_t params = 
{
    .scan_phys = BLE_GAP_PHY_1MBPS,
    .interval = 64,
    .window = 64,
    .timeout = 0
};

static uint32_t adv_report_parse(uint8_t type, ble_data_t const * p_advdata, ble_data_t * p_typedata)
{
    uint32_t index = 0;
    uint8_t * p_data;

    p_data = p_advdata->p_data;

    while (index < p_advdata->len)
    {
        uint8_t field_length = p_data[index];
        uint8_t field_type = p_data[index+1];

        if (field_type == type)
        {
            p_typedata->p_data = &p_data[index+2];
            p_typedata->len = field_length-1;
            return NRF_SUCCESS;
        }
        index += field_length+1;
    }
    return NRF_ERROR_NOT_FOUND;
}

uint8_t db = 0;

static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * context)
{
    ret_code_t err_code;
    ble_data_t manuf_data;
    
    uint8_t node_addr = *((uint8_t*) context);

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_ADV_REPORT:
            db++;
            err_code = adv_report_parse(0xFF, &p_ble_evt->evt.gap_evt.params.adv_report.data, &manuf_data);
            if (err_code == NRF_SUCCESS)
            {
                if (manuf_data.p_data[0] == 0x59 && 
                    manuf_data.p_data[1] == 0x00 && 
                    manuf_data.p_data[2] == node_addr)
                {
                    NRF_BREAKPOINT_COND;
                }
            }
            ret_code_t err_code = sd_ble_gap_scan_start(NULL, &ble_data);
            APP_ERROR_CHECK(err_code);
            break;

        default:
            break;
    }
}

void app_nnet_init(uint8_t node_addr)
{
    static uint8_t context = 0;
    context = node_addr;

    NRF_SDH_BLE_OBSERVER(m_ble_observer1, APP_BLE_OBSERVER_PRIO, ble_evt_handler, (void*) &context);

    ret_code_t err_code = sd_ble_gap_scan_start(&params, &ble_data);
    APP_ERROR_CHECK(err_code);
}