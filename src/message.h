#pragma once
#include <string>
#include <ctime>

enum class MessageRole {
    USER,
    ASSISTANT,
    SYSTEM
};

struct Message {
    MessageRole role;
    std::string content;
    time_t timestamp;
    
    Message(MessageRole r, const std::string& c);
    std::string getRoleString() const;
};

