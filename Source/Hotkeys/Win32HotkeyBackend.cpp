#include "Win32HotkeyBackend.h"

// The Win32 implementation below is intentionally compiled only on Windows.
// On this macOS dev machine only the factory (bottom of file) is built, which
// returns the NoopHotkeyBackend. The Windows path is real code, just not
// verifiable here.

#if defined(_WIN32)

    #include <windows.h>

namespace eq
{
namespace
{
    // Custom messages posted to the hotkey thread.
    constexpr UINT kMsgRegister = WM_APP + 1;
    constexpr UINT kMsgQuit     = WM_APP + 2;
}

Win32HotkeyBackend::Win32HotkeyBackend()
{
    running.store(true);
    messageThread = std::thread([this] { threadMain(); });

    // Spin until the message queue exists so early registerHotkey() calls land.
    while (threadId == 0 && running.load())
        std::this_thread::yield();
}

Win32HotkeyBackend::~Win32HotkeyBackend()
{
    running.store(false);
    if (threadId != 0)
        PostThreadMessage(static_cast<DWORD>(threadId), kMsgQuit, 0, 0);
    if (messageThread.joinable())
        messageThread.join();
}

void Win32HotkeyBackend::setCallback(Callback cb)
{
    callback = std::move(cb);
}

bool Win32HotkeyBackend::registerHotkey(const HotkeyBinding& binding)
{
    if (threadId == 0)
        return false;

    // RegisterHotKey must run on the thread that owns the message queue, so we
    // hand the binding to the hotkey thread. The id doubles as the WM_HOTKEY id.
    pending.push_back(binding);
    return PostThreadMessage(static_cast<DWORD>(threadId), kMsgRegister,
                             static_cast<WPARAM>(pending.size() - 1), 0) != 0;
}

void Win32HotkeyBackend::unregisterAll()
{
    if (threadId == 0)
        return;

    for (const auto& b : pending)
        UnregisterHotKey(nullptr, b.id);
    pending.clear();
}

void Win32HotkeyBackend::threadMain()
{
    // Force message-queue creation for this thread.
    MSG msg;
    PeekMessage(&msg, nullptr, WM_USER, WM_USER, PM_NOREMOVE);
    threadId = static_cast<unsigned long>(GetCurrentThreadId());

    while (running.load() && GetMessage(&msg, nullptr, 0, 0) > 0)
    {
        if (msg.message == kMsgQuit)
            break;

        if (msg.message == kMsgRegister)
        {
            const auto index = static_cast<size_t>(msg.wParam);
            if (index < pending.size())
            {
                const auto& b = pending[index];
                RegisterHotKey(nullptr, b.id,
                               static_cast<UINT>(b.modifiers),
                               static_cast<UINT>(b.keyCode));
            }
            continue;
        }

        if (msg.message == WM_HOTKEY)
        {
            if (callback)
                callback(static_cast<int>(msg.wParam));
            continue;
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    for (const auto& b : pending)
        UnregisterHotKey(nullptr, b.id);
}
} // namespace eq

#endif // _WIN32

namespace eq
{
std::unique_ptr<IHotkeyBackend> createPlatformHotkeyBackend()
{
#if defined(_WIN32)
    return std::make_unique<Win32HotkeyBackend>();
#else
    return std::make_unique<NoopHotkeyBackend>();
#endif
}
} // namespace eq
