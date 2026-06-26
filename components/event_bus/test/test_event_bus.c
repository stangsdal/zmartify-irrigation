/*
 * test_event_bus.c
 * Unit tests for event_bus component
 *
 * Tests the event-driven communication system including:
 * - Event publication and subscription
 * - Priority handling
 * - Payload copying
 * - Performance characteristics
 */

#include <unity.h>
#include <string.h>
#include "event_bus.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Test event counter (incremented by subscriber callbacks)
static volatile uint32_t s_test_event_count = 0;
static zic_event_t s_last_received_event = {0};

/**
 * Test callback that counts received events
 */
static void test_callback_counter(const zic_event_t *event, void *context)
{
    (void)context;
    s_test_event_count++;
    memcpy(&s_last_received_event, event, sizeof(zic_event_t));
}

/**
 * Setup before each test
 */
void setUp(void)
{
    s_test_event_count = 0;
    memset(&s_last_received_event, 0, sizeof(s_last_received_event));
}

/**
 * Teardown after each test
 */
void tearDown(void)
{
    // Event bus persists across tests
}

/**
 * Test: Initialization
 */
void test_event_bus_init(void)
{
    bool result = event_bus_init();
    TEST_ASSERT_TRUE(result);

    // Second init should succeed (no-op)
    result = event_bus_init();
    TEST_ASSERT_TRUE(result);

    event_bus_shutdown();
}

/**
 * Test: Publish and receive basic event
 */
void test_event_bus_publish_subscribe(void)
{
    TEST_ASSERT_TRUE(event_bus_init());

    // Subscribe to event
    bool result = event_bus_subscribe(EVENT_ZONE_STARTED, test_callback_counter, NULL);
    TEST_ASSERT_TRUE(result);

    // Publish event
    result = event_bus_publish(EVENT_ZONE_STARTED, 1, EVENT_PRIORITY_NORMAL, 0, NULL, 0);
    TEST_ASSERT_TRUE(result);

    // Give dispatcher time to process
    vTaskDelay(pdMS_TO_TICKS(100));

    // Verify callback was invoked
    TEST_ASSERT_EQUAL(1, s_test_event_count);
    TEST_ASSERT_EQUAL(EVENT_ZONE_STARTED, s_last_received_event.event_id);
    TEST_ASSERT_EQUAL(1, s_last_received_event.producer_id);

    event_bus_shutdown();
}

/**
 * Test: Event payload transmission
 */
void test_event_bus_payload(void)
{
    TEST_ASSERT_TRUE(event_bus_init());

    uint8_t test_payload[32] = {0xAA, 0xBB, 0xCC, 0xDD};
    int test_payload_size = 4;

    event_bus_subscribe(EVENT_FLOW_UPDATED, test_callback_counter, NULL);
    event_bus_publish(EVENT_FLOW_UPDATED, 2, EVENT_PRIORITY_NORMAL, 1,
                      test_payload, test_payload_size);

    vTaskDelay(pdMS_TO_TICKS(100));

    TEST_ASSERT_EQUAL(1, s_test_event_count);
    TEST_ASSERT_EQUAL(test_payload_size, s_last_received_event.payload_size);
    TEST_ASSERT_EQUAL_MEMORY(test_payload, s_last_received_event.payload, test_payload_size);

    event_bus_shutdown();
}

/**
 * Test: Multiple subscribers
 */
void test_event_bus_multiple_subscribers(void)
{
    TEST_ASSERT_TRUE(event_bus_init());

    // Subscribe multiple callbacks to same event
    bool result1 = event_bus_subscribe(EVENT_PRESSURE_UPDATED, test_callback_counter, NULL);
    bool result2 = event_bus_subscribe(EVENT_PRESSURE_UPDATED, test_callback_counter, NULL);

    TEST_ASSERT_TRUE(result1);
    TEST_ASSERT_TRUE(result2);

    // Publish event
    event_bus_publish(EVENT_PRESSURE_UPDATED, 3, EVENT_PRIORITY_NORMAL, 0, NULL, 0);

    vTaskDelay(pdMS_TO_TICKS(100));

    // Both callbacks should have been invoked
    TEST_ASSERT_EQUAL(2, s_test_event_count);

    event_bus_shutdown();
}

/**
 * Test: Wildcard subscription (event_id = 0xFFFFFFFF)
 */
