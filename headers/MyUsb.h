#ifndef MY_USB_H
#define MY_USB_H

extern "C" {
    #include "MyUSB_callbacks.h"
}

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

class MyUsb{
    public:
        MyUsb();
        void initUSB();
        char get_char();
        bool check_key(uint8_t key);
        bool check_modifier(uint8_t modifier);
    private:
        StackType_t xUsbStack[configMINIMAL_STACK_SIZE];
        StaticTask_t xUsbControl;
        TaskHandle_t vUsbHandle;

    private:
        static void poll_task(void*);
        
};

#endif // MY_USB_H