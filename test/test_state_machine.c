#include <assert.h>
#include <stdbool.h>

#include "zic_state_machine.h"

static void test_controller_boot_to_idle(void)
{
    zic_controller_t controller;
    zic_controller_init(&controller);

    assert(controller.state == ZIC_CTRL_BOOT);
    assert(zic_controller_apply_event(&controller, ZIC_EV_BOOT_DONE, -1));
    assert(zic_controller_apply_event(&controller, ZIC_EV_INIT_DONE, -1));
    assert(controller.state == ZIC_CTRL_IDLE);
}

static void test_controller_start_stop_zone(void)
{
    zic_controller_t controller;
    zic_controller_init(&controller);

    zic_controller_apply_event(&controller, ZIC_EV_BOOT_DONE, -1);
    zic_controller_apply_event(&controller, ZIC_EV_INIT_DONE, -1);

    assert(zic_controller_apply_event(&controller, ZIC_EV_START_ZONE, 1));
    assert(controller.state == ZIC_CTRL_RUNNING);
    assert(controller.master_valve_on == true);

    assert(zic_controller_apply_event(&controller, ZIC_EV_STOP_ZONE, 1));
    assert(controller.state == ZIC_CTRL_IDLE);
    assert(controller.master_valve_on == false);
}

int main(void)
{
    test_controller_boot_to_idle();
    test_controller_start_stop_zone();
    return 0;
}
