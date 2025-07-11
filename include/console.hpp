#pragma once

#include <shared_mutex>
#include <string>
#include <vector>

#include "luaconf.h"

namespace fakeroblox {

class Console {
public:
    static Console ScriptConsole;
    static Console TestsConsole;

    class Message {
    public:
        enum Type {
            INFO,
            WARNING,
            ERROR,
            DEBUG
        } type;
        std::string content;
    };
    std::shared_mutex mutex;
    std::vector<Message> messages;
    size_t message_count = 0;

    bool show_info;
    bool show_warning;
    bool show_error;
    bool show_debug;

    Console(bool show_info = true, bool show_warning = true, bool show_error = true, bool show_debug = false);

    void clear();
    void renderMessages();

    void log(std::string message, Message::Type type);

    void info(std::string message);
    void warning(std::string message);
    void error(std::string message);
    void debug(std::string message);

    void LUA_PRINTF_ATTR(3, 4) logf(Message::Type type, const char* fmt, ...);
    void LUA_PRINTF_ATTR(2, 3) debugf(const char* fmt, ...);
    void LUA_PRINTF_ATTR(2, 3) infof(const char* fmt, ...);
    void LUA_PRINTF_ATTR(2, 3) warningf(const char* fmt, ...);
    void LUA_PRINTF_ATTR(2, 3) errorf(const char* fmt, ...);
};

}; // namespace fakeroblox
