#include "../src/console_logic_internal.h"

#include <stdint.h>

#include <cstring>
#include <iostream>

namespace {

int g_failures = 0;

void expect_true(bool condition, const char *msg)
{
    if (!condition) {
        std::cerr << "FAIL: " << msg << "\n";
        ++g_failures;
    }
}

void expect_false(bool condition, const char *msg)
{
    expect_true(!condition, msg);
}

void expect_streq(const char *actual, const char *expected, const char *msg)
{
    if ((actual == nullptr) != (expected == nullptr)) {
        std::cerr << "FAIL: " << msg << " (nullptr mismatch)\n";
        ++g_failures;
        return;
    }
    if (actual != nullptr && std::strcmp(actual, expected) != 0) {
        std::cerr << "FAIL: " << msg << " (expected '" << expected << "', got '" << actual << "')\n";
        ++g_failures;
    }
}

void test_parse_u16()
{
    uint16_t out = 0;

    expect_true(console_logic::parse_u16("0", &out), "parse_u16 accepts 0");
    expect_true(out == 0, "parse_u16 writes 0");

    expect_true(console_logic::parse_u16("65535", &out), "parse_u16 accepts max");
    expect_true(out == 65535U, "parse_u16 writes max");

    expect_false(console_logic::parse_u16("", &out), "parse_u16 rejects empty");
    expect_false(console_logic::parse_u16("65536", &out), "parse_u16 rejects overflow");
    expect_false(console_logic::parse_u16("12abc", &out), "parse_u16 rejects mixed token");
    expect_false(console_logic::parse_u16("-1", &out), "parse_u16 rejects minus sign");
    expect_false(console_logic::parse_u16(" 12", &out), "parse_u16 rejects leading space");
}

void test_split_cmd_arg()
{
    {
        char line[] = "RESET 30";
        char *cmd = nullptr;
        char *arg = nullptr;
        console_logic::split_cmd_arg(line, &cmd, &arg);
        expect_streq(cmd, "RESET", "split_cmd_arg extracts command");
        expect_streq(arg, "30", "split_cmd_arg extracts arg");
    }

    {
        char line[] = "\t  status\t";
        char *cmd = nullptr;
        char *arg = nullptr;
        console_logic::split_cmd_arg(line, &cmd, &arg);
        expect_streq(cmd, "status", "split_cmd_arg trims leading whitespace");
        expect_streq(arg, nullptr, "split_cmd_arg handles no arg");
    }

    {
        char line[] = "   ";
        char *cmd = nullptr;
        char *arg = nullptr;
        console_logic::split_cmd_arg(line, &cmd, &arg);
        expect_streq(cmd, nullptr, "split_cmd_arg rejects whitespace-only line");
        expect_streq(arg, nullptr, "split_cmd_arg keeps arg null for empty line");
    }

    {
        char line[] = "SETDELAY  1234   trailing";
        char *cmd = nullptr;
        char *arg = nullptr;
        console_logic::split_cmd_arg(line, &cmd, &arg);
        expect_streq(cmd, "SETDELAY", "split_cmd_arg keeps first token as command");
        expect_streq(arg, "1234", "split_cmd_arg keeps second token as arg");
    }
}

void test_str_upper()
{
    char s[] = "HeLLo123!?";
    console_logic::str_upper(s);
    expect_streq(s, "HELLO123!?", "str_upper uppercases letters only");
}

void test_remaining_tenths()
{
    expect_true(console_logic::remaining_tenths(1000UL, 2500UL) == 15U,
                "remaining_tenths converts ms to tenths");
    expect_true(console_logic::remaining_tenths(2500UL, 2500UL) == 0U,
                "remaining_tenths returns zero at deadline");
    expect_true(console_logic::remaining_tenths(3000UL, 2500UL) == 0U,
                "remaining_tenths returns zero when expired");

    const uint32_t near_wrap_now = 0xFFFFFFF0UL;
    const uint32_t end_after_wrap = 0x00000054UL;
    expect_true(console_logic::remaining_tenths(near_wrap_now, end_after_wrap) == 1U,
                "remaining_tenths handles millis wraparound");
}

void expect_command(console_logic::CommandId actual,
                    console_logic::CommandId expected,
                    const char *msg)
{
    expect_true(actual == expected, msg);
}

void test_command_decode_and_parse_line()
{
    {
        char line[] = "  help  ";
        const console_logic::ParsedLine parsed = console_logic::parse_line(line);
        expect_command(parsed.id, console_logic::CommandId::Help,
                       "parse_line decodes HELP");
        expect_streq(parsed.cmd, "HELP", "parse_line uppercases command token");
        expect_streq(parsed.arg, nullptr, "parse_line keeps null arg when missing");
    }

    {
        char line[] = "id";
        const console_logic::ParsedLine parsed = console_logic::parse_line(line);
        expect_command(parsed.id, console_logic::CommandId::Version,
                       "parse_line decodes ID as VERSION alias");
    }

    {
        char line[] = "?";
        const console_logic::ParsedLine parsed = console_logic::parse_line(line);
        expect_command(parsed.id, console_logic::CommandId::Help,
                       "parse_line decodes ? as HELP alias");
    }

    {
        char line[] = "reset 40";
        const console_logic::ParsedLine parsed = console_logic::parse_line(line);
        expect_command(parsed.id, console_logic::CommandId::Reset,
                       "parse_line decodes RESET");
        expect_streq(parsed.arg, "40", "parse_line extracts RESET arg");
    }

    {
        char line[] = "bogus";
        const console_logic::ParsedLine parsed = console_logic::parse_line(line);
        expect_command(parsed.id, console_logic::CommandId::Unknown,
                       "parse_line marks unknown commands");
        expect_streq(parsed.cmd, "BOGUS", "parse_line still normalizes unknown command");
    }

    {
        char line[] = " \t  ";
        const console_logic::ParsedLine parsed = console_logic::parse_line(line);
        expect_command(parsed.id, console_logic::CommandId::Empty,
                       "parse_line marks whitespace-only input empty");
        expect_streq(parsed.cmd, nullptr, "parse_line keeps cmd null on empty input");
    }
}

void test_argument_policy_helpers()
{
    uint16_t out = 0;

    expect_true(console_logic::resolve_reset_tenths(nullptr, 25U, &out),
                "resolve_reset_tenths accepts missing arg");
    expect_true(out == 25U, "resolve_reset_tenths uses default when missing arg");

    expect_true(console_logic::resolve_reset_tenths("123", 25U, &out),
                "resolve_reset_tenths parses explicit arg");
    expect_true(out == 123U, "resolve_reset_tenths writes parsed explicit arg");

    expect_false(console_logic::resolve_reset_tenths("12x", 25U, &out),
                 "resolve_reset_tenths rejects malformed explicit arg");

    expect_false(console_logic::resolve_setdelay_tenths(nullptr, &out),
                 "resolve_setdelay_tenths requires argument");

    expect_true(console_logic::resolve_setdelay_tenths("456", &out),
                "resolve_setdelay_tenths parses valid argument");
    expect_true(out == 456U, "resolve_setdelay_tenths writes parsed value");

    expect_false(console_logic::resolve_setdelay_tenths("-1", &out),
                 "resolve_setdelay_tenths rejects malformed argument");
}

} // namespace

int main()
{
    test_parse_u16();
    test_split_cmd_arg();
    test_str_upper();
    test_remaining_tenths();
    test_command_decode_and_parse_line();
    test_argument_policy_helpers();

    if (g_failures != 0) {
        std::cerr << g_failures << " test(s) failed\n";
        return 1;
    }

    std::cout << "All console logic tests passed\n";
    return 0;
}
