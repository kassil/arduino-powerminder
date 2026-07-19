/*
 * Relay / load control.
 *
 * A single GPIO drives a relay module that switches an external load.  The
 * rest of the firmware works only in terms of the logical load state (ON/OFF);
 * the electrical polarity is a private detail of this module.
 */
#pragma once
#include <stdint.h>

// Digital pin wired to the relay module.
constexpr uint8_t RELAY_PIN = 7;

// Active-low relay board: driving the pin LOW energizes the relay and powers
// the load; driving it HIGH releases the relay and removes power.  Flip this
// to false for an active-high board without touching any call sites.
constexpr bool RELAY_ACTIVE_LOW = true;

// Configure the relay pin as an output and power the load ON.
//
// Startup guarantee: relay_init() drives the ON level before enabling output,
// then drives ON again after pinMode so boot does not produce an OFF pulse.
void relay_init();

// Power the external load ON (normal running state).
void relay_on();

// Power the external load OFF.
void relay_off();

// True when the load is currently powered ON.
bool relay_is_on();
