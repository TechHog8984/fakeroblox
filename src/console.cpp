#include "console.hpp"

#include <cstdarg>
#include "imgui.h"

namespace fakeroblox {

Console::Console(bool show_info, bool show_warning, bool show_error, bool show_debug)
    : show_info(show_info), show_warning(show_warning), show_error(show_error), show_debug(show_debug) {};

Console Console::ScriptConsole;
Console Console::TestsConsole(true, true, true, true);

void Console::clear() {
    std::shared_lock lock(mutex);
    message_count = 0;
    messages.clear();
}

void Console::renderMessages() {
    for (size_t i = 0; i < message_count; i++) {
        bool skip = false;
        auto& message = messages[i];
        ImVec4 color;
        switch (message.type) {
            case Console::Message::INFO:
                skip = !show_info;
                color = ImVec4{1,1,1,1};
                break;
            case Console::Message::WARNING:
                skip = !show_warning;
                color = ImVec4{1,1,0,1};
                break;
            case Console::Message::ERROR:
                skip = !show_error;
                color = ImVec4{1,0,0,1};
                break;
            case Console::Message::DEBUG:
                skip = !show_debug;
                color = ImVec4{1,0,1,1};
                break;
        }
        if (skip) continue;
        std::string content = message.content;
        size_t size = content.size();
        if (size == 0) {
            const float dec = 0.14;
            if (color.x >= dec) color.x -= dec;
            if (color.y >= dec) color.y -= dec;
            if (color.z >= dec) color.z -= dec;
            if (color.w >= dec) color.w -= dec;
            content.assign("[empty]");
            size = content.size();
        }
        ImGui::TextColored(color, "%.*s", static_cast<int>(size), content.c_str());
    }
}

void Console::log(std::string message, Message::Type type) {
    std::shared_lock lock(mutex);
    message_count++;
    messages.push_back({ .type = type, .content = message });
}

void Console::info(std::string message) {
    log(message, Message::INFO);
}
void Console::warning(std::string message) {
    log(message, Message::WARNING);
}
void Console::error(std::string message) {
    log(message, Message::ERROR);
}
void Console::debug(std::string message) {
    log(message, Message::DEBUG);
}

std::string format(const char* fmt, va_list args) {
    va_list args_copy;
    va_copy(args_copy, args);

    int size = std::vsnprintf(nullptr, 0, fmt, args_copy);
    va_end(args_copy);

    std::vector<char> buffer(size + 1);
    std::vsnprintf(buffer.data(), buffer.size(), fmt, args);
    return std::string(buffer.data(), size);
}

#define internal_logf(type) { \
    va_list args; \
    va_start(args, fmt); \
    std::string result = format(fmt, args); \
    va_end(args); \
\
    log(result, type); \
}

void Console::logf(Message::Type type, const char* fmt, ...) internal_logf(type)

void Console::infof(const char* fmt, ...) internal_logf(Message::INFO)
void Console::warningf(const char* fmt, ...) internal_logf(Message::WARNING)
void Console::errorf(const char* fmt, ...) internal_logf(Message::ERROR)
void Console::debugf(const char* fmt, ...) internal_logf(Message::DEBUG)

#undef internal_logf

}; // namespace fakeroblox
