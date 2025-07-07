#pragma once

#include <shared_mutex>
#include <string>
#include <vector>

namespace fakeroblox {

class ScriptConsole {
public:
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
    static std::shared_mutex mutex;
    static std::vector<Message> messages;
    static size_t message_count;

    static void clear();

    static void log(std::string message, Message::Type type);
    static void info(std::string message);
    static void warning(std::string message);
    static void error(std::string message);
    static void debug(std::string message);
};

}; // namespace fakeroblox
