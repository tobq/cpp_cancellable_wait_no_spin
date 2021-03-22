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

class cancellable_sleep {
    using ms = std::chrono::milliseconds;
    Handle event;
    ms::rep duration_millis;
public:
    template<class rep_t, class period_t>
    cancellable_sleep(const std::chrono::duration<rep_t, period_t> &duration) :
            duration_millis(std::chrono::duration_cast<ms>(duration).count()),
            event(CreateEventA(nullptr, false, false, nullptr)) {
        if (event == nullptr) {
            throw std::runtime_error("Failed to create system event backend for cancellable sleep");
        }
    }

    cancellable_sleep &operator=(cancellable_sleep &&other) noexcept {
        if (this != &other) {
            event = std::move(other.event);
            duration_millis = other.duration_millis;
        }
        return *this;
    }

    cancellable_sleep(cancellable_sleep &&other) noexcept: event(std::move(other.event)),
                                                           duration_millis(other.duration_millis) {}

    /**
     * Blocking wait
     * @return whether cancelled
     */
    bool start() {
        if (duration_millis <= 0) return false;
//        println("Starting wait: {} - {}ms ", event.get(), duration_millis.count());
        return WaitForSingleObject(event, duration_millis) == WAIT_OBJECT_0;
    };

    /**
     * Blocking wait
     * @return whether cancelled
     */
    bool start(const std::stop_token &st) {
        std::stop_callback cb(st, [&] { cancel(); });
        return start();
    };

    /**
     * Idempotent cancellation
     */
    void cancel() {
        println("Cancelling wait: {} of duration {}ms", event.get(), duration_millis);
        SetEvent(event);
    }
};


template<class rep_t, class per_t>
bool cancellable_sleep_for(const std::chrono::duration<rep_t, per_t> &duration, const std::stop_token &st) {
    cancellable_sleep sleep{duration};
    return sleep.start(st);
}
