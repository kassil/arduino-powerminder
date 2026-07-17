#include "relay.h"
#include <Arduino.h>

namespace {

// Cached logical state so relay_is_on() never has to read the pin back.
bool s_load_on = false;

// Translate a logical load state into a physical pin level, honouring the
// board polarity, and remember the new state.
void drive(bool on)
{
    const uint8_t level = (on ^ RELAY_ACTIVE_LOW) ? HIGH : LOW;
    digitalWrite(RELAY_PIN, level);
    s_load_on = on;
}

} // namespace

void relay_init()
{
    // Establish the ON level before enabling the output so the pin does not
    // glitch through the OFF state as it becomes a driven output.
    drive(true);
    pinMode(RELAY_PIN, OUTPUT);
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
