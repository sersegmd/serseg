#include "../compiler/windows_process.hpp"
#include <shellapi.h>
#include <iostream>
#include <stdexcept>

int main() {
    try {
        int count = 0;
        wchar_t** raw = CommandLineToArgvW(GetCommandLineW(), &count);
        if (!raw) throw std::runtime_error("CommandLineToArgvW failed");
        std::vector<std::wstring> args;
        for (int i = 0; i < count; ++i) args.emplace_back(raw[i]);
        LocalFree(raw);
        if (args.size() > 1 && args[1] == L"child") {
            const std::vector<std::wstring> expected = {L"child", L"", L"has spaces", L"trailing\\", L"quote\"test", L"backslash\\\"quote", L"عربي"};
            if (args.size() != expected.size() + 1) return 10;
            for (size_t i = 0; i < expected.size(); ++i) if (args[i + 1] != expected[i]) return 11;
            return 37;
        }
        std::vector<wchar_t> path(32768);
        DWORD size = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
        if (!size || size >= path.size()) throw std::runtime_error("Cannot locate process test executable");
        std::vector<std::string> child = {serseg_narrow(std::wstring(path.data(), size)), "child", "", "has spaces", "trailing\\", "quote\"test", "backslash\\\"quote", serseg_narrow(L"عربي")};
        if (serseg_windows_execute(child) != 37) throw std::runtime_error("CreateProcessW argument or exit code mismatch");
        std::cout << "PASS CreateProcessW argument roundtrip and exit status\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL Windows process test: " << e.what() << '\n'; return 1;
    }
}
