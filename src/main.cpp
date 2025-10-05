#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include "chat.h"
#include "llm.h"
#include "ui.h"
#include <SDL2/SDL.h>
#include <SDL_opengles2.h>
#include <emscripten.h>
#include <emscripten/html5.h>

// Global state
struct AppState {
    SDL_Window* window;
    SDL_GLContext gl_context;
    ChatSession chatSession;
    LLM llm;
    UI* ui;
    bool running;
};

static AppState g_app;

// C functions to be called from JavaScript
extern "C" {
    EMSCRIPTEN_KEEPALIVE
    void showLoadingMessage() {
        g_app.chatSession.addMessage(MessageRole::ASSISTANT, 
            "Loading Qwen2.5-0.5B model... This may take a moment...");
    }
    
    EMSCRIPTEN_KEEPALIVE
    void loadModelFromFS() {
        printf("Loading model from filesystem...\n");
        
        // Remove the loading message
        auto& messages = const_cast<std::vector<Message>&>(g_app.chatSession.getMessages());
        if (!messages.empty() && messages.back().content.find("Loading Qwen") != std::string::npos) {
            messages.pop_back();
        }
        
        if (g_app.llm.loadModel("/models/model.gguf")) {
            g_app.chatSession.addMessage(MessageRole::ASSISTANT, 
                "Model loaded successfully! You can now chat with me.");
        } else {
            g_app.chatSession.addMessage(MessageRole::ASSISTANT, 
                "Failed to load model. Please check the console for errors.");
        }
    }
}

void main_loop() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT) {
            g_app.running = false;
        }
    }
    
    // Start ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    
    // Render UI (this processes input and may set pendingGeneration flag)
    g_app.ui->render();
    
    // Rendering
    ImGui::Render();
    SDL_GL_MakeCurrent(g_app.window, g_app.gl_context);
    glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(g_app.window);
    
    // AFTER rendering, handle pending generation (just queues it)
    bool justStarted = false;
    if (g_app.ui->processPendingGeneration()) {
        justStarted = true;
    }
    
    // Process one token per frame if generating (but not on the frame we just started)
    if (g_app.llm.isGenerating() && !justStarted) {
        g_app.llm.stepGeneration();
    }
}

int main(int argc, char** argv) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }
    
    // Create window with OpenGL ES 2.0 context
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    
    SDL_DisplayMode current;
    SDL_GetCurrentDisplayMode(0, &current);
    
    g_app.window = SDL_CreateWindow("Terminal Chatbot",
                                    SDL_WINDOWPOS_CENTERED,
                                    SDL_WINDOWPOS_CENTERED,
                                    1280, 720,
                                    SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    
    g_app.gl_context = SDL_GL_CreateContext(g_app.window);
    if (!g_app.gl_context) {
        printf("Failed to create GL context: %s\n", SDL_GetError());
        return -1;
    }
    
    SDL_GL_MakeCurrent(g_app.window, g_app.gl_context);
    
    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr; // Disable imgui.ini
    
    ImGui_ImplSDL2_InitForOpenGL(g_app.window, g_app.gl_context);
    ImGui_ImplOpenGL3_Init("#version 100");
    
    // Initialize app
    g_app.running = true;
    g_app.ui = new UI(g_app.chatSession, g_app.llm);
    g_app.ui->setup();
    
    // Add welcome message
    g_app.chatSession.addMessage(MessageRole::ASSISTANT, 
        "Welcome to Terminal Chatbot powered by llama.cpp! "
        "Click 'LOAD MODEL' button to start chatting with Qwen2.5-0.5B!");
    
    // Main loop - use 0 fps to sync with browser refresh rate (typically 60fps)
    emscripten_set_main_loop(main_loop, 0, true);
    
    // Set swap interval AFTER main loop is established
    SDL_GL_SetSwapInterval(1);
    
    // Note: Cleanup code below never executes with emscripten_set_main_loop
    // Emscripten handles cleanup automatically on page unload
    
    return 0;
}

