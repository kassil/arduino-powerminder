#include "config.h"
#include <avr/eeprom.h>

namespace {

// A magic byte marks the EEPROM as programmed by this firmware; without it we
// treat the stored bytes as uninitialized and fall back to the compiled-in
// default.
constexpr uint8_t EE_MAGIC_VALUE = 0xA5;

// EEPROM storage layout (addresses assigned by the linker via EEMEM).
uint8_t  EEMEM ee_magic;
uint16_t EEMEM ee_reset_tenths;

// RAM cache so reads never touch the EEPROM.
uint16_t s_reset_tenths = CONFIG_DEFAULT_RESET_TENTHS;

} // namespace

void config_init()
{
    if (eeprom_read_byte(&ee_magic) == EE_MAGIC_VALUE) {
        s_reset_tenths = eeprom_read_word(&ee_reset_tenths);
    } else {
        s_reset_tenths = CONFIG_DEFAULT_RESET_TENTHS;
    }
}

uint16_t config_reset_tenths()
{
    return s_reset_tenths;
}

void config_set_reset_tenths(uint16_t tenths)
{
    s_reset_tenths = tenths;
    // eeprom_update_* only writes cells that actually change, sparing wear.
    eeprom_update_word(&ee_reset_tenths, tenths);
    eeprom_update_byte(&ee_magic, EE_MAGIC_VALUE);
}
