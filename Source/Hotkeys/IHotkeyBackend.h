#pragma once

#include <cstdint>
#include <functional>
#include <utility>

namespace eq
{
/** A global hotkey binding, identified by an app-level command id. */
struct HotkeyBinding
{
    int id { 0 };            // app command id reported back in the callback
    int keyCode { 0 };       // platform virtual-key code
    uint32_t modifiers { 0 }; // platform modifier bitmask (ctrl/alt/shift/win)
};

/**
    Abstraction over OS global-hotkey registration.

    Global hotkeys are genuinely Windows-specific (RegisterHotKey), so this is
    one of the few interfaces we hide platform code behind. The callback fires
    on the OS input thread; implementations must keep it cheap (typically just
    a lock-free push onto HotkeyEventQueue).
*/
class IHotkeyBackend
{
public:
    virtual ~IHotkeyBackend() = default;

    using Callback = std::function<void(int /*id*/)>;

    virtual void setCallback(Callback cb) = 0;

    /**
        Registers a hotkey. If `binding.id` was already registered, it is
        transparently unregistered first (Windows requires the old
        registration to be dropped before an id can be reused with a
        different key). Callers are responsible for resolving *cross-id*
        conflicts (two different ids wanting the same key) by calling
        unregisterHotkey() on the id that currently owns that key before
        calling this - see MainComponent::registerExclusiveHotkey.
    */
    virtual bool registerHotkey(const HotkeyBinding& binding) = 0;

    /** Unregisters a single id, if currently registered. Idempotent. */
    virtual bool unregisterHotkey(int id) = 0;

    virtual void unregisterAll() = 0;

    /** False on platforms where global hotkeys are not implemented. */
    virtual bool isSupported() const = 0;
};

/**
    No-op backend used on macOS (dev) and in tests. Registration always fails
    (nothing is wired to the OS), but fire() lets tests/dev simulate a key.
*/
class NoopHotkeyBackend : public IHotkeyBackend
{
public:
    void setCallback(Callback cb) override { callback = std::move(cb); }
    bool registerHotkey(const HotkeyBinding&) override { return false; }
    bool unregisterHotkey(int) override { return false; }
    void unregisterAll() override {}
    bool isSupported() const override { return false; }

    /** Dev/test helper: pretend the hotkey with this id was pressed. */
    void fire(int id)
    {
        if (callback)
            callback(id);
    }

private:
    Callback callback;
};
} // namespace eq
