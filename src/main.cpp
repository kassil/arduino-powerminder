/*
 * PowerMinder - serial-controlled relay power controller.
 *
 * On boot the external load is powered ON.  A 115200 8N1 serial console accepts
 * simple case-insensitive commands (HELP, ON, OFF, RESET, DELAY, STATUS,
 * VERSION, REBOOT) to control and power-cycle the load.
 */
#include "relay.h"
#include "config.h"
#include "console.h"

#include <Arduino.h>
#include <avr/wdt.h>

void setup()
{
    // Critical startup order:
    // 1) relay_init() establishes ON with no OFF pulse on boot.
    // 2) config_init() restores persisted defaults.
    // 3) console_init() publishes the command interface.
    // 4) watchdog is armed after services are ready.

    // Power the external load on as early as possible.
    relay_init();

    // Restore persisted settings (the default RESET pulse length).
    config_init();

    // Bring up the serial console and greet the operator.
    console_init();

    // Recover automatically if the firmware ever hangs.
    wdt_enable(WDTO_2S);
}

void loop()
{
    console_tick();
    wdt_reset(); // feed the watchdog
}
