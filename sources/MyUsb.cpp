#include "MyUsb.h"

#include "bsp/board_api.h"
#include "tusb.h"

MyUsb::MyUsb(){}

void MyUsb::initUSB()
{
    board_init();
    tuh_init(BOARD_TUH_RHPORT);

    vUsbHandle = xTaskCreateStaticAffinitySet(
        poll_task,
        "vUsb",
        configMINIMAL_STACK_SIZE,
        NULL,
        configMAX_PRIORITIES-1,
        xUsbStack,
        &xUsbControl,
        1<<0
    );
}

void MyUsb::poll_task(void*)
{
    while(true){
        tuh_task();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

char MyUsb::get_char()
{
    return _get_char_usb();
}

bool MyUsb::check_key(uint8_t key)
{
    return _check_key_callback(key);
}

bool MyUsb::check_modifier(uint8_t modifier)
{
    return _check_modifier_callback(modifier);
}
