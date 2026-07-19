/*
 * Persistent configuration, stored in the ATmega's internal EEPROM.
 *
 * We hold the default RESET pulse length, so the operator's
 * preferred power-cycle duration survives reboots.
 */
#pragma once
#include <stdint.h>

// Fallback default RESET pulse length (tenths of a second) used when the
// EEPROM has never been programmed.
constexpr uint16_t CONFIG_DEFAULT_RESET_TENTHS = 50;

// Load persisted configuration from EEPROM.  Call once during startup.
void config_init();

// The default RESET pulse duration, in tenths of a second.
uint16_t config_reset_tenths();

// Update and persist the default RESET pulse duration.
void config_set_reset_tenths(uint16_t tenths);
