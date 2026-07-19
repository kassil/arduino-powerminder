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

} // namespace

int main()
{
    test_parse_u16();
    test_split_cmd_arg();
    test_str_upper();
    test_remaining_tenths();

    if (g_failures != 0) {
        std::cerr << g_failures << " test(s) failed\n";
        return 1;
    }

    std::cout << "All console logic tests passed\n";
    return 0;
}
