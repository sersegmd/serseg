#pragma once
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <climits>
#include "windows_quote.hpp"
#include <filesystem>
#include <string>
#include <stdexcept>
#include <vector>

inline std::wstring serseg_widen(const std::string& value) {
    if (value.empty()) return {};
    if (value.size() > static_cast<size_t>(INT_MAX)) throw std::runtime_error("Windows argument is too long");
    int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), nullptr, 0);
    if (!length) throw std::runtime_error("Invalid UTF-8 in Windows argument");
    std::wstring result(static_cast<size_t>(length), L'\0');
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), &result[0], length))
        throw std::runtime_error("Cannot convert Windows argument to UTF-16");
    return result;
}
inline std::string serseg_narrow(const std::wstring& value) {
    if (value.empty()) return {};
    if (value.size() > static_cast<size_t>(INT_MAX)) throw std::runtime_error("Windows path is too long");
    int length = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    if (!length) throw std::runtime_error("Invalid UTF-16 in Windows argument");
    std::string result(static_cast<size_t>(length), '\0');
    if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), &result[0], length, nullptr, nullptr))
        throw std::runtime_error("Cannot convert Windows argument to UTF-8");
    return result;
}
inline std::filesystem::path serseg_path(const std::string& value) { return std::filesystem::path(serseg_widen(value)); }
inline int serseg_windows_execute(const std::vector<std::string>& args, const std::filesystem::path& directory = {}) {
    if (args.empty()) throw std::runtime_error("Missing process command");
    std::vector<std::wstring> wide;
    wide.reserve(args.size());
    for (const auto& arg : args) wide.push_back(serseg_widen(arg));
    std::wstring command = serseg_windows_command_line(wide);
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    const wchar_t* cwd = directory.empty() ? nullptr : directory.c_str();
    if (!CreateProcessW(nullptr, &command[0], nullptr, nullptr, TRUE, 0, nullptr, cwd, &startup, &process)) {
        throw std::runtime_error("Cannot start '" + args[0] + "' (Windows error " + std::to_string(GetLastError()) + ")");
    }
    CloseHandle(process.hThread);
    DWORD waited = WaitForSingleObject(process.hProcess, INFINITE);
    DWORD status = 0;
    bool ok = waited == WAIT_OBJECT_0 && GetExitCodeProcess(process.hProcess, &status);
    DWORD error = ok ? 0 : GetLastError();
    CloseHandle(process.hProcess);
    if (!ok) throw std::runtime_error("Cannot wait for process (Windows error " + std::to_string(error) + ")");
    return static_cast<int>(status);
}
#endif
