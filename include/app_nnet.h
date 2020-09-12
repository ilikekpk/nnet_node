#ifndef APP_NNET_H
#define APP_NNET_H

#include <stdint.h>
#include <stdbool.h>

#define APP_NNET_PACKET_SIZE 10

typedef struct { 
    uint8_t nnet_addr;
    uint8_t aes_key[16];
    uint32_t start_counter;
} nnet_config_t;


void app_nnet_init(nnet_config_t * config);
bool app_nnet_get_cmd(nnet_config_t * config, uint8_t* p_buffer);


#endif
