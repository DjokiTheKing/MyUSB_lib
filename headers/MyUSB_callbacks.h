#ifndef MY_USB_CALLBACKS_H
#define MY_USB_CALLBACKS_H

#include <stdint.h>

bool _check_modifier_callback(uint8_t modifier);
bool _check_key_callback(uint8_t key);
char _get_char_usb();

#endif // MY_USB_CALLBACKS_H