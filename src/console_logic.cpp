#include "console_logic_internal.h"

#include <string.h>

#ifdef __AVR__
#include <avr/pgmspace.h>
#define CMD_EQ(cmd, lit) (strcmp_P((cmd), PSTR(lit)) == 0)
#else
#define CMD_EQ(cmd, lit) (strcmp((cmd), (lit)) == 0)
#endif

namespace {

bool is_space_tab(char c)
{
    return c == ' ' || c == '\t';
}

} // namespace

namespace console_logic {

void str_upper(char *s)
{
    for (; *s; ++s) {
        if (*s >= 'a' && *s <= 'z') {
            *s = static_cast<char>(*s - ('a' - 'A'));
        }
    }
}

void split_cmd_arg(char *line, char **cmd_out, char **arg_out)
{
    *cmd_out = nullptr;
    *arg_out = nullptr;

    char *p = line;
    while (is_space_tab(*p)) {
        ++p;
    }
    if (*p == '\0') {
        return;
    }

    *cmd_out = p;
    while (*p != '\0' && !is_space_tab(*p)) {
        ++p;
    }
    if (*p == '\0') {
        return;
    }

    *p++ = '\0';
    while (is_space_tab(*p)) {
        ++p;
    }
    if (*p == '\0') {
        return;
    }

    *arg_out = p;
    while (*p != '\0' && !is_space_tab(*p)) {
        ++p;
    }
    *p = '\0';
}

CommandId decode_command(const char *cmd_upper)
{
    if (cmd_upper == nullptr || *cmd_upper == '\0') {
        return CommandId::Empty;
    }
    if (CMD_EQ(cmd_upper, "HELP") || CMD_EQ(cmd_upper, "?")) {
        return CommandId::Help;
    }
    if (CMD_EQ(cmd_upper, "ON")) {
        return CommandId::On;
    }
    if (CMD_EQ(cmd_upper, "OFF")) {
        return CommandId::Off;
    }
    if (CMD_EQ(cmd_upper, "RESET")) {
        return CommandId::Reset;
    }
    if (CMD_EQ(cmd_upper, "SETDELAY")) {
        return CommandId::SetDelay;
    }
    if (CMD_EQ(cmd_upper, "STATUS")) {
        return CommandId::Status;
    }
    if (CMD_EQ(cmd_upper, "VERSION") || CMD_EQ(cmd_upper, "ID")) {
        return CommandId::Version;
    }
    if (CMD_EQ(cmd_upper, "REBOOT")) {
        return CommandId::Reboot;
    }
    return CommandId::Unknown;
}

ParsedLine parse_line(char *line)
{
    ParsedLine out = {CommandId::Empty, nullptr, nullptr};
    split_cmd_arg(line, &out.cmd, &out.arg);
    if (out.cmd == nullptr) {
        return out;
    }
    str_upper(out.cmd);
    out.id = decode_command(out.cmd);
    return out;
}

bool parse_u16(const char *s, uint16_t *out)
{
    if (s == nullptr || *s == '\0') {
        return false;
    }
    uint32_t v = 0;
    for (; *s; ++s) {
        if (*s < '0' || *s > '9') {
            return false;
        }
        v = v * 10 + static_cast<uint8_t>(*s - '0');
        if (v > 0xFFFFUL) {
            return false;
        }
    }
    *out = static_cast<uint16_t>(v);
    return true;
}

bool resolve_reset_tenths(const char *arg, uint16_t default_tenths, uint16_t *out)
{
    *out = default_tenths;
    if (arg == nullptr) {
        return true;
    }
    return parse_u16(arg, out);
}

bool resolve_setdelay_tenths(const char *arg, uint16_t *out)
{
    if (arg == nullptr) {
        return false;
    }
    return parse_u16(arg, out);
}

uint16_t remaining_tenths(uint32_t now_ms, uint32_t end_ms)
{
    if (static_cast<int32_t>(now_ms - end_ms) < 0) {
        return static_cast<uint16_t>((end_ms - now_ms) / 100UL);
    }
    return 0;
}

} // namespace console_logic

#undef CMD_EQ
