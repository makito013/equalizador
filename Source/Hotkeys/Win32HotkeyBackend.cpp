#include "Win32HotkeyBackend.h"

// The Win32 implementation below is intentionally compiled only on Windows.
// On this macOS dev machine only the factory (bottom of file) is built, which
// returns the NoopHotkeyBackend. The Windows path is real code, just not
// verifiable here.

#if defined(_WIN32)

    #include <windows.h>

    #include <future>

namespace eq
{
namespace
{
    // Custom messages posted to the hotkey thread. registerHotkey/unregisterHotkey
    // pack the whole binding into WPARAM/LPARAM (id in WPARAM, keyCode/modifiers
    // packed into LPARAM's low/high words) so no shared vector needs to be
    // handed across threads - threadMain owns `active` exclusively.
    constexpr UINT kMsgRegister      = WM_APP + 1;
    constexpr UINT kMsgQuit          = WM_APP + 2;
    constexpr UINT kMsgUnregister    = WM_APP + 3;
    constexpr UINT kMsgUnregisterAll = WM_APP + 4;
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
    // pack the binding into the message itself (id doubles as the WM_HOTKEY
    // id) rather than sharing a vector across threads.
    const auto lParam = MAKELPARAM(static_cast<WORD>(binding.keyCode),
                                   static_cast<WORD>(binding.modifiers));
    return PostThreadMessage(static_cast<DWORD>(threadId), kMsgRegister,
                             static_cast<WPARAM>(binding.id),
                             static_cast<LPARAM>(lParam)) != 0;
}

bool Win32HotkeyBackend::unregisterHotkey(int id)
{
    if (threadId == 0)
        return false;

    return PostThreadMessage(static_cast<DWORD>(threadId), kMsgUnregister,
                             static_cast<WPARAM>(id), 0) != 0;
}

void Win32HotkeyBackend::unregisterAll()
{
    if (threadId == 0)
        return;

    // `active` and UnregisterHotKey() must only ever be touched from the
    // hotkey message-thread (threadMain) - both because `active` isn't
    // synchronised for cross-thread access, and because Windows requires
    // UnregisterHotKey(nullptr, id) to run on the same thread that
    // registered the id, or it fails silently. So, like registerHotkey()/
    // unregisterHotkey(), post a request instead of touching either here.
    // Unlike those fire-and-forget calls, callers of unregisterAll() (just
    // MainComponent's destructor today) need every hotkey gone before they
    // proceed - e.g. before the message thread itself is torn down - so
    // this blocks until threadMain confirms the work is done.
    std::promise<void> done;
    std::future<void> doneFuture = done.get_future();

    if (PostThreadMessage(static_cast<DWORD>(threadId), kMsgUnregisterAll,
                          reinterpret_cast<WPARAM>(&done), 0) == 0)
        return; // message queue is gone; nothing left to synchronise on

    doneFuture.wait();
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
            const int id = static_cast<int>(msg.wParam);
            const int keyCode = static_cast<int>(LOWORD(msg.lParam));
            const auto modifiers = static_cast<uint32_t>(HIWORD(msg.lParam));

            // An id already in use must be unregistered before it can be
            // re-registered with a different key (Windows rejects a second
            // RegisterHotKey call for the same id otherwise).
            for (auto it = active.begin(); it != active.end();)
            {
                if (it->id == id)
                {
                    UnregisterHotKey(nullptr, it->id);
                    it = active.erase(it);
                }
                else
                {
                    ++it;
                }
            }

            if (RegisterHotKey(nullptr, id, static_cast<UINT>(modifiers), static_cast<UINT>(keyCode)))
                active.push_back({ id, keyCode, modifiers });
            // else: registration failed (e.g. the key combo is owned by
            // another id/app); the caller is expected to have resolved
            // same-key cross-id conflicts before calling registerHotkey.
            continue;
        }

        if (msg.message == kMsgUnregister)
        {
            const int id = static_cast<int>(msg.wParam);
            for (auto it = active.begin(); it != active.end();)
            {
                if (it->id == id)
                {
                    UnregisterHotKey(nullptr, it->id);
                    it = active.erase(it);
                }
                else
                {
                    ++it;
                }
            }
            continue;
        }

        if (msg.message == kMsgUnregisterAll)
        {
            for (const auto& b : active)
                UnregisterHotKey(nullptr, b.id);
            active.clear();

            // Signal the waiting caller (unregisterAll(), on whatever thread
            // called it) that every hotkey is now actually gone.
            auto* promise = reinterpret_cast<std::promise<void>*>(msg.wParam);
            promise->set_value();
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

    for (const auto& b : active)
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
