#include "bsp/board_api.h"
#include "tusb.h"

#include "MyUSB_callbacks.h"

#include "FreeRTOS.h"

#define MAX_REPORT  4

static uint8_t const keycode2ascii[128][2] =  { HID_KEYCODE_TO_ASCII };

// Each HID instance can has multiple reports
static struct
{
  uint8_t report_count;
  tuh_hid_report_info_t report_info[MAX_REPORT];
}hid_info[CFG_TUH_HID];


volatile hid_keyboard_report_t keys_report = { 0, 0, {0} };
volatile bool get_char_bool = false; 
volatile char res_char = 0;

static void process_kbd_report(uint8_t dev_addr, uint8_t instance, hid_keyboard_report_t const *report);
static void process_mouse_report(hid_mouse_report_t const * report);
static void process_generic_report(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len);
static inline bool find_key_in_report(hid_keyboard_report_t const *report, uint8_t keycode);

void hid_app_task(void)
{
}

char _get_char_usb(){
  get_char_bool = true;
  while(get_char_bool) vTaskDelay(pdMS_TO_TICKS(1));
  return res_char;
}

bool _check_key_callback(uint8_t key){
  return find_key_in_report(&keys_report, key);
}

bool _check_modifier_callback(uint8_t modifier){
  return (keys_report.modifier & modifier) > 0;
}

void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len)
{
  // Interface protocol (hid_interface_protocol_enum_t)
  const char* protocol_str[] = { "None", "Keyboard", "Mouse" };
  uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);

  // By default host stack will use activate boot protocol on supported interface.
  // Therefore for this simple example, we only need to parse generic report descriptor (with built-in parser)
  if ( itf_protocol == HID_ITF_PROTOCOL_NONE )
  {
    hid_info[instance].report_count = tuh_hid_parse_report_descriptor(hid_info[instance].report_info, MAX_REPORT, desc_report, desc_len);
  }

  // request to receive report
  // tuh_hid_report_received_cb() will be invoked when report is available
  if ( !tuh_hid_receive_report(dev_addr, instance) ){}
}

// Invoked when device with hid interface is un-mounted
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance)
{
  // printf("HID device address = %d, instance = %d is unmounted\r\n", dev_addr, instance);
}

// Invoked when received report from device via interrupt endpoint
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
  uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);

  switch (itf_protocol)
  {
    case HID_ITF_PROTOCOL_KEYBOARD:
      TU_LOG2("HID receive boot keyboard report\r\n");
      process_kbd_report(dev_addr, instance, (hid_keyboard_report_t const*) report );
    break;

    case HID_ITF_PROTOCOL_MOUSE:
      TU_LOG2("HID receive boot mouse report\r\n");
      process_mouse_report( (hid_mouse_report_t const*) report );
    break;

    default:
      // Generic report requires matching ReportID and contents with previous parsed report info
      process_generic_report(dev_addr, instance, report, len);
    break;
  }

  // continue to request to receive report
  if ( !tuh_hid_receive_report(dev_addr, instance) )
  {
    // printf("Error: cannot request to receive report\r\n");
  }
}

//--------------------------------------------------------------------+
// Keyboard
//--------------------------------------------------------------------+

// look up new key in previous keys
static inline bool find_key_in_report(hid_keyboard_report_t const *report, uint8_t keycode)
{
  for(uint8_t i=0; i<6; i++)
  {
    if (report->keycode[i] == keycode)  return true;
  }

  return false;
}

