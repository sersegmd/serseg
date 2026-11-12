#pragma once
#include <string>
#include <vector>

// Quote each argument according to the Microsoft C runtime command-line rules.
inline std::wstring serseg_windows_quote(const std::wstring& argument) {
    std::wstring result(1, L'"');
    size_t slashes = 0;
    for (wchar_t ch : argument) {
        if (ch == L'\\') { ++slashes; continue; }
        if (ch == L'"') {
            result.append(slashes * 2 + 1, L'\\');
            result.push_back(L'"');
        } else {
            result.append(slashes, L'\\');
            result.push_back(ch);
        }
        slashes = 0;
    }
    result.append(slashes * 2, L'\\');
    result.push_back(L'"');
    return result;
}
inline std::wstring serseg_windows_command_line(const std::vector<std::wstring>& arguments) {
    std::wstring command;
    for (const auto& argument : arguments) {
        if (!command.empty()) command.push_back(L' ');
        command += serseg_windows_quote(argument);
    }
    return command;
}
