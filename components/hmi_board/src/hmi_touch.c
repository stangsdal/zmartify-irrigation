#include "hmi_7b_touch.h"

bool hmi_touch_detect(void)
{
    return hmi_7b_touch_detect();
}

bool hmi_touch_init(void)
{
    return hmi_7b_touch_init();
}

bool hmi_touch_read(uint16_t *x, uint16_t *y, bool *pressed)
{
    return hmi_7b_touch_read(x, y, pressed);
}
