#pragma once
#include <string>
#include <functional>
#include <vector>

// Forward declarations for llama.cpp types
struct llama_model;
struct llama_context;
struct llama_sampler;

class LLM {
public:
    LLM();
    ~LLM();
    
    bool isLoaded() const;
    bool loadModel(const std::string& modelPath);
    void unloadModel();
    
    // Start generation (non-blocking setup)
    void startGeneration(const std::string& prompt, std::function<void(const std::string&)> onToken);
    
    // Process next token (call this in main loop)
    bool stepGeneration();
    
    void stopGeneration();
    
    bool isGenerating() const;
    std::string getModelInfo() const;
    bool isUsingGPU() const;
    
private:
    llama_model* model;
    llama_context* ctx;
    llama_sampler* sampler;
    
    bool loaded;
    bool generating;
    bool usingGPU;
    std::string modelInfo;
    
    // Generation state
    std::function<void(const std::string&)> onTokenCallback;
    std::string currentResponse;
    std::string pendingPrompt;
    int tokensGenerated;
    int maxTokens;
    bool promptProcessed;
    
    std::vector<int> tokenize(const std::string& text, bool add_special);
    std::string detokenize(int token);
};

