#pragma once

#include <stdint.h>

namespace console_logic {

enum class CommandId : uint8_t {
	Empty,
	Help,
	On,
	Off,
	Reset,
	SetDelay,
	Status,
	Version,
	Reboot,
	Unknown,
};

struct ParsedLine {
	CommandId id;
	char *cmd;
	char *arg;
};

void str_upper(char *s);

void split_cmd_arg(char *line, char **cmd_out, char **arg_out);

CommandId decode_command(const char *cmd_upper);

ParsedLine parse_line(char *line);

bool parse_u16(const char *s, uint16_t *out);

bool resolve_reset_tenths(const char *arg, uint16_t default_tenths, uint16_t *out);

bool resolve_setdelay_tenths(const char *arg, uint16_t *out);

uint16_t remaining_tenths(uint32_t now_ms, uint32_t end_ms);

} // namespace console_logic
