#include <assert.h>
#include <string.h>

#include "alarm_manager.h"

static void test_warning_auto_recovery(void)
{
    alarm_manager_t manager;
    alarm_manager_init(&manager);

    alarm_manager_raise(&manager, ZIC_ALARM_HIGH_FLOW, ZIC_ALARM_WARNING);
    assert(alarm_manager_get_state(&manager, ZIC_ALARM_HIGH_FLOW) ==
           ZIC_ALARM_STATE_ACTIVE);
    assert(alarm_manager_acknowledge(&manager, ZIC_ALARM_HIGH_FLOW));
    assert(alarm_manager_get_state(&manager, ZIC_ALARM_HIGH_FLOW) ==
           ZIC_ALARM_STATE_ACKNOWLEDGED);
    alarm_manager_clear(&manager, ZIC_ALARM_HIGH_FLOW);
    assert(alarm_manager_get_state(&manager, ZIC_ALARM_HIGH_FLOW) ==
           ZIC_ALARM_STATE_CLEARED);
    assert(!alarm_manager_has_severity(&manager, ZIC_ALARM_WARNING));
}

static void test_critical_requires_resolution_and_acknowledgment(void)
{
    alarm_manager_t manager;
    alarm_manager_init(&manager);

    alarm_manager_raise(&manager, ZIC_ALARM_PRESSURE_COLLAPSE,
                        ZIC_ALARM_CRITICAL);
    assert(alarm_manager_has_lockout(&manager));
    assert(!alarm_manager_manual_clear(&manager, ZIC_ALARM_PRESSURE_COLLAPSE));
    assert(alarm_manager_acknowledge(&manager, ZIC_ALARM_PRESSURE_COLLAPSE));
    assert(alarm_manager_has_lockout(&manager));
    assert(!alarm_manager_manual_clear(&manager, ZIC_ALARM_PRESSURE_COLLAPSE));

    alarm_manager_clear(&manager, ZIC_ALARM_PRESSURE_COLLAPSE);
    assert(!alarm_manager_is_active(&manager, ZIC_ALARM_PRESSURE_COLLAPSE));
    assert(alarm_manager_get_state(&manager, ZIC_ALARM_PRESSURE_COLLAPSE) ==
           ZIC_ALARM_STATE_RESOLVED);
    assert(alarm_manager_has_lockout(&manager));
    assert(alarm_manager_manual_clear(&manager, ZIC_ALARM_PRESSURE_COLLAPSE));
    assert(!alarm_manager_has_lockout(&manager));
}

static void test_critical_severity_cannot_be_downgraded(void)
{
       alarm_manager_t manager;
       alarm_manager_init(&manager);

       alarm_manager_raise(&manager, ZIC_ALARM_HIGH_FLOW, ZIC_ALARM_CRITICAL);
       alarm_manager_raise(&manager, ZIC_ALARM_HIGH_FLOW, ZIC_ALARM_WARNING);
       alarm_manager_clear(&manager, ZIC_ALARM_HIGH_FLOW);
       assert(alarm_manager_get_state(&manager, ZIC_ALARM_HIGH_FLOW) ==
                 ZIC_ALARM_STATE_RESOLVED);
       assert(alarm_manager_has_lockout(&manager));
       assert(alarm_manager_recovery_policy(ZIC_ALARM_PIPE_BREAK, ZIC_ALARM_WARNING) ==
                 ZIC_ALARM_RECOVERY_MANUAL_CLEAR);
}

static void test_invalid_transitions_and_reactivation(void)
{
    alarm_manager_t manager;
    alarm_manager_init(&manager);

    assert(!alarm_manager_acknowledge(&manager, ZIC_ALARM_LOW_FLOW));
    assert(!alarm_manager_manual_clear(&manager, ZIC_ALARM_LOW_FLOW));
    alarm_manager_raise(&manager, ZIC_ALARM_LOW_FLOW, ZIC_ALARM_CRITICAL);
    assert(alarm_manager_acknowledge(&manager, ZIC_ALARM_LOW_FLOW));
    alarm_manager_clear(&manager, ZIC_ALARM_LOW_FLOW);
    alarm_manager_raise(&manager, ZIC_ALARM_LOW_FLOW, ZIC_ALARM_CRITICAL);
    assert(alarm_manager_get_state(&manager, ZIC_ALARM_LOW_FLOW) ==
           ZIC_ALARM_STATE_ACTIVE);
    assert(alarm_manager_acknowledge(&manager, ZIC_ALARM_LOW_FLOW));
    assert(alarm_manager_history_count(&manager) == 5u);

    zic_alarm_transition_t transition;
    assert(alarm_manager_history_get(&manager, 3u, &transition));
    assert(transition.from == ZIC_ALARM_STATE_RESOLVED);
    assert(transition.to == ZIC_ALARM_STATE_ACTIVE);
}

static void test_snapshot_round_trip_and_corruption(void)
{
       alarm_manager_t source;
       alarm_manager_init(&source);
       alarm_manager_raise(&source, ZIC_ALARM_PIPE_BREAK, ZIC_ALARM_CRITICAL);
       assert(alarm_manager_acknowledge(&source, ZIC_ALARM_PIPE_BREAK));
       alarm_manager_clear(&source, ZIC_ALARM_PIPE_BREAK);
       assert(alarm_manager_is_dirty(&source));

       alarm_manager_snapshot_t snapshot;
       assert(alarm_manager_export_snapshot(&source, &snapshot));
       alarm_manager_t restored;
       alarm_manager_init(&restored);
       assert(alarm_manager_restore_snapshot(&restored, &snapshot));
       assert(alarm_manager_get_state(&restored, ZIC_ALARM_PIPE_BREAK) ==
                 ZIC_ALARM_STATE_RESOLVED);
       assert(alarm_manager_has_lockout(&restored));
       assert(!alarm_manager_is_dirty(&restored));
       assert(alarm_manager_history_count(&restored) == 3u);

       alarm_manager_snapshot_t corrupt = snapshot;
       corrupt.alarms[0].severity = ZIC_ALARM_INFO;
       assert(!alarm_manager_restore_snapshot(&restored, &corrupt));
       assert(alarm_manager_has_lockout(&restored));
       alarm_manager_mark_persisted(&source);
       assert(!alarm_manager_is_dirty(&source));
}

int main(void)
{
    test_warning_auto_recovery();
    test_critical_requires_resolution_and_acknowledgment();
       test_critical_severity_cannot_be_downgraded();
    test_invalid_transitions_and_reactivation();
       test_snapshot_round_trip_and_corruption();
    return 0;
}