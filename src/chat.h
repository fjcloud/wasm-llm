#pragma once
#include "message.h"
#include <vector>
#include <string>

class ChatSession {
public:
    ChatSession();
    
    void addMessage(MessageRole role, const std::string& content);
    void clearMessages();
    const std::vector<Message>& getMessages() const;
    
    void setSystemPrompt(const std::string& prompt);
    std::string getSystemPrompt() const;
    
    std::string buildPrompt() const;
    
private:
    std::vector<Message> messages;
    std::string systemPrompt;
};