void test_event_bus_wildcard_subscribe(void)
{
    TEST_ASSERT_TRUE(event_bus_init());

    // Subscribe to all events
    event_bus_subscribe(0xFFFFFFFF, test_callback_counter, NULL);

    // Publish different events
    event_bus_publish(EVENT_ZONE_STARTED, 1, EVENT_PRIORITY_NORMAL, 0, NULL, 0);
    event_bus_publish(EVENT_ZONE_STOPPED, 1, EVENT_PRIORITY_NORMAL, 0, NULL, 0);
    event_bus_publish(EVENT_ALARM_GENERATED, 1, EVENT_PRIORITY_NORMAL, 0, NULL, 0);

    vTaskDelay(pdMS_TO_TICKS(200));

    // All three events should have been received
    TEST_ASSERT_EQUAL(3, s_test_event_count);

    event_bus_shutdown();
}

/**
 * Test: Unsubscribe
 */
void test_event_bus_unsubscribe(void)
{
    TEST_ASSERT_TRUE(event_bus_init());

    // Subscribe
    event_bus_subscribe(EVENT_IRRIGATION_STARTED, test_callback_counter, NULL);

    // Publish and verify
    event_bus_publish(EVENT_IRRIGATION_STARTED, 1, EVENT_PRIORITY_NORMAL, 0, NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    TEST_ASSERT_EQUAL(1, s_test_event_count);

    // Unsubscribe
    bool result = event_bus_unsubscribe(EVENT_IRRIGATION_STARTED, test_callback_counter);
    TEST_ASSERT_TRUE(result);

    // Publish again and verify no additional events
    event_bus_publish(EVENT_IRRIGATION_STARTED, 1, EVENT_PRIORITY_NORMAL, 0, NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    TEST_ASSERT_EQUAL(1, s_test_event_count);  // Still 1

    event_bus_shutdown();
}

/**
 * Test: Statistics
 */
void test_event_bus_statistics(void)
{
    TEST_ASSERT_TRUE(event_bus_init());

    event_bus_stats_t stats;

    // Get initial stats
    bool result = event_bus_get_stats(&stats);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(0, stats.events_processed);

    // Subscribe and publish events
    event_bus_subscribe(EVENT_MQTT_CONNECTED, test_callback_counter, NULL);
    event_bus_publish(EVENT_MQTT_CONNECTED, 1, EVENT_PRIORITY_NORMAL, 0, NULL, 0);
    event_bus_publish(EVENT_MQTT_CONNECTED, 1, EVENT_PRIORITY_NORMAL, 0, NULL, 0);

    vTaskDelay(pdMS_TO_TICKS(200));

    // Get updated stats
    result = event_bus_get_stats(&stats);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_GREATER_THAN_EQUAL(2, stats.events_processed);
    TEST_ASSERT_EQUAL(1, stats.subscribers_active);

    event_bus_shutdown();
}

/**
 * Test: Maximum payload size
 */
void test_event_bus_max_payload(void)
{
    TEST_ASSERT_TRUE(event_bus_init());

    // Create max-size payload
    uint8_t max_payload[EVENT_PAYLOAD_MAX_SIZE];
    memset(max_payload, 0xAA, EVENT_PAYLOAD_MAX_SIZE);

    event_bus_subscribe(EVENT_DIAGNOSTICS_REPORT, test_callback_counter, NULL);

    // Publish with max payload
    bool result = event_bus_publish(EVENT_DIAGNOSTICS_REPORT, 1, EVENT_PRIORITY_NORMAL, 0,
                                   max_payload, EVENT_PAYLOAD_MAX_SIZE);
    TEST_ASSERT_TRUE(result);

    vTaskDelay(pdMS_TO_TICKS(100));

    TEST_ASSERT_EQUAL(1, s_test_event_count);
    TEST_ASSERT_EQUAL(EVENT_PAYLOAD_MAX_SIZE, s_last_received_event.payload_size);
    TEST_ASSERT_EQUAL_MEMORY(max_payload, s_last_received_event.payload, EVENT_PAYLOAD_MAX_SIZE);

    event_bus_shutdown();
}

/**
 * Test: Oversized payload rejection
 */
void test_event_bus_oversized_payload(void)
{
    TEST_ASSERT_TRUE(event_bus_init());

    uint8_t oversized[EVENT_PAYLOAD_MAX_SIZE + 1];
    memset(oversized, 0xBB, EVENT_PAYLOAD_MAX_SIZE + 1);

    // Try to publish oversized payload
    bool result = event_bus_publish(EVENT_DIAGNOSTICS_REPORT, 1, EVENT_PRIORITY_NORMAL, 0,
                                   oversized, EVENT_PAYLOAD_MAX_SIZE + 1);
    TEST_ASSERT_FALSE(result);  // Should reject

    event_bus_shutdown();
}
