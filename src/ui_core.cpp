#include "ui.h"
#include <cstring>
#include <emscripten.h>

UI::UI(ChatSession& chat, LLM& llm) 
    : chatSession(chat), llm(llm), showModelDialog(false), autoScroll(true), chatScrollY(0.0f),
      pendingGeneration(false), pendingResponseIndex(0) {
    memset(inputBuffer, 0, sizeof(inputBuffer));
    
    // Terminal green color scheme
    colorBackground = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    colorTerminal = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
    colorBorder = ImVec4(0.0f, 0.5f, 0.0f, 1.0f);
    colorUser = ImVec4(0.0f, 0.8f, 1.0f, 1.0f);
    colorAssistant = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
}

void UI::setup() {
    applyTerminalStyle();
}

void UI::applyTerminalStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    
    // Terminal colors
    style.Colors[ImGuiCol_Text] = colorTerminal;
    style.Colors[ImGuiCol_WindowBg] = colorBackground;
    style.Colors[ImGuiCol_Border] = colorBorder;
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.0f, 0.1f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.0f, 0.2f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.0f, 0.3f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.0f, 0.3f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.5f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.0f, 0.7f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.0f, 0.4f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.0f, 0.6f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.0f, 0.8f, 0.0f, 1.0f);
    
    // Rounding and spacing
    style.WindowRounding = 0.0f;
    style.FrameRounding = 0.0f;
    style.ScrollbarRounding = 0.0f;
    style.GrabRounding = 0.0f;
    style.WindowPadding = ImVec2(8.0f, 8.0f);
    style.FramePadding = ImVec2(4.0f, 3.0f);
    style.ItemSpacing = ImVec2(8.0f, 4.0f);
    style.WindowBorderSize = 0.0f;
    style.ScrollbarSize = 12.0f;
}

void UI::render() {
    ImGuiIO& io = ImGui::GetIO();
    
    // Full screen window
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | 
                             ImGuiWindowFlags_NoResize | 
                             ImGuiWindowFlags_NoMove | 
                             ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoBringToFrontOnFocus |
                             ImGuiWindowFlags_NoScrollbar |
                             ImGuiWindowFlags_NoScrollWithMouse;
    
    ImGui::Begin("Terminal Chat", nullptr, flags);
    
    renderHeader();
    ImGui::Separator();
    
    renderChatView();
    
    ImGui::Separator();
    renderFooter();
    
    ImGui::End();
    
    if (showModelDialog) {
        renderModelDialog();
    }
}

bool UI::processPendingGeneration() {
    if (pendingGeneration) {
        pendingGeneration = false;
        
        // Start generation with the stored prompt and callback
        // This just queues it, actual processing happens on next frame
        llm.startGeneration(pendingPrompt, [this](const std::string& token) {
            // Update the last message with streaming tokens in real-time
            auto& messages = const_cast<std::vector<Message>&>(chatSession.getMessages());
            if (pendingResponseIndex < messages.size()) {
                messages[pendingResponseIndex].content += token;
                autoScroll = true;  // Keep scrolling as tokens arrive
            }
        });
        
        return true; // Generation was just started
    }
    
    return false; // No generation started
}

void UI::renderHeader() {
    ImGui::Text("TERMINAL CHATBOT - WEBGPU + LLAMA.CPP");
    ImGui::SameLine(ImGui::GetWindowWidth() - 200);
    
    if (ImGui::Button("LOAD MODEL")) {
        EM_ASM({
            if (typeof window.loadLocalModel === 'function') {
                window.loadLocalModel();
            } else {
                console.error('loadLocalModel() not found');
            }
        });
    }
    
    ImGui::SameLine();
    if (ImGui::Button("CLEAR CHAT")) {
        chatSession.clearMessages();
    }
}

void UI::renderFooter() {
    ImGui::Text("Model: %s", llm.getModelInfo().c_str());
    
    ImGui::SameLine();
    if (llm.isLoaded()) {
        if (llm.isUsingGPU()) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), " [WebGPU]");
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), " [CPU 4-threads]");
        }
    }
    
    ImGui::SameLine(ImGui::GetWindowWidth() - 300);
    ImGui::Text("Messages: %zu", chatSession.getMessages().size());
}

