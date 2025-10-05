#include "message.h"

Message::Message(MessageRole r, const std::string& c) 
    : role(r), content(c), timestamp(std::time(nullptr)) {}

std::string Message::getRoleString() const {
    switch (role) {
        case MessageRole::USER: return "USER";
        case MessageRole::ASSISTANT: return "ASSISTANT";
        case MessageRole::SYSTEM: return "SYSTEM";
        default: return "UNKNOWN";
    }
}

