/*
 * Serial command console.
 *
 * Reads line-oriented, case-insensitive commands from the 115200 8N1 serial
 * port, dispatches them, and drives the non-blocking RESET power-cycle timer.
 *
 * Contracts:
 * - Commands are case-insensitive and accept at most one argument.
 * - ON and OFF cancel any RESET pulse in progress.
 * - RESET is non-blocking; console_tick() completes it once the timer elapses.
 */
#pragma once

// Bring up the serial port, print the banner, and show the first prompt.
void console_init();

// Service the console once per main loop iteration: consume any serial input,
// dispatch complete command lines, and advance an in-progress RESET pulse.
void console_tick();
