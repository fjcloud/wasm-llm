#pragma once
#include "chat.h"
#include "llm.h"
#include <imgui.h>
#include <string>

class UI {
public:
    UI(ChatSession& chat, LLM& llm);
    
    void setup();
    void render();
    bool processPendingGeneration(); // Returns true if generation was just started
    
private:
    // Core rendering
    void applyTerminalStyle();
    void renderHeader();
    void renderFooter();
    
    // Chat view
    void renderChatView();
    void renderMessageList();
    void renderInputArea();
    void renderLoadingIndicator();
    
    // Model management
    void renderModelDialog();
    
    ChatSession& chatSession;
    LLM& llm;
    
    // UI State
    char inputBuffer[1024];
    bool showModelDialog;
    bool autoScroll;
    float chatScrollY;
    
    // Deferred generation (to allow UI to render user message first)
    bool pendingGeneration;
    std::string pendingPrompt;
    size_t pendingResponseIndex;
    
    // Colors
    ImVec4 colorBackground;
    ImVec4 colorTerminal;
    ImVec4 colorBorder;
    ImVec4 colorUser;
    ImVec4 colorAssistant;
};

