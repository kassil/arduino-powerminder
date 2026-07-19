#include "console_logic_internal.h"

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

uint16_t remaining_tenths(uint32_t now_ms, uint32_t end_ms)
{
    if (static_cast<int32_t>(now_ms - end_ms) < 0) {
        return static_cast<uint16_t>((end_ms - now_ms) / 100UL);
    }
    return 0;
}

} // namespace console_logic
