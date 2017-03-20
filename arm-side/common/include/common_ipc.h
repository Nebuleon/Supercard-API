#ifndef COMMON_IPC_H
#define COMMON_IPC_H

#define IPC_GET_INPUT           0x0001

#define IPC_RPL_INPUT_BUTTONS   0x0001
#define     RPL_INPUT_BUTTONS_BIT    16
#define     RPL_INPUT_BUTTONS_MASK   (0xFFFF << RPL_INPUT_BUTTONS_BIT)
#define     RPL_INPUT_BUTTONS(n)     ((uint32_t) (n) << RPL_INPUT_BUTTONS_BIT)
#define IPC_RPL_INPUT_TOUCH     0x0002
#define     RPL_INPUT_TOUCH_X_BIT    16
#define     RPL_INPUT_TOUCH_X_MASK   (0xFF << RPL_INPUT_TOUCH_X_BIT)
#define     RPL_INPUT_TOUCH_X(n)     ((uint32_t) (n) << RPL_INPUT_TOUCH_X_BIT)
#define     RPL_INPUT_TOUCH_Y_BIT    24
#define     RPL_INPUT_TOUCH_Y_MASK   (0xFF << RPL_INPUT_TOUCH_Y_BIT)
#define     RPL_INPUT_TOUCH_Y(n)     ((uint32_t) (n) << RPL_INPUT_TOUCH_Y_BIT)

#define IPC_GET_RTC             0x0002
/* Sends a datamsg reply */

#define IPC_SET_BACKLIGHT       0x0003
#define     SET_BACKLIGHT_DATA_BIT   16
#define     SET_BACKLIGHT_DATA_MASK  (0x3 << SET_BACKLIGHT_DATA_BIT)
#define     SET_BACKLIGHT_DATA(n)    ((uint32_t) (n) << SET_BACKLIGHT_DATA_BIT)

#define IPC_START_RESET         0xFFFF

#endif /* !COMMON_IPC_H */
