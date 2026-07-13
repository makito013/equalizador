#pragma once

#include "IHotkeyBackend.h"

#include <memory>

#if defined(_WIN32)
    #include <atomic>
    #include <thread>
    #include <vector>
#endif

namespace eq
{
#if defined(_WIN32)
/**
    Windows implementation using RegisterHotKey on a dedicated message-only
    thread. NOTE: not compiled or exercised on macOS (this session runs on a
    Mac); the code exists so the Windows port is a matter of building, not
    writing. See Win32HotkeyBackend.cpp.
*/
class Win32HotkeyBackend : public IHotkeyBackend
{
public:
    Win32HotkeyBackend();
    ~Win32HotkeyBackend() override;

    void setCallback(Callback cb) override;
    bool registerHotkey(const HotkeyBinding& binding) override;
    void unregisterAll() override;
    bool isSupported() const override { return true; }

private:
    void threadMain();

    Callback callback;
    std::thread messageThread;
    std::atomic<bool> running { false };
    unsigned long threadId { 0 }; // DWORD, kept as unsigned long to avoid <windows.h> in header
    std::vector<HotkeyBinding> pending;
};
#endif // _WIN32

/**
    Returns the best hotkey backend for the current platform:
    Win32HotkeyBackend on Windows, NoopHotkeyBackend everywhere else.
*/
std::unique_ptr<IHotkeyBackend> createPlatformHotkeyBackend();
} // namespace eq
