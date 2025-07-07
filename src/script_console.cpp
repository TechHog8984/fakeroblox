#include "script_console.hpp"

using namespace fakeroblox;

std::shared_mutex ScriptConsole::mutex;
std::vector<ScriptConsole::Message> ScriptConsole::messages;
size_t ScriptConsole::message_count = 0;

bool ScriptConsole::show_info = true;
bool ScriptConsole::show_warning = true;
bool ScriptConsole::show_error = true;
bool ScriptConsole::show_debug = false;

void ScriptConsole::clear() {
    std::shared_lock lock(mutex);
    message_count = 0;
    messages.clear();
}

void ScriptConsole::log(std::string message, Message::Type type) {
    std::shared_lock lock(mutex);
    message_count++;
    messages.push_back({ .type = type, .content = message });
}
void ScriptConsole::info(std::string message) {
    log(message, Message::INFO);
}
void ScriptConsole::warning(std::string message) {
    log(message, Message::WARNING);
}
void ScriptConsole::error(std::string message) {
    log(message, Message::ERROR);
}
void ScriptConsole::debug(std::string message) {
    log(message, Message::DEBUG);
}
