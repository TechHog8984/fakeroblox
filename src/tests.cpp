#include "tests.hpp"

#include <cstring>
#include <fstream>
#include <shared_mutex>
#include <thread>
#include <variant>

#include "libraries/tasklib.hpp"
#include "console.hpp"

#include "lua.h"
#include "lualib.h"

namespace fakeroblox {
    int canSpawnLuaFunction(lua_State* L);
    int canSpawnCFunction(lua_State* L);

    FakeRobloxTest test_list[] = {
        { .name = "can spawn lua function", .value = canSpawnLuaFunction },
        { .name = "can spawn C function", .value = canSpawnCFunction },
        { .name = "task.wait", .value = "local time_before = os.clock() \
            local count = math.random(1, 8000) / 10000; \
            print('waiting for ' .. count .. ' seconds') \
            task.wait(count) \
            local elapsed = (os.clock() - time_before) \
            assert(elapsed >= count, 'not enough time was elapsed') \
            print('waited for ' .. elapsed .. ' seconds')"
        },
        { .name = "instance cache", .value = "assert(game.Workspace == workspace) "},
        { .name = "instance method cache", .value = "assert(game.Destroy == workspace.Destroy)" },
        { .name = "instance method route", .value = "assert(game.children == game.GetChildren)" },
        { .name = "BindableEvent", .value = "local target = {} \
            local upvalue\
            local inst = Instance.new('BindableEvent') \
            inst.Event:Connect(function(...) upvalue = ... end) \
            assert(not upvalue) \
            inst:Fire(target) \
            task.wait(0.1) \
            assert(upvalue == target) \
        " }
    };
    constexpr int test_count = sizeof(test_list) / sizeof(test_list[0]);

    // super cursed, sorry
    #define PASS "\1PASS"
    #define PASS_ERROR "failed to start thread: " PASS
    const int pass_error_length = strlen(PASS_ERROR);

    void runAllTests(lua_State* L, bool& is_running_tests, bool& all_tests_succeeded) {
        all_tests_succeeded = false;

        std::vector<std::thread> thread_list;
        thread_list.reserve(test_count);

        bool fail = false;
        std::shared_mutex mutex;
        for (int i = 0; i < test_count; i++) {
            auto& test = test_list[i];
            Console::TestsConsole.debugf("Spawning test %s...", test.name);

            thread_list.emplace_back([&L, &fail, &mutex, &test] () {
                std::lock_guard<std::shared_mutex> lock(mutex);
                bool feedback_ran = false;
                Feedback feedback = [&fail, &test, &feedback_ran](std::string error) {
                    feedback_ran = true;
                    if (error.size() == pass_error_length && error.rfind(PASS_ERROR, 0) == 0)
                        Console::TestsConsole.debugf("Test '%s' passed!", test.name);
                    else {
                        fail = true;
                        Console::TestsConsole.errorf("error occured while testing '%s': %s", test.name, error.c_str());
                    }
                };

                if (std::holds_alternative<lua_CFunction>(test.value)) {
                    lua_pushcfunction(L, std::get<lua_CFunction>(test.value), test.name);

                    auto error = tryCreateThreadAndSpawnFunction(L, feedback, &Console::TestsConsole);
                    if (error) feedback(*error);
                } else {
                    auto gate = std::make_shared<ThreadGate>();
                    auto error = tryRunCode(L, "TEST", std::get<std::string>(test.value).c_str(), feedback, gate->func, &Console::TestsConsole);
                    if (error) feedback(*error);

                    gate->wait();

                    if (!feedback_ran)
                        feedback(PASS_ERROR);
                }
            });
        }
        for (auto& t : thread_list) {
            if (t.joinable())
                t.join();
        }

        Console::TestsConsole.debugf("All tests finished!");
        all_tests_succeeded = !fail;
        if (fail)
            Console::TestsConsole.debug("There were test failures!");
        else {
            std::ofstream f(".test_success");
            if (!f)
                Console::TestsConsole.error("failed to create .test_success file");
            f.close();
        }

        is_running_tests = false;
    }

    // FIXME: pcall just to error is redundant; reflect in underlying tests as well
    void callLoadstring(lua_State* L, const char* code) {
        lua_getglobal(L, "loadstring");
        lua_pushstring(L, code);
        lua_pushstring(L, "TEST");
        int r = lua_pcall(L, 2, 2, 0);
        switch (r) {
            case LUA_OK:
                break;
            case LUA_ERRRUN: {
                size_t l;
                const char* str = lua_tolstring(L, -1, &l);
                lua_pop(L, 1);
                luaL_errorL(L, "INTERNAL: runtime error when calling loadstring: %.*s", static_cast<int>(l), str);
                break;
            }
            default:
                luaL_error(L, "INTERNAL: unknown error when calling loadstring");
                break;
        }

        if (lua_isstring(L, 2)) {
            size_t l;
            const char* str = lua_tolstring(L, -1, &l);
            lua_pop(L, 1);
            luaL_errorL(L, "INTERNAL: failed to compile chunk: %.*s", static_cast<int>(l), str);
        }
        lua_remove(L, 2);

        r = lua_pcall(L, 0, 0, 0);
        switch (r) {
            case LUA_OK:
                break;
            case LUA_ERRRUN: {
                size_t l;
                const char* str = lua_tolstring(L, -1, &l);
                lua_pop(L, 1);
                luaL_errorL(L, "failed to run chunk: %.*s", static_cast<int>(l), str);
                break;
            }
            default:
                luaL_error(L, "INTERNAL: unknown error when calling chunk");
                break;
        }
    }

    int canSpawnLuaFunction(lua_State* L) {
        const char* code = "local val = false; \
            task.spawn(function() \
                val = true \
            end) \
            for i = 1, 1000 do (function()end)() end -- wait some time without calling task.wait \
            assert(val)";

        callLoadstring(L, code);

        luaL_error(L, PASS);
        return 0;
    }
    int cfunction_to_spawn(lua_State* L) {
        *static_cast<bool*>(lua_tolightuserdata(L, lua_upvalueindex(1))) = true;

        return 0;
    }
    int canSpawnCFunction(lua_State* L) {
        lua_getglobal(L, "task");
        lua_getfield(L, -1, "spawn");
        lua_remove(L, 1); // remove task

        bool upvalue = false;
        lua_pushlightuserdata(L, &upvalue);
        lua_pushcclosure(L, cfunction_to_spawn, "cfunction", 1);

        int r = lua_pcall(L, 1, 1, 0);
        switch (r) {
            case LUA_OK:
                break;
            case LUA_ERRRUN: {
                size_t l;
                const char* str = lua_tolstring(L, -1, &l);
                lua_pop(L, 1);
                luaL_errorL(L, "failed to spawn C function: %.*s", static_cast<int>(l), str);
                break;
            }
            default:
                luaL_error(L, "INTERNAL: unknown error when spawning C function");
                break;
        }

        if (!upvalue)
            luaL_error(L, "C function did not modify the upvalue");

        luaL_error(L, PASS);
        return 0;
    }

    #undef PASS
}; // namespace fakeroblox
