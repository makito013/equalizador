#pragma once

#include <array>
#include <atomic>
#include <cstddef>

namespace eq
{
/** A command produced by the hotkey layer and consumed by the audio thread. */
struct Command
{
    enum class Type
    {
        None,
        PlaySlot,
        ToggleBypass
    };

    Type type { Type::None };
    int slot { -1 }; // valid only for PlaySlot

    static Command playSlot(int slotIndex) noexcept { return { Type::PlaySlot, slotIndex }; }
    static Command toggleBypass() noexcept { return { Type::ToggleBypass, -1 }; }
};

/**
    Single-producer / single-consumer lock-free ring buffer.

    Producer: the hotkey/OS input thread (push).
    Consumer: the real-time audio thread (pop).

    Neither side allocates or locks. Capacity must be a power of two so the
    wrap-around is a cheap bit mask. The buffer keeps one slot empty to
    distinguish "full" from "empty", so usable capacity is Capacity - 1.
*/
template <std::size_t Capacity>
class HotkeyEventQueueT
{
    static_assert(Capacity >= 2 && (Capacity & (Capacity - 1)) == 0,
                  "Capacity must be a power of two >= 2");

public:
    /** Producer side. Returns false if the queue is full (command dropped). */
    bool push(const Command& command) noexcept
    {
        const auto head = writeIndex.load(std::memory_order_relaxed);
        const auto next = (head + 1) & mask;

        if (next == readIndex.load(std::memory_order_acquire))
            return false; // full

        buffer[head] = command;
        writeIndex.store(next, std::memory_order_release);
        return true;
    }

    /** Consumer side. Returns false (and leaves out untouched) if empty. */
    bool pop(Command& out) noexcept
    {
        const auto tail = readIndex.load(std::memory_order_relaxed);

        if (tail == writeIndex.load(std::memory_order_acquire))
            return false; // empty

        out = buffer[tail];
        readIndex.store((tail + 1) & mask, std::memory_order_release);
        return true;
    }

    bool isEmpty() const noexcept
    {
        return readIndex.load(std::memory_order_acquire)
               == writeIndex.load(std::memory_order_acquire);
    }

private:
    static constexpr std::size_t mask = Capacity - 1;

    std::array<Command, Capacity> buffer {};
    std::atomic<std::size_t> writeIndex { 0 };
    std::atomic<std::size_t> readIndex { 0 };
};

using HotkeyEventQueue = HotkeyEventQueueT<64>;
} // namespace eq
