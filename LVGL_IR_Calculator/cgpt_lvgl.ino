#include <Arduino.h>
#include <lvgl.h>
#include <math.h>

/*
  LVGL v8/v9-style Arduino calculator example.

  Assumptions:
  - lv_init() is already called elsewhere
  - Display driver / flush callback is already registered elsewhere
  - Touchscreen input is already registered elsewhere
  - Your loop calls lv_timer_handler()

  Physical keypad flow:
  4x4 keypad -> cgpt_ex_keyboard_read() -> LVGL keypad indev
             -> focused button matrix -> LV_EVENT_KEY
             -> cgpt_ex_button_matrix_event_cb()
             -> cgpt_ex_handle_calculator_key()

  Touchscreen flow:
  LVGL button matrix -> LV_EVENT_VALUE_CHANGED
                     -> cgpt_ex_button_matrix_event_cb()
                     -> cgpt_ex_handle_calculator_key()
*/

// -----------------------------
// 4x4 keypad wiring
// -----------------------------
const uint8_t cgpt_ex_keypad_row_pins[4] = {2, 3, 4, 5};
const uint8_t cgpt_ex_keypad_col_pins[4] = {6, 7, 8, 9};

/*
   Physical keypad layout:

   1 2 3 +
   4 5 6 -
   7 8 9 *
   . 0 = /
*/
const char cgpt_ex_keypad_map[4][4] =
{
  {'1', '2', '3', '+'},
  {'4', '5', '6', '-'},
  {'7', '8', '9', '*'},
  {'.', '0', '=', '/'}
};

// -----------------------------
// LVGL objects and input device
// -----------------------------
static lv_obj_t *cgpt_ex_display_label;
static lv_obj_t *cgpt_ex_button_matrix;

static lv_indev_t *cgpt_ex_keypad_indev;
static lv_group_t *cgpt_ex_keypad_group;

static bool cgpt_ex_key_is_pressed = false;
static uint32_t cgpt_ex_lvgl_key_to_send = 0;

// -----------------------------
// Calculator state
// -----------------------------
static String cgpt_ex_display_text = "0";
static double cgpt_ex_left_value = 0.0;
static char cgpt_ex_pending_operator = 0;
static bool cgpt_ex_start_new_number = true;
static bool cgpt_ex_error_state = false;

// -----------------------------
// Calculator helpers
// -----------------------------
void cgpt_ex_update_display()
{
  lv_label_set_text(cgpt_ex_display_label, cgpt_ex_display_text.c_str());
}

void cgpt_ex_show_error()
{
  cgpt_ex_display_text = "Error";
  cgpt_ex_left_value = 0.0;
  cgpt_ex_pending_operator = 0;
  cgpt_ex_start_new_number = true;
  cgpt_ex_error_state = true;
  cgpt_ex_update_display();
}

void cgpt_ex_clear_calculator()
{
  cgpt_ex_display_text = "0";
  cgpt_ex_left_value = 0.0;
  cgpt_ex_pending_operator = 0;
  cgpt_ex_start_new_number = true;
  cgpt_ex_error_state = false;
  cgpt_ex_update_display();
}

String cgpt_ex_format_number(double cgpt_ex_value)
{
  char cgpt_ex_buffer[24];

  if (fabs(cgpt_ex_value) > 99999999.0 ||
      (fabs(cgpt_ex_value) < 0.000001 && cgpt_ex_value != 0.0))
  {
    snprintf(cgpt_ex_buffer, sizeof(cgpt_ex_buffer), "%.6e", cgpt_ex_value);
  }
  else
  {
    snprintf(cgpt_ex_buffer, sizeof(cgpt_ex_buffer), "%.6f", cgpt_ex_value);

    char *cgpt_ex_decimal_point = strchr(cgpt_ex_buffer, '.');

    if (cgpt_ex_decimal_point != NULL)
    {
      char *cgpt_ex_end = cgpt_ex_buffer + strlen(cgpt_ex_buffer) - 1;

      while (cgpt_ex_end > cgpt_ex_decimal_point && *cgpt_ex_end == '0')
      {
        *cgpt_ex_end = '\0';
        cgpt_ex_end--;
      }

      if (*cgpt_ex_end == '.')
      {
        *cgpt_ex_end = '\0';
      }
    }
  }

  return String(cgpt_ex_buffer);
}

