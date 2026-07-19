#include "console.h"
#include "relay.h"
#include "config.h"
#include "version.h"
#include "console_logic_internal.h"

#include <Arduino.h>
#include <avr/wdt.h>

namespace {

// ---- Line buffer -----------------------------------------------------------

constexpr uint8_t LINE_MAX = 32;
char    s_line[LINE_MAX];
uint8_t s_len = 0;
char    s_prev = 0; // last raw char, used to collapse CRLF into one line end

// ---- Non-blocking RESET pulse ----------------------------------------------

bool     s_reset_active = false;
uint32_t s_reset_end_ms = 0;

// ---- Small helpers ---------------------------------------------------------

void print_prompt()
{
    Serial.print(F("> "));
}

void print_reset_default()
{
    Serial.print(F("reset-default="));
    Serial.print(config_reset_tenths());
    Serial.println(F(" tenths"));
}

// ---- Command handlers ------------------------------------------------------

void cmd_help()
{
    Serial.println(F(
        "Commands:\r\n"
        "  HELP            Show this help\r\n"
        "  ON              Power the load ON\r\n"
        "  OFF             Power the load OFF\r\n"
        "  RESET [tenths]  Power-cycle: OFF for [tenths] of a\r\n"
        "                  second (default if omitted), then ON.\r\n"
        "                  ON or OFF cancels a RESET in progress\r\n"
        "  SETDELAY tenths Set+save the default RESET duration\r\n"
        "  STATUS          Show load state and settings\r\n"
        "  VERSION         Show firmware name and version\r\n"
        "  REBOOT          Restart the controller"));
}

void cmd_on()
{
    s_reset_active = false; // cancel any RESET in progress
    relay_on();
    Serial.println(F("load ON"));
}

void cmd_off()
{
    s_reset_active = false; // cancel any RESET in progress
    relay_off();
    Serial.println(F("load OFF"));
}

// RESET is non-blocking: it drops the load and schedules the re-raise, then
// returns immediately so the console stays responsive.  console_tick() raises
// the load once the timer elapses.  Issuing RESET again restarts the timer;
// ON or OFF cancels it.
void cmd_reset(const char *arg)
{
    uint16_t tenths;
    if (!console_logic::resolve_reset_tenths(arg, config_reset_tenths(), &tenths)) {
        Serial.println(F("? RESET delay must be a decimal number 0..65535 tenths"));
        return;
    }

    relay_off();

    if (tenths == 0) {
        // Degenerate zero-length pulse: bring the load straight back up.
        relay_on();
        s_reset_active = false;
        Serial.println(F("reset done (load ON)"));
        return;
    }

    s_reset_active = true;
    s_reset_end_ms = millis() + static_cast<uint32_t>(tenths) * 100UL;
    Serial.print(F("reset: load OFF for "));
    Serial.print(tenths);
    Serial.println(F(" tenths (ON/OFF to cancel)..."));
}

void cmd_setdelay(const char *arg)
{
    uint16_t tenths;
    if (!console_logic::resolve_setdelay_tenths(arg, &tenths)) {
        Serial.println(F("? SETDELAY needs a decimal number 0..65535 tenths"));
        return;
    }
    config_set_reset_tenths(tenths);
    Serial.print(F("reset-default set to "));
    Serial.print(tenths);
    Serial.println(F(" tenths"));
}

void cmd_status()
{
    Serial.print(F("load="));
    Serial.println(relay_is_on() ? F("ON") : F("OFF"));
    print_reset_default();
    if (s_reset_active) {
        const uint16_t remaining_tenths =
            console_logic::remaining_tenths(millis(), s_reset_end_ms);
        Serial.print(F("reset=in-progress, "));
        Serial.print(remaining_tenths);
        Serial.println(F(" tenths left"));
    }
}

void cmd_version()
{
    Serial.println(F(FW_NAME " v" FW_VERSION));
}

void cmd_reboot()
{
    Serial.println(F("rebooting..."));
    Serial.flush();
    // Arm the shortest watchdog timeout and spin until it resets the MCU.
    wdt_enable(WDTO_15MS);
    for (;;) {
        // wait for reset
    }
}

// ---- Dispatch --------------------------------------------------------------

void dispatch(char *line)
{
    const console_logic::ParsedLine parsed = console_logic::parse_line(line);
    if (parsed.id == console_logic::CommandId::Empty) {
        return; // whitespace-only line
    }

    // Command text was already decoded once in parse_line(); branching on the
    // enum id here is cheap (if/switch both compile to simple integer control flow).

    if (parsed.id == console_logic::CommandId::Help) {
        cmd_help();
    } else if (parsed.id == console_logic::CommandId::On) {
        cmd_on();
    } else if (parsed.id == console_logic::CommandId::Off) {
        cmd_off();
    } else if (parsed.id == console_logic::CommandId::Reset) {
        cmd_reset(parsed.arg);
    } else if (parsed.id == console_logic::CommandId::SetDelay) {
        cmd_setdelay(parsed.arg);
    } else if (parsed.id == console_logic::CommandId::Status) {
        cmd_status();
    } else if (parsed.id == console_logic::CommandId::Version) {
        cmd_version();
    } else if (parsed.id == console_logic::CommandId::Reboot) {
        cmd_reboot();
    } else {
        Serial.print(F("? unknown command: "));
        Serial.println(parsed.cmd);
        Serial.println(F("type HELP for the command list"));
    }
}

} // namespace

// ---- Public API ------------------------------------------------------------

void console_init()
{
    Serial.begin(115200);
    while (!Serial) {
        // wait for the serial port to be ready (no-op on classic Uno)
    }
    Serial.println(F(
        "\r\n"
        FW_NAME " v" FW_VERSION " ready\r\n"
        "type HELP for the command list"));
    print_prompt();
}

void console_tick()
{
    // Raise the load once a non-blocking RESET pulse has elapsed.  The signed
    // difference tolerates millis() wraparound.
    if (s_reset_active &&
        static_cast<int32_t>(millis() - s_reset_end_ms) >= 0) {
        s_reset_active = false;
        relay_on();
        Serial.println(F("reset done (load ON)"));
        print_prompt();
    }

    while (Serial.available() > 0) {
        const char c = static_cast<char>(Serial.read());

        // Collapse a CRLF pair into a single line terminator.
        if (c == '\n' && s_prev == '\r') {
            s_prev = c;
            continue;
        }
        s_prev = c;

        if (c == '\r' || c == '\n') {  // line terminator
            Serial.println();
            s_line[s_len] = '\0';
            if (s_len > 0) {
                dispatch(s_line);
                s_len = 0;
            }
            print_prompt();
        } else if (c == 0x08 || c == 0x7F) { // backspace / delete
            if (s_len > 0) {
                --s_len;
                Serial.print(F("\b \b"));
            }
        } else if (c >= 0x20 && s_len < LINE_MAX - 1) {
            s_line[s_len++] = c;
            Serial.write(c); // echo so an interactive terminal shows typing
        }
        // Control characters and buffer overflow are silently ignored.
    }
}
