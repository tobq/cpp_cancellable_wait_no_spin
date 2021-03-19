#include <windows.h>
#include <cstdio>
#include <cinttypes>
#include <thread>
#include <iostream>

class Handle {
    HANDLE h;
    bool moved = false;

public:
    Handle(HANDLE h) : h(h) {}

    Handle(Handle &&other) {
        moved = true;
        other.h = h;
    }

    operator HANDLE() {
        return h;
    }

    auto get() {
        return h;
    }

    ~Handle() {
        if (!moved) CloseHandle(h);
    }

};

template<class...Ts>
class cancellable_sleep {
    using duration_t = std::chrono::duration<Ts...>;
    Handle event;
    duration_t duration;

public:
    cancellable_sleep(const duration_t &duration) :
            duration(duration),
            event(CreateEventA(NULL, false, false, NULL)) {
        if (event == NULL) {
            throw std::runtime_error("Failed to create system event backend for cancellable sleep");
        }
    }

    /**
     * Blocking wait
     * @return whether cancelled
     */
    bool start() {
        auto res = WaitForSingleObject(event, std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
        return res == WAIT_OBJECT_0;
    };

    void cancel() {
        SetEvent(event);
    }

    ~cancellable_sleep() {
        CloseHandle(event);
    }
};


using cycle_t = int64_t;

auto cycle_period_millis = [] {
    LARGE_INTEGER f;
    QueryPerformanceFrequency(&f);
    return 1000.0 / f.QuadPart;
}();

cycle_t get_current_cycle() {
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return t.QuadPart;
}

int main() {
    std::cout << "CPU cycle period: " << cycle_period_millis << "ms" << std::endl;
    for (size_t i = 0; i < 10; i++) {
        cancellable_sleep sleep{std::chrono::minutes(10)};
        cycle_t wakeup_cycle;

        std::thread t([&wakeup_cycle, &sleep] {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            wakeup_cycle = get_current_cycle();
            sleep.cancel();
        });

        if (sleep.start()) {
            // do stuff in case we got interrupted by the event
            auto wakeup_registered_cycle = get_current_cycle();
            auto delta_cycles = wakeup_registered_cycle - wakeup_cycle;
            printf("Took %.6fms (%lld cycles) to observe cancellation\n",
                   delta_cycles * cycle_period_millis,
                   delta_cycles);
        }

        t.join();
    }
    return 0;
}