bool cgpt_ex_apply_operator(double cgpt_ex_right_value)
{
  switch (cgpt_ex_pending_operator)
  {
    case '+':
      cgpt_ex_left_value += cgpt_ex_right_value;
      return true;

    case '-':
      cgpt_ex_left_value -= cgpt_ex_right_value;
      return true;

    case '*':
      cgpt_ex_left_value *= cgpt_ex_right_value;
      return true;

    case '/':
      if (cgpt_ex_right_value == 0.0)
      {
        return false;
      }

      cgpt_ex_left_value /= cgpt_ex_right_value;
      return true;

    default:
      cgpt_ex_left_value = cgpt_ex_right_value;
      return true;
  }
}

void cgpt_ex_handle_digit(char cgpt_ex_digit)
{
  if (cgpt_ex_error_state)
  {
    cgpt_ex_clear_calculator();
  }

  if (cgpt_ex_start_new_number || cgpt_ex_display_text == "0")
  {
    cgpt_ex_display_text = String(cgpt_ex_digit);
    cgpt_ex_start_new_number = false;
  }
  else
  {
    if (cgpt_ex_display_text.length() < 14)
    {
      cgpt_ex_display_text += cgpt_ex_digit;
    }
  }

  cgpt_ex_update_display();
}

void cgpt_ex_handle_decimal_point()
{
  if (cgpt_ex_error_state)
  {
    cgpt_ex_clear_calculator();
  }

  if (cgpt_ex_start_new_number)
  {
    cgpt_ex_display_text = "0.";
    cgpt_ex_start_new_number = false;
  }
  else if (cgpt_ex_display_text.indexOf('.') < 0)
  {
    if (cgpt_ex_display_text.length() < 14)
    {
      cgpt_ex_display_text += ".";
    }
  }

  cgpt_ex_update_display();
}

void cgpt_ex_handle_operator(char cgpt_ex_operator)
{
  if (cgpt_ex_error_state)
  {
    cgpt_ex_clear_calculator();
  }

  double cgpt_ex_current_value = cgpt_ex_display_text.toDouble();

  if (cgpt_ex_pending_operator != 0 && !cgpt_ex_start_new_number)
  {
    if (!cgpt_ex_apply_operator(cgpt_ex_current_value))
    {
      cgpt_ex_show_error();
      return;
    }

    cgpt_ex_display_text = cgpt_ex_format_number(cgpt_ex_left_value);
  }
  else
  {
    cgpt_ex_left_value = cgpt_ex_current_value;
  }

  cgpt_ex_pending_operator = cgpt_ex_operator;
  cgpt_ex_start_new_number = true;
  cgpt_ex_update_display();
}

void cgpt_ex_handle_equals()
{
  if (cgpt_ex_error_state)
  {
    cgpt_ex_clear_calculator();
    return;
  }

  if (cgpt_ex_pending_operator == 0)
  {
    return;
  }

  double cgpt_ex_right_value = cgpt_ex_display_text.toDouble();

  if (!cgpt_ex_apply_operator(cgpt_ex_right_value))
  {
    cgpt_ex_show_error();
    return;
  }

  cgpt_ex_display_text = cgpt_ex_format_number(cgpt_ex_left_value);
  cgpt_ex_pending_operator = 0;
  cgpt_ex_start_new_number = true;
  cgpt_ex_update_display();
}

void cgpt_ex_handle_calculator_key(char cgpt_ex_key)
{
  if (cgpt_ex_key >= '0' && cgpt_ex_key <= '9')
  {
    cgpt_ex_handle_digit(cgpt_ex_key);
  }
  else if (cgpt_ex_key == '.')
  {
    cgpt_ex_handle_decimal_point();
  }
  else if (cgpt_ex_key == '+' ||
           cgpt_ex_key == '-' ||
           cgpt_ex_key == '*' ||
           cgpt_ex_key == '/')
  {
    cgpt_ex_handle_operator(cgpt_ex_key);
  }
  else if (cgpt_ex_key == 'C')
  {
      cgpt_ex_clear_calculator();
  }
  else if (cgpt_ex_key == '=')
  {
    cgpt_ex_handle_equals();
  }
}

