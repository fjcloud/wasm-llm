#include "chat.h"

ChatSession::ChatSession() 
    : systemPrompt("You are Qwen, created by Alibaba Cloud. You are a helpful assistant.") {}

void ChatSession::addMessage(MessageRole role, const std::string& content) {
    messages.emplace_back(role, content);
}

void ChatSession::clearMessages() {
    messages.clear();
}

const std::vector<Message>& ChatSession::getMessages() const {
    return messages;
}

void ChatSession::setSystemPrompt(const std::string& prompt) {
    systemPrompt = prompt;
}

std::string ChatSession::getSystemPrompt() const {
    return systemPrompt;
}

std::string ChatSession::buildPrompt() const {
    // Qwen2.5 ChatML format
    std::string prompt = "<|im_start|>system\n" + systemPrompt + "<|im_end|>\n";
    
    // Include last 3 exchanges (6 messages) for context
    size_t start_idx = messages.size() > 6 ? messages.size() - 6 : 0;
    
    for (size_t i = start_idx; i < messages.size(); i++) {
        const auto& msg = messages[i];
        if (msg.role == MessageRole::USER) {
            prompt += "<|im_start|>user\n" + msg.content + "<|im_end|>\n";
        } else if (msg.role == MessageRole::ASSISTANT && !msg.content.empty()) {
            prompt += "<|im_start|>assistant\n" + msg.content + "<|im_end|>\n";
        }
    }
    
    prompt += "<|im_start|>assistant\n";
    return prompt;
}

