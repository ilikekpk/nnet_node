#include <stdint.h>
#include <string.h>

#include "nordic_common.h"
#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
#include "nrf_sdh_ble.h"
#include "app_timer.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"

#include "ble_commands.h"
#include "app_nnet.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#define SWITCH_PIN 13
#define FIRMWARE_VERSION 1

static uint8_t buffer[APP_NNET_PACKET_SIZE];

nnet_config_t nnet_config = {
        .nnet_addr = 0xff,
        .aes_key = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15},
        .start_counter = 30
};

static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}


static void timers_init(void)
{
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);
}

static void ble_stack_init(void)
{
    ret_code_t err_code;

    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(1, &ram_start);
    APP_ERROR_CHECK(err_code);

    // Enable BLE stack.
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    // Register a handler for BLE events.
    
}

void ble_commands_evt_handler(char * command_line)
{
    char command[10] = {0};

    sprintf(command, "%s", "CONFIG,");
    if (strstr(command_line, command))
    {
        if (strstr(command_line, "START_DFU"))
        {
            ret_code_t err_code = sd_power_gpregret_set(0, 0xB1);
            APP_ERROR_CHECK(err_code);

            sd_nvic_SystemReset();  
        }
    }
}

int main(void)
{
    ble_stack_init();
    log_init();
    timers_init();
    ble_commands_init(ble_commands_evt_handler);

    app_nnet_init(&nnet_config);

    NRF_LOG_INFO("Debug logging for UART over RTT started.");

    nrf_gpio_cfg_output(SWITCH_PIN);
    nrf_gpio_pin_write(SWITCH_PIN, 0);

    for (;;)
    {
       if (app_nnet_get_cmd(&nnet_config, buffer)) nrf_gpio_pin_write(SWITCH_PIN, !buffer[9]);
    }
}
