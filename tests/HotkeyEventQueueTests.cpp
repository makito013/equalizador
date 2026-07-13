#include "Hotkeys/HotkeyEventQueue.h"

#include <atomic>
#include <thread>

#include <juce_core/juce_core.h>

using namespace eq;

class HotkeyEventQueueTests : public juce::UnitTest
{
public:
    HotkeyEventQueueTests() : juce::UnitTest("HotkeyEventQueue", "eq") {}

    void runTest() override
    {
        beginTest("empty pop returns false");
        {
            HotkeyEventQueueT<8> q;
            Command out;
            expect(! q.pop(out));
            expect(q.isEmpty());
        }

        beginTest("FIFO order preserved");
        {
            HotkeyEventQueueT<8> q;
            expect(q.push(Command::playSlot(1)));
            expect(q.push(Command::toggleBypass()));
            expect(q.push(Command::playSlot(2)));

            Command out;
            expect(q.pop(out) && out.type == Command::Type::PlaySlot && out.slot == 1);
            expect(q.pop(out) && out.type == Command::Type::ToggleBypass);
            expect(q.pop(out) && out.type == Command::Type::PlaySlot && out.slot == 2);
            expect(! q.pop(out));
        }

        beginTest("push fails when full (usable capacity = N-1)");
        {
            HotkeyEventQueueT<4> q; // usable capacity 3
            expect(q.push(Command::playSlot(0)));
            expect(q.push(Command::playSlot(1)));
            expect(q.push(Command::playSlot(2)));
            expect(! q.push(Command::playSlot(3))); // full
        }

        beginTest("concurrent single-producer / single-consumer");
        {
            HotkeyEventQueueT<64> q;
            constexpr int total = 200000;
            std::atomic<bool> producerDone { false };

            std::thread producer([&]
            {
                for (int i = 0; i < total; ++i)
                    while (! q.push(Command::playSlot(i)))
                        std::this_thread::yield();
                producerDone.store(true);
            });

            int received = 0;
            bool ordered = true;
            Command out;
            while (received < total)
            {
                if (q.pop(out))
                {
                    if (out.slot != received)
                        ordered = false;
                    ++received;
                }
                else if (producerDone.load() && q.isEmpty())
                {
                    break;
                }
            }

            producer.join();
            expectEquals(received, total);
            expect(ordered, "commands were consumed out of order or duplicated");
        }
    }
};

static HotkeyEventQueueTests hotkeyEventQueueTests;