// -----------------------------
// LVGL button matrix
// -----------------------------
static const char *cgpt_ex_button_matrix_map[] =
{
  "7", "8", "9", "/", "\n",
  "4", "5", "6", "*", "\n",
  "1", "2", "3", "-", "\n",
  ".", "0", "=", "+",
  ""
};

void cgpt_ex_button_matrix_event_cb(lv_event_t *cgpt_ex_event)
{
  lv_event_code_t cgpt_ex_code = lv_event_get_code(cgpt_ex_event);
  lv_obj_t *cgpt_ex_obj = (lv_obj_t*) lv_event_get_target(cgpt_ex_event);

  if (cgpt_ex_code == LV_EVENT_VALUE_CHANGED)
  {
    uint16_t cgpt_ex_button_id = lv_btnmatrix_get_selected_btn(cgpt_ex_obj);

    const char *cgpt_ex_button_text =
      lv_btnmatrix_get_btn_text(cgpt_ex_obj, cgpt_ex_button_id);

    if (cgpt_ex_button_text != NULL)
    {
      cgpt_ex_handle_calculator_key(cgpt_ex_button_text[0]);
    }
  }
  else if (cgpt_ex_code == LV_EVENT_KEY)
  {
    uint32_t *cgpt_ex_key_ptr = (uint32_t *)lv_event_get_param(cgpt_ex_event);

    if (cgpt_ex_key_ptr != NULL)
    {
      char cgpt_ex_key = (char)(*cgpt_ex_key_ptr);
      cgpt_ex_handle_calculator_key(cgpt_ex_key);
    }
  }
}

// -----------------------------
// Physical keypad scan
// -----------------------------
void cgpt_ex_setup_keypad()
{
  /*
  for (uint8_t cgpt_ex_row = 0; cgpt_ex_row < 4; cgpt_ex_row++)
  {
    pinMode(cgpt_ex_keypad_row_pins[cgpt_ex_row], OUTPUT);
    digitalWrite(cgpt_ex_keypad_row_pins[cgpt_ex_row], HIGH);
  }

  for (uint8_t cgpt_ex_col = 0; cgpt_ex_col < 4; cgpt_ex_col++)
  {
    pinMode(cgpt_ex_keypad_col_pins[cgpt_ex_col], INPUT_PULLUP);
  }
  */
}

char cgpt_ex_scan_keypad()
{
  static int lastReceivedKey = -1;
  static uint32_t lastReceivedKeyMillis = 0;
  int receivedKey = remcon.irrx.read();

  
  if (receivedKey != -1 && lastReceivedKey == receivedKey && (lastReceivedKeyMillis + 200) > millis())
  {
    return 0;
  }
  if (receivedKey == -1)
  {
    return 0;
  }
  
  lastReceivedKey = receivedKey;
  lastReceivedKeyMillis = millis();
 
  switch (receivedKey)
  {
    /*
     * Samsung Remote:     
    
Red Power   0x02
Source    0x01
1   4
2   5
3   6
4   8
5   9
6   A
7   C
8   D
9   E
-   23
0   11
Pre-Ch    13
Vol+    7
Vol-    B
Mute    F
Ch List   0x6B
Ch+   12
Ch-   0x10
Menu/Settings   1A
Home/Smart Hub    79
Guide   4F
Tools   4B
Info    1F
Up    60
Left    65
Enter   68
Right   62
Down    61
Return    58
Exit    2D
A   6C
B   14
C   15
D   16
Manual    3F
Sports    B8
CC    25
Stop    46
Reverse   45
Play    47
Pause   4A
Forward   48
NETFLIX   F3
HULU    BB
PRIME VIDEO   F4
CC/VD   25

     */
    case 4:
      return '1';
    break;

    case 5:
      return '2';
     break;

     case 6:
       return '3';
     break;

     case 8:
      return '4';
      break;

      case 9: return '5';

      case 0xA: return '6';
      case 0xC: return '7';
      case 0xD: return '8';
      case 0xE: return '9';
      case 0x11: return '0';
      case 7: return '+';
      case 0xB: return '-';
      case 0x13: return '=';
      case 0x12: return '*';
      case 0x10: return '/';
      case 0x23: return '.';
      case 0x6B: return 'C';
      case 0x60: return LV_KEY_UP;
      case 0x61: return LV_KEY_DOWN;
      case 0x62: return LV_KEY_RIGHT;
      case 0x65: return LV_KEY_LEFT;
      case 0x68: return LV_KEY_ENTER;
      
  }
  /*
  for (uint8_t cgpt_ex_row = 0; cgpt_ex_row < 4; cgpt_ex_row++)
  {
    digitalWrite(cgpt_ex_keypad_row_pins[cgpt_ex_row], LOW);

    for (uint8_t cgpt_ex_col = 0; cgpt_ex_col < 4; cgpt_ex_col++)
    {
      if (digitalRead(cgpt_ex_keypad_col_pins[cgpt_ex_col]) == LOW)
      {
        digitalWrite(cgpt_ex_keypad_row_pins[cgpt_ex_row], HIGH);
        return cgpt_ex_keypad_map[cgpt_ex_row][cgpt_ex_col];
      }
    }

    digitalWrite(cgpt_ex_keypad_row_pins[cgpt_ex_row], HIGH);
  }
  */
  return 0;
}