static void process_kbd_report(uint8_t dev_addr, uint8_t instance, hid_keyboard_report_t const *report)
{
  static hid_keyboard_report_t prev_report = { 0, 0, {0} }; // previous report to check key released
  static uint8_t lock_report = 0;
  //------------- example code ignore control (non-printable) key affects -------------//
  keys_report = *report;
  if(get_char_bool){
    for(uint8_t i=0; i<6; i++)
    {
      if ( report->keycode[i] )
      {
        if ( find_key_in_report(&prev_report, report->keycode[i]) )
        {
          // exist in previous report means the current key is holding

        }else
        {
          switch(report->keycode[i]){
            case HID_KEY_CAPS_LOCK:

              lock_report = lock_report&KEYBOARD_LED_CAPSLOCK ?  lock_report&~KEYBOARD_LED_CAPSLOCK : lock_report|KEYBOARD_LED_CAPSLOCK;

              tuh_hid_set_report(dev_addr, instance, 0, HID_REPORT_TYPE_OUTPUT, &lock_report, sizeof(lock_report));
              break;
            case HID_KEY_NUM_LOCK:
              lock_report = lock_report&KEYBOARD_LED_NUMLOCK ?  lock_report&~KEYBOARD_LED_NUMLOCK : lock_report|KEYBOARD_LED_NUMLOCK;
              tuh_hid_set_report(dev_addr, instance, 0, HID_REPORT_TYPE_OUTPUT, &lock_report, sizeof(lock_report));
              break;
            case HID_KEY_SCROLL_LOCK:
              lock_report = lock_report&KEYBOARD_LED_SCROLLLOCK ?  lock_report&~KEYBOARD_LED_SCROLLLOCK : lock_report|KEYBOARD_LED_SCROLLLOCK;
              tuh_hid_set_report(dev_addr, instance, 0, HID_REPORT_TYPE_OUTPUT, &lock_report, sizeof(lock_report));
              break;
            default:
              bool const is_shift = report->modifier & (KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT);
              char res = keycode2ascii[report->keycode[i]][is_shift | lock_report&KEYBOARD_LED_CAPSLOCK ? 1 : 0];
              if(res){
                res_char = res;
                get_char_bool = false;
              }
              break;
          }
        }
      }
    }
  }

  prev_report = *report;
}

//--------------------------------------------------------------------+
// Mouse
//--------------------------------------------------------------------+

void cursor_movement(int8_t x, int8_t y, int8_t wheel)
{
  // printf("(%d %d %d)\r\n", x, y, wheel);
}

static void process_mouse_report(hid_mouse_report_t const * report)
{
  static hid_mouse_report_t prev_report = { 0 };

  //------------- button state  -------------//
  uint8_t button_changed_mask = report->buttons ^ prev_report.buttons;
  if ( button_changed_mask & report->buttons)
  {
    // printf(" %c%c%c ",
    //   report->buttons & MOUSE_BUTTON_LEFT   ? 'L' : '-',
    //   report->buttons & MOUSE_BUTTON_MIDDLE ? 'M' : '-',
    //   report->buttons & MOUSE_BUTTON_RIGHT  ? 'R' : '-');
  }

  //------------- cursor movement -------------//
  cursor_movement(report->x, report->y, report->wheel);
}

//--------------------------------------------------------------------+
// Generic Report
//--------------------------------------------------------------------+
static void process_generic_report(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
  (void) dev_addr;

  uint8_t const rpt_count = hid_info[instance].report_count;
  tuh_hid_report_info_t* rpt_info_arr = hid_info[instance].report_info;
  tuh_hid_report_info_t* rpt_info = NULL;

  if ( rpt_count == 1 && rpt_info_arr[0].report_id == 0)
  {
    // Simple report without report ID as 1st byte
    rpt_info = &rpt_info_arr[0];
  }else
  {
    // Composite report, 1st byte is report ID, data starts from 2nd byte
    uint8_t const rpt_id = report[0];

    // Find report id in the array
    for(uint8_t i=0; i<rpt_count; i++)
    {
      if (rpt_id == rpt_info_arr[i].report_id )
      {
        rpt_info = &rpt_info_arr[i];
        break;
      }
    }

    report++;
    len--;
  }

  if (!rpt_info)
  {
    // printf("Couldn't find report info !\r\n");
    return;
  }

  // For complete list of Usage Page & Usage checkout src/class/hid/hid.h. For examples:
  // - Keyboard                     : Desktop, Keyboard
  // - Mouse                        : Desktop, Mouse
  // - Gamepad                      : Desktop, Gamepad
  // - Consumer Control (Media Key) : Consumer, Consumer Control
  // - System Control (Power key)   : Desktop, System Control
  // - Generic (vendor)             : 0xFFxx, xx
  if ( rpt_info->usage_page == HID_USAGE_PAGE_DESKTOP )
  {
    switch (rpt_info->usage)
    {
      case HID_USAGE_DESKTOP_KEYBOARD:
        // TU_LOG1("HID receive keyboard report\r\n");
        // Assume keyboard follow boot report layout
        process_kbd_report(dev_addr, instance, (hid_keyboard_report_t const*) report );
      break;

      case HID_USAGE_DESKTOP_MOUSE:
        // TU_LOG1("HID receive mouse report\r\n");
        // Assume mouse follow boot report layout
        process_mouse_report( (hid_mouse_report_t const*) report );
      break;

      default: break;
    }
  }
}
