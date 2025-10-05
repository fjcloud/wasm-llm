#include "ui.h"
#include <cstring>

void UI::renderChatView() {
    // Calculate available height
    float availHeight = ImGui::GetContentRegionAvail().y - 80;
    
    // Message list area
    ImGui::BeginChild("MessageList", ImVec2(0, availHeight), true);
    renderMessageList();
    
    // Auto-scroll to bottom
    if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();
    
    // Input area
    renderInputArea();
}

void UI::renderMessageList() {
    const auto& messages = chatSession.getMessages();
    
    if (messages.empty()) {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), 
                          "No messages yet. Type something to start chatting...");
        return;
    }
    
    for (size_t i = 0; i < messages.size(); i++) {
        const auto& msg = messages[i];
        ImVec4 color = (msg.role == MessageRole::USER) ? colorUser : colorAssistant;
        
        // Role header
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::Text("[%s]", msg.getRoleString().c_str());
        ImGui::PopStyleColor();
        
        // Message content
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
        
        // If this is the last message and it's empty (generating), show loading animation
        if (i == messages.size() - 1 && msg.content.empty() && llm.isGenerating()) {
            renderLoadingIndicator();
        } else {
            ImGui::TextWrapped("%s", msg.content.c_str());
        }
        
        ImGui::PopStyleColor();
        
        ImGui::Spacing();
    }
}

void UI::renderLoadingIndicator() {
    // Animated "thinking" dots
    float time = ImGui::GetTime();
    int dots = (int)(time * 2.0f) % 4;
    
    ImGui::PushStyleColor(ImGuiCol_Text, colorTerminal);
    
    switch(dots) {
        case 0: ImGui::Text("."); break;
        case 1: ImGui::Text(".."); break;
        case 2: ImGui::Text("..."); break;
        case 3: ImGui::Text(".."); break;
    }
    
    ImGui::PopStyleColor();
}

void UI::renderInputArea() {
    ImGui::Spacing();
    
    // Input field (Enter to send, Ctrl+Enter for new line)
    ImGui::PushItemWidth(-120);
    bool enterPressed = ImGui::InputTextMultiline("##Input", inputBuffer, 
                                                   sizeof(inputBuffer), 
                                                   ImVec2(0, 60),
                                                   ImGuiInputTextFlags_EnterReturnsTrue | 
                                                   ImGuiInputTextFlags_CtrlEnterForNewLine);
    ImGui::PopItemWidth();
    
    ImGui::SameLine();
    
    // Send button
    bool sendClicked = ImGui::Button("SEND", ImVec2(100, 60));
    
    // Handle send - allow sending even while generating
    if ((enterPressed || sendClicked) && strlen(inputBuffer) > 0) {
        std::string userMessage = inputBuffer;
        
        // Check if model is loaded
        if (!llm.isLoaded()) {
            chatSession.addMessage(MessageRole::USER, userMessage);
            chatSession.addMessage(MessageRole::ASSISTANT, 
                                  "Please load a model first. Click 'LOAD MODEL' button to load Qwen2.5-0.5B.");
            memset(inputBuffer, 0, sizeof(inputBuffer));
            autoScroll = true;
            return;
        }
        
        // Stop any ongoing generation if user sends new message
        if (llm.isGenerating()) {
            llm.stopGeneration();
        }
        
        // Add user message immediately - it will show up right away!
        chatSession.addMessage(MessageRole::USER, userMessage);
        autoScroll = true;  // Force scroll to show the new message
        
        // Clear input immediately
        memset(inputBuffer, 0, sizeof(inputBuffer));
        
        // Build prompt
        pendingPrompt = chatSession.buildPrompt();
        
        // Add empty assistant message for loading animation
        chatSession.addMessage(MessageRole::ASSISTANT, "");
        pendingResponseIndex = chatSession.getMessages().size() - 1;
        
        // Defer generation start to next frame (after UI renders the user message)
        pendingGeneration = true;
    }
    
    // Hint text
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), 
                      llm.isGenerating() ? "Generating..." : "(Press Enter to send)");
}

void UI::renderModelDialog() {
    ImGui::OpenPopup("Load Model");
    
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(600, 400));
    
    if (ImGui::BeginPopupModal("Load Model", &showModelDialog, ImGuiWindowFlags_NoResize)) {
        ImGui::TextColored(colorTerminal, "LOAD GGUF MODEL");
        ImGui::Separator();
        ImGui::Spacing();
        
        ImGui::TextWrapped("llama.cpp is integrated! Load TinyLlama to start chatting.");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        ImGui::TextColored(colorTerminal, "LOCAL MODEL (Pre-downloaded):");
        ImGui::Spacing();
        ImGui::BulletText("TinyLlama 1.1B (Q4_K_M) - 653MB - Fast & Ready!");
        ImGui::Spacing();
        
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), 
                          "Open browser console (F12) and run:");
        ImGui::Text("  loadLocalModel()");
        ImGui::Spacing();
        if (ImGui::Button("Close", ImVec2(200, 0))) {
            showModelDialog = false;
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        ImGui::Text("Or upload your own GGUF file (in console):");
        ImGui::Text("  document.getElementById('model-upload').click()");
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        if (ImGui::Button("Cancel", ImVec2(200, 0))) {
            showModelDialog = false;
        }
        
        ImGui::EndPopup();
    }
}

