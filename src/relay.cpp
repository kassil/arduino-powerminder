#include "relay.h"
#include <Arduino.h>

namespace {

// Cached logical state so relay_is_on() never has to read the pin back.
bool s_load_on = false;

// Translate a logical load state into a physical relay pin level and remember
// the new state.
void drive(bool on)
{
    // Map logical load state to relay GPIO with NC/fail-safe policy.
    const uint8_t relay_level = (on ^ RELAY_OFF_ACTIVE_LOW) ? LOW : HIGH;
    digitalWrite(RELAY_PIN, relay_level);
    const uint8_t led_level = (on ^ BUILTIN_LED_ACTIVE_LOW) ? HIGH : LOW;
    digitalWrite(LED_BUILTIN, led_level); // mirror the load state on the built-in LED
    s_load_on = on;
}

} // namespace

void relay_init()
{
    // Establish the ON level before enabling the output so the pin does not
    // glitch through the OFF state as it becomes a driven output.
    drive(true);
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(LED_BUILTIN, OUTPUT);
    drive(true);
}

void relay_on()
{
    drive(true);
}

void relay_off()
{
    drive(false);
}

bool relay_is_on()
{
    return s_load_on;
}
