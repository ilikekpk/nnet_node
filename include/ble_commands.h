#ifndef BLE_COMMANDS_H
#define BLE_COMMANDS_H

typedef void (* ble_commands_handler_t)(char * command_str);

void ble_commands_init(ble_commands_handler_t handler);

#endif