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

// Fail-safe NC wiring policy:
// - OFF command drives the relay pin LOW (energized relay opens NC contact,
//   interrupting the load).
// - ON command drives the relay pin HIGH (de-energized relay closes NC
//   contact, powering the load).
// Keep this true for NC/fail-safe behavior where a dead controller releases
// the relay and the load returns ON.
constexpr bool RELAY_OFF_ACTIVE_LOW = true;

// Built-in LED: HIGH is on, LOW is off.
constexpr bool BUILTIN_LED_ACTIVE_LOW = false;

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