// -----------------------------
// LVGL keypad input device
// -----------------------------
void cgpt_ex_keyboard_read(lv_indev_t *cgpt_ex_indev, lv_indev_data_t *cgpt_ex_data)
{
  char cgpt_ex_key = cgpt_ex_scan_keypad();

  if (cgpt_ex_key != 0 && !cgpt_ex_key_is_pressed)
  {
    cgpt_ex_key_is_pressed = true;
    cgpt_ex_lvgl_key_to_send = (uint32_t)cgpt_ex_key;

    cgpt_ex_data->state = LV_INDEV_STATE_PRESSED;
    cgpt_ex_data->key = cgpt_ex_lvgl_key_to_send;
  }
  else if (cgpt_ex_key != 0 && cgpt_ex_key_is_pressed)
  {
    cgpt_ex_data->state = LV_INDEV_STATE_PRESSED;
    cgpt_ex_data->key = cgpt_ex_lvgl_key_to_send;
  }
  else
  {
    cgpt_ex_data->state = LV_INDEV_STATE_RELEASED;
    cgpt_ex_data->key = cgpt_ex_lvgl_key_to_send;
    cgpt_ex_key_is_pressed = false;
  }
}

void cgpt_ex_setup_lvgl_keypad_indev()
{
  cgpt_ex_keypad_group = lv_group_create();

  lv_group_add_obj(cgpt_ex_keypad_group, cgpt_ex_button_matrix);
  lv_group_focus_obj(cgpt_ex_button_matrix);

  cgpt_ex_keypad_indev = lv_indev_create();
  lv_indev_set_type(cgpt_ex_keypad_indev, LV_INDEV_TYPE_KEYPAD);
  lv_indev_set_read_cb(cgpt_ex_keypad_indev, cgpt_ex_keyboard_read);
  lv_indev_set_group(cgpt_ex_keypad_indev, cgpt_ex_keypad_group);
}

