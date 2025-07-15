#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>
#include <variant>

#include "lua.h"

namespace fakeroblox {
    struct FakeRobloxTest {
        const char* name;
        std::variant<lua_CFunction, std::string> value;
    };

    class ThreadGate {
        std::mutex mutex;
        std::condition_variable cv;
        bool done;
    public:
        std::function<void()> func;

        ThreadGate() {
            done = false;
            func = [this]() {
                {
                    std::lock_guard<std::mutex> lock(mutex);
                    done = true;
                }
                cv.notify_all();
            };
        }

        void wait() {
            std::unique_lock<std::mutex> lock(mutex);
            cv.wait(lock, [this]() { return done; });
        }
    };

    void runAllTests(lua_State* L, bool& is_running_tests, bool& all_tests_succeeded);
}; // namespace fakeroblox
