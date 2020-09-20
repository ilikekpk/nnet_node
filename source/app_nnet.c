#include "app_nnet.h"
//#include "sdk_config.h"
#include "ble_advertising.h"
#include "nrf_sdh_ble.h"
#include "app_error.h"
//#include "ble_gap.h"
#include "app_util_platform.h" // for breakpoint

#include "cifra_eax_aes.h"
#include "crc16.h"

#define APP_BLE_OBSERVER_PRIO 3
#define SCAN_REPORT_BUFFER_SIZE 31

static uint8_t scan_report_buffer[SCAN_REPORT_BUFFER_SIZE] = {0};
static uint32_t nnet_counter = 0;
static ble_data_t ble_data =
{
    .p_data = scan_report_buffer,
    .len = SCAN_REPORT_BUFFER_SIZE
};
static ble_gap_scan_params_t params = 
{
    .scan_phys = BLE_GAP_PHY_1MBPS,
    .interval = 64,
    .window = 64,
    .timeout = 0
};
static struct
{
    uint8_t buffer[SCAN_REPORT_BUFFER_SIZE];
    uint8_t buffer_size;
    bool data_ready_flag;
} app_nnet_rx = {0};
static cf_aes_context cf_aes_config = {
    .rounds = AES128_ROUNDS
};

static uint32_t adv_report_parse(uint8_t type, ble_data_t const * p_advdata, ble_data_t * p_typedata)
{
    uint32_t index = 0;
    uint8_t * p_data;

    p_data = p_advdata->p_data;

    while (index < p_advdata->len)
    {
        uint8_t field_length = p_data[index];
        uint8_t field_type = p_data[index + 1];

        if (field_type == type)
        {
            p_typedata->p_data = &p_data[index + 2];
            p_typedata->len = field_length - 1;
            return NRF_SUCCESS;
        }
        index += field_length + 1;
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
                    manuf_data.p_data[2] == node_addr &&
                    manuf_data.len == 16 + 3)                                          // size of ecb block + manuf_id and node_addr
                {
                    memset(app_nnet_rx.buffer, 0, SCAN_REPORT_BUFFER_SIZE);
                    memcpy(app_nnet_rx.buffer, manuf_data.p_data + 3, manuf_data.len - 3); // +3 skip manuf_id and node_addr
                    app_nnet_rx.buffer_size = manuf_data.len - 3;                      // -3 size without manuf_id and node addr
                    app_nnet_rx.data_ready_flag = true;
                }
            }
            ret_code_t err_code = sd_ble_gap_scan_start(NULL, &ble_data);
            APP_ERROR_CHECK(err_code);
            break;

        default:
            break;
    }
}

bool app_nnet_get_cmd(nnet_config_t * config, uint8_t* p_buffer) //size of buffer == 10 (APP_NNET_SIZE)
{
    if (!app_nnet_rx.data_ready_flag) return false;
    app_nnet_rx.data_ready_flag = false;
    uint8_t packet[16] = {0};

    cf_aes_decrypt(&cf_aes_config, app_nnet_rx.buffer, packet); 

    uint16_t packet_crc16 = packet[14] << 8 | packet[15];
    packet[14] = 0;
    packet[15] = 0;
    if (crc16_compute(packet, 16, NULL) != packet_crc16) return false;
    
    uint32_t packet_counter = 0;
    memcpy(&packet_counter, &packet[10], 4);
   // if (nnet_counter > packet_counter) return false;
    nnet_counter++;

    memcpy(p_buffer, packet, APP_NNET_PACKET_SIZE);  //packet without counter and crc

    return true;
}

void app_nnet_init(nnet_config_t * config)
{
    static uint8_t context = 0;
    context = config->nnet_addr;
    
    nnet_counter = config->start_counter;

    cf_aes_init(&cf_aes_config, config->aes_key, 16);

    NRF_SDH_BLE_OBSERVER(m_ble_observer1, APP_BLE_OBSERVER_PRIO, ble_evt_handler, (void*) &context);

    ret_code_t err_code = sd_ble_gap_scan_start(&params, &ble_data);
    APP_ERROR_CHECK(err_code);
}