#include "console.hpp"
#include "imgui.h"
#include "raylib.h"

#include <cstdarg>

namespace fakeroblox {

Console::Console(ConsoleId id, bool show_info, bool show_warning, bool show_error, bool show_debug)
    : id(id), show_info(show_info), show_warning(show_warning), show_error(show_error), show_debug(show_debug) {};

Console Console::ScriptConsole(Script);
Console Console::TestsConsole(Tests, true, true, true, true);

ImVec4 Console::ColorINFO = {1, 1, 1, 1};
ImVec4 Console::ColorWARNING = {1, 1, 0, 1};
ImVec4 Console::ColorERROR = {1, 0, 0, 1};
ImVec4 Console::ColorDEBUG = {1, 0, 1, 1};

const char* Console::getMessageTypeString(Message::Type type) {
    switch (type) {
        case Message::INFO:
            return "[INFO]";
        case Message::WARNING:
            return "[WARNING]";
        case Message::ERROR:
            return "[ERROR]";
        case Message::DEBUG:
            return "[DEBUG]";
    }
    return nullptr;
}

void Console::clear() {
    std::lock_guard lock(mutex);
    messages.clear();
}

void Console::renderMessages() {
    std::shared_lock lock(mutex);
    for (size_t i = 0; i < messages.size(); i++) {
        bool skip = false;
        auto& message = messages[i];
        ImVec4 color;
        switch (message.type) {
            case Console::Message::INFO:
                skip = !show_info;
                color = ColorINFO;
                break;
            case Console::Message::WARNING:
                skip = !show_warning;
                color = ColorWARNING;
                break;
            case Console::Message::ERROR:
                skip = !show_error;
                color = ColorERROR;
                break;
            case Console::Message::DEBUG:
                skip = !show_debug;
                color = ColorDEBUG;
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
        std::string popupid = "consolepopup";
        popupid.append(std::to_string(i));

        ImGui::TextColored(color, "%.*s", static_cast<int>(size), content.c_str());
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
            ImGui::OpenPopup(ImGui::GetID(popupid.c_str()));

        if (ImGui::BeginPopup(popupid.c_str())) {
            const bool copy = ImGui::Button("copy");
            const bool any = copy;
            if (copy)
                SetClipboardText(content.c_str());

            if (any)
                ImGui::CloseCurrentPopup();

            ImGui::EndPopup();
        }
    }
}
// NOTE: not thread-safe
std::string& Console::getWholeContent() {
    whole_content.clear();

    std::shared_lock lock(mutex);
    for (size_t i = 0; i < messages.size(); i++) {
        bool skip = false;
        auto& message = messages[i];
        switch (message.type) {
            case Console::Message::INFO:
                skip = !show_info;
                break;
            case Console::Message::WARNING:
                skip = !show_warning;
                break;
            case Console::Message::ERROR:
                skip = !show_error;
                break;
            case Console::Message::DEBUG:
                skip = !show_debug;
                break;
        }
        if (skip) continue;

        whole_content.append(message.content);
        whole_content += '\n';
    }
    if (whole_content.size())
        whole_content.erase(whole_content.size() - 1, 1);

    return whole_content;
}

void Console::log(std::string message, Message::Type type) {
    std::lock_guard lock(mutex);
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
