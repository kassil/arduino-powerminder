#pragma once

#include <stdint.h>

namespace console_logic {

void str_upper(char *s);

void split_cmd_arg(char *line, char **cmd_out, char **arg_out);

bool parse_u16(const char *s, uint16_t *out);

uint16_t remaining_tenths(uint32_t now_ms, uint32_t end_ms);

} // namespace console_logic
