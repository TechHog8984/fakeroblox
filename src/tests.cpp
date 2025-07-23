#include "tests.hpp"

#include <cstring>
#include <shared_mutex>
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
        "},
        { .name = "Enum equality", .value = "assert(Enum.KeyCode == Enum.KeyCode) "},
        { .name = "EnumItem equality", .value = "assert(Enum.KeyCode.A == Enum.KeyCode.A)" },
        { .name = "ServiceProvider", .value = "if shared.ignoreserviceprovider then \
                warn('SKIPPING SERVICEPROVIDER TEST SINCE IT ALREADY RAN') \
                return \
            end \
            shared.ignoreserviceprovider = true \
            assert(not game:FindService('HeightmapImporterService')) \
            assert(game:GetService('HeightmapImporterService')) \
            assert(game:FindService('HeightmapImporterService')) \
        "},
        { .name = "UDim2.new", .value = "local obj = UDim2.new() \
            local function check(a,b,c,d) \
                assert(obj.X.Scale == a, 'xscale: ' .. a .. ', ' .. b .. ', ' .. c .. ', ' .. d .. ' | ' .. tostring(obj)) \
                assert(obj.X.Offset == b, 'xoffset: ' .. a .. ', ' .. b .. ', ' .. c .. ', ' .. d .. ' | ' .. tostring(obj)) \
                assert(obj.Y.Scale == c, 'yscale: ' .. a .. ', ' .. b .. ', ' .. c .. ', ' .. d .. ' | ' .. tostring(obj)) \
                assert(obj.Y.Offset == d, 'yoffset: ' .. a .. ', ' .. b .. ', ' .. c .. ', ' .. d .. ' | ' .. tostring(obj)) \
            end \
            check(0, 0, 0, 0) \
            obj = UDim2.new(1) \
            check(1, 0, 0, 0) \
            obj = UDim2.new(nil, nil, 1) \
            check(0, 0, 1, 0) \
            obj = UDim2.new(1, 2, 3, 4) \
            check(1, 2, 3, 4) \
            obj = UDim2.new(UDim.new(1, 2)) \
            check(1, 2, 0, 0) \
            obj = UDim2.new(nil, UDim.new(1, 2)) \
            check(0, 0, 0, 0) \
            obj = UDim2.new(UDim.new(2, 1), UDim.new(4, 3)) \
            check(2, 1, 4, 3) \
            obj = UDim2.new(UDim.new('$', 2), 3, 4) \
            check(0, 2, 0, 0) \
            obj = UDim2.new(UDim.new(3, 4), 5) \
            check(3, 4, 0, 0) \
            obj = UDim2.new(UDim.new('2', 3), newproxy()) \
            check(2, 3, 0, 0) \
        "}
    };
    constexpr int test_count = sizeof(test_list) / sizeof(test_list[0]);

    // super cursed, sorry
    #define PASS "\1PASS"
    #define PASS_ERROR "failed to start thread: " PASS
    const int pass_error_length = strlen(PASS_ERROR);

    int finish_count = 0;
    bool fail = false;

    bool* is_running_tests = NULL;
    bool* all_tests_succeeded = NULL;

    std::vector<bool> feedback_ran_list;
    std::vector<Feedback> test_feedbacks;

    void setupTests(bool *is_running_tests_in, bool *all_tests_succeeded_in) {
        is_running_tests = is_running_tests_in;
        all_tests_succeeded = all_tests_succeeded_in;

        feedback_ran_list.reserve(test_count);
        test_feedbacks.reserve(test_count);
    }

    std::shared_mutex finish_mutex;
    void onFinish() {
        std::lock_guard lock(finish_mutex);

        if (++finish_count == test_count) {
            *is_running_tests = false;
            *all_tests_succeeded = !fail;
        }
    }

    void startAllTests(lua_State *L) {
        fail = false;
        finish_count = 0;

        feedback_ran_list.clear();
        test_feedbacks.clear();

        for (int i = 0; i < test_count; i++) {
            auto& test = test_list[i];

            feedback_ran_list.push_back(false);
            test_feedbacks.push_back([i](std::string error) {
                feedback_ran_list[i] = true;

                auto& test = test_list[i];
                if (error.size() == pass_error_length && error.rfind(PASS_ERROR, 0) == 0)
                    Console::TestsConsole.debugf("Test '%s' passed!", test.name);
                else {
                    fail = true;
                    Console::TestsConsole.errorf("error occured while testing '%s': %s", test.name, error.c_str());
                }
            });

            if (std::holds_alternative<lua_CFunction>(test.value)) {
                lua_pushcfunction(L, std::get<lua_CFunction>(test.value), test.name);

                auto error = tryCreateThreadAndSpawnFunction(L, test_feedbacks[i], &Console::TestsConsole);
                if (error) test_feedbacks[i](*error);
                onFinish();
            } else {
                auto error = tryRunCode(L, "TEST", std::get<std::string>(test.value).c_str(), test_feedbacks[i], [i] {
                    if (!feedback_ran_list[i])
                        test_feedbacks[i](PASS_ERROR);
                    onFinish();
                }, &Console::TestsConsole);
                if (error) test_feedbacks[i](*error);
            }
        }
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
