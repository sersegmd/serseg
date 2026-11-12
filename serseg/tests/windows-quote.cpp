#include "../compiler/windows_quote.hpp"
#include <iostream>
#include <stdexcept>
static void check(const std::wstring& input, const std::wstring& expected) {
    if (serseg_windows_quote(input) != expected) throw std::runtime_error("Windows argument quoting mismatch");
}
int main() {
    try {
        check(L"", L"\"\"");
        check(L"plain", L"\"plain\"");
        check(L"a b", L"\"a b\"");
        check(L"a\\b", L"\"a\\b\"");
        check(L"a\\", L"\"a\\\\\"");
        check(L"a\"b", L"\"a\\\"b\"");
        check(L"a\\\"b", L"\"a\\\\\\\"b\"");
        check(L"عربي path", L"\"عربي path\"");
        if (serseg_windows_command_line({L"tool path", L"", L"a\\"}) != L"\"tool path\" \"\" \"a\\\\\"")
            throw std::runtime_error("Windows command line mismatch");
        std::cout << "PASS Windows argument quoting cases\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL " << e.what() << '\n'; return 1;
    }
}
