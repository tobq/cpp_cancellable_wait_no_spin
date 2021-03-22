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