// -----------------------------
// LVGL UI creation
// -----------------------------
void cgpt_ex_create_calculator_ui()
{
  lv_obj_t *cgpt_ex_screen = lv_scr_act();

  lv_obj_set_style_bg_color(cgpt_ex_screen, lv_color_hex(0x202020), 0);

  cgpt_ex_display_label = lv_label_create(cgpt_ex_screen);
  lv_label_set_text(cgpt_ex_display_label, "0");
  lv_obj_set_width(cgpt_ex_display_label, 300);
  lv_obj_set_height(cgpt_ex_display_label, 45);
  lv_obj_align(cgpt_ex_display_label, LV_ALIGN_TOP_MID, 0, 10);

  lv_obj_set_style_text_color(
    cgpt_ex_display_label,
    lv_color_hex(0xFFFFFF),
    0
  );

  lv_obj_set_style_text_font(
    cgpt_ex_display_label,
    &lv_font_montserrat_28,
    0
  );

  lv_obj_set_style_text_align(
    cgpt_ex_display_label,
    LV_TEXT_ALIGN_RIGHT,
    0
  );

  lv_obj_set_style_bg_color(
    cgpt_ex_display_label,
    lv_color_hex(0x000000),
    0
  );

  lv_obj_set_style_bg_opa(cgpt_ex_display_label, LV_OPA_COVER, 0);
  lv_obj_set_style_pad_all(cgpt_ex_display_label, 6, 0);

  cgpt_ex_button_matrix = lv_btnmatrix_create(cgpt_ex_screen);
  lv_btnmatrix_set_map(cgpt_ex_button_matrix, cgpt_ex_button_matrix_map);

  lv_obj_set_size(cgpt_ex_button_matrix, 300, 170);
  lv_obj_align(cgpt_ex_button_matrix, LV_ALIGN_BOTTOM_MID, 0, -5);

  lv_obj_set_style_text_font(
    cgpt_ex_button_matrix,
    &lv_font_montserrat_22,
    0
  );

  // Normal calculator key style
  lv_obj_set_style_bg_color(
    cgpt_ex_button_matrix,
    lv_color_hex(0x404040),
    LV_PART_ITEMS
  );

  lv_obj_set_style_text_color(
    cgpt_ex_button_matrix,
    lv_color_hex(0xFFFFFF),
    LV_PART_ITEMS
  );

  lv_obj_set_style_border_width(
    cgpt_ex_button_matrix,
    1,
    LV_PART_ITEMS
  );

  lv_obj_set_style_border_color(
    cgpt_ex_button_matrix,
    lv_color_hex(0x707070),
    LV_PART_ITEMS
  );

  // Currently selected / focused key glow
  lv_obj_set_style_bg_color(
    cgpt_ex_button_matrix,
    lv_palette_main(LV_PALETTE_ORANGE),
    LV_PART_ITEMS | LV_STATE_FOCUSED
  );

  lv_obj_set_style_text_color(
    cgpt_ex_button_matrix,
    lv_color_hex(0x000000),
    LV_PART_ITEMS | LV_STATE_FOCUSED
  );

  lv_obj_set_style_border_width(
    cgpt_ex_button_matrix,
    2,
    LV_PART_ITEMS | LV_STATE_FOCUSED
  );

  lv_obj_set_style_border_color(
    cgpt_ex_button_matrix,
    lv_color_hex(0xFFFFFF),
    LV_PART_ITEMS | LV_STATE_FOCUSED
  );

  lv_obj_set_style_shadow_width(
    cgpt_ex_button_matrix,
    25,
    LV_PART_ITEMS | LV_STATE_FOCUSED
  );

  lv_obj_set_style_shadow_color(
    cgpt_ex_button_matrix,
    lv_palette_main(LV_PALETTE_ORANGE),
    LV_PART_ITEMS | LV_STATE_FOCUSED
  );

  lv_obj_set_style_shadow_spread(
    cgpt_ex_button_matrix,
    6,
    LV_PART_ITEMS | LV_STATE_FOCUSED
  );

  // Pressed key feedback, for touchscreen or enter/select actions
  lv_obj_set_style_bg_color(
    cgpt_ex_button_matrix,
    lv_palette_main(LV_PALETTE_YELLOW),
    LV_PART_ITEMS | LV_STATE_PRESSED
  );

  lv_obj_set_style_text_color(
    cgpt_ex_button_matrix,
    lv_color_hex(0x000000),
    LV_PART_ITEMS | LV_STATE_PRESSED
  );

  lv_obj_add_event_cb(
    cgpt_ex_button_matrix,
    cgpt_ex_button_matrix_event_cb,
    LV_EVENT_VALUE_CHANGED,
    NULL
  );

  lv_obj_add_event_cb(
    cgpt_ex_button_matrix,
    cgpt_ex_button_matrix_event_cb,
    LV_EVENT_KEY,
    NULL
  );

   // Initially select "=" button
  // Button order:
  // 0:7 1:8 2:9 3:/
  // 4:4 5:5 6:6 7:*
  // 8:1 9:2 10:3 11:-
  // 12:. 13:0 14:= 15:+
  lv_btnmatrix_set_selected_btn(cgpt_ex_button_matrix, 14);
}

// -----------------------------
// Public functions to call
// -----------------------------
void cgpt_ex_setup_calculator()
{
  cgpt_ex_setup_keypad();
  cgpt_ex_create_calculator_ui();
  cgpt_ex_setup_lvgl_keypad_indev();
  cgpt_ex_clear_calculator();
}
