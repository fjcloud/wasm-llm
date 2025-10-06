#include "llm.h"
#include "llama.h"
#include <cstring>
#include <cstdio>
#include <emscripten.h>

// Log to console.info instead of console.error
#define LOG_INFO(...) do { \
    char buf[512]; \
    snprintf(buf, sizeof(buf), __VA_ARGS__); \
    EM_ASM({ console.info(UTF8ToString($0)); }, buf); \
} while(0)

LLM::LLM() : model(nullptr), ctx(nullptr), sampler(nullptr), loaded(false), generating(false), 
             usingGPU(false), modelInfo("No model loaded"), tokensGenerated(0), maxTokens(512),
             promptProcessed(false) {
    // Initialize llama backend
    llama_backend_init();
    llama_numa_init(GGML_NUMA_STRATEGY_DISABLED);
}

LLM::~LLM() {
    unloadModel();
    llama_backend_free();
}

bool LLM::isLoaded() const {
    return loaded;
}

bool LLM::loadModel(const std::string& modelPath) {
    if (loaded) {
        unloadModel();
    }
    
    LOG_INFO("Loading model from: %s", modelPath.c_str());
    
    // Model parameters
    llama_model_params model_params = llama_model_default_params();
    model_params.n_gpu_layers = 99; // Try to offload all layers to GPU (experimental!)
    model_params.use_mmap = false; // Don't use mmap in browser
    model_params.use_mlock = false;
    
    // Load model
    model = llama_load_model_from_file(modelPath.c_str(), model_params);
    if (!model) {
        printf("Failed to load model\n");
        return false;
    }
    
    // Context parameters
    llama_context_params ctx_params = llama_context_default_params();
    ctx_params.n_ctx = 2048; // Context size
    ctx_params.n_batch = 512; // Batch size
    ctx_params.n_threads = 4; // Multi-threaded with pthread support!
    ctx_params.n_threads_batch = 4; // Multi-threaded for batch processing
    
    // Create context
    ctx = llama_new_context_with_model(model, ctx_params);
    if (!ctx) {
        printf("Failed to create context\n");
        llama_free_model(model);
        model = nullptr;
        return false;
    }
    
    // Create sampler with better settings for chat
    sampler = llama_sampler_chain_init(llama_sampler_chain_default_params());
    llama_sampler_chain_add(sampler, llama_sampler_init_temp(0.8f));
    llama_sampler_chain_add(sampler, llama_sampler_init_top_k(40));
    llama_sampler_chain_add(sampler, llama_sampler_init_top_p(0.95f, 1));
    llama_sampler_chain_add(sampler, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));
    
    loaded = true;
    
    // Detect if GPU is being used
    // Check if any layers were offloaded (experimental WebGPU detection)
    usingGPU = (model_params.n_gpu_layers > 0);
    
    // Get model info
    char buf[256];
    snprintf(buf, sizeof(buf), "llama.cpp model (ctx: %d, device: %s)", 
             ctx_params.n_ctx, usingGPU ? "WebGPU (experimental)" : "CPU");
    modelInfo = buf;
    
    LOG_INFO("Model loaded successfully - Running on: %s", usingGPU ? "GPU (experimental)" : "CPU");
    return true;
}

void LLM::unloadModel() {
    if (!loaded) return;
    
    if (sampler) {
        llama_sampler_free(sampler);
        sampler = nullptr;
    }
    
    if (ctx) {
        llama_free(ctx);
        ctx = nullptr;
    }
    
    if (model) {
        llama_free_model(model);
        model = nullptr;
    }
    
    loaded = false;
    modelInfo = "No model loaded";
    LOG_INFO("Model unloaded");
}

std::vector<int> LLM::tokenize(const std::string& text, bool add_special) {
    if (!model) return {};
    
    const llama_vocab* vocab = llama_model_get_vocab(model);
    
    int n_tokens = text.length() + (add_special ? 2 : 0);
    std::vector<int> tokens(n_tokens);
    
    n_tokens = llama_tokenize(vocab, text.c_str(), text.length(), 
                               tokens.data(), tokens.size(), 
                               add_special, false);
    
    if (n_tokens < 0) {
        tokens.resize(-n_tokens);
        n_tokens = llama_tokenize(vocab, text.c_str(), text.length(), 
                                   tokens.data(), tokens.size(), 
                                   add_special, false);
    }
    
    tokens.resize(n_tokens);
    return tokens;
}

std::string LLM::detokenize(int token) {
    if (!model) return "";
    
    const llama_vocab* vocab = llama_model_get_vocab(model);
    
    char buf[128];
    int n = llama_token_to_piece(vocab, token, buf, sizeof(buf), 0, false);
    
    if (n > 0) {
        return std::string(buf, n);
    }
    return "";
}

// Start generation (just setup, no processing yet)
void LLM::startGeneration(const std::string& prompt, std::function<void(const std::string&)> onToken) {
    if (!loaded || generating) {
        printf("Cannot generate: loaded=%d, generating=%d\n", loaded, generating);
        return;
    }
    
    generating = true;
    promptProcessed = false;
    pendingPrompt = prompt;
    onTokenCallback = onToken;
    currentResponse = "";
    tokensGenerated = 0;
    
    LOG_INFO("Generation queued for prompt: %s", prompt.substr(0, 50).c_str());
}

// Generate one token per call (call this in main loop)
bool LLM::stepGeneration() {
    if (!generating || !loaded) {
        return false;
    }
    
    // First call: process the prompt (tokenize + decode)
    if (!promptProcessed) {
        LOG_INFO("Processing prompt...");
        
        // Tokenize prompt
        std::vector<int> tokens = tokenize(pendingPrompt, true);
        
        if (tokens.empty()) {
            printf("Failed to tokenize prompt\n");
            generating = false;
            return false;
        }
        
        LOG_INFO("Tokenized prompt: %zu tokens", tokens.size());
        
        // Create batch
        llama_batch batch = llama_batch_get_one(tokens.data(), tokens.size());
        
        // Process prompt
        if (llama_decode(ctx, batch) != 0) {
            printf("Failed to decode prompt\n");
            generating = false;
            return false;
        }
        
        promptProcessed = true;
        LOG_INFO("Prompt processed, ready to generate tokens");
        return true; // Continue next frame
    }
    
    // Check if we've generated enough tokens
    if (tokensGenerated >= maxTokens) {
        generating = false;
        LOG_INFO("Max tokens reached");
        return false;
    }
    
    // Sample next token
    int new_token_id = llama_sampler_sample(sampler, ctx, -1);
    
    // Check for EOS token
    const llama_vocab* vocab = llama_model_get_vocab(model);
    if (llama_vocab_is_eog(vocab, new_token_id)) {
        LOG_INFO("EOS token detected, stopping generation");
        generating = false;
        return false;
    }
    
    // Detokenize
    std::string piece = detokenize(new_token_id);
    
    // Skip empty pieces
    if (piece.empty()) {
        return true; // Continue generating
    }
    
    // Filter out any Qwen2.5 special tokens or fragments
    if (piece.find("<|") != std::string::npos ||
        piece.find("|>") != std::string::npos ||
        piece.find("im_end") != std::string::npos ||
        piece.find("im_start") != std::string::npos ||
        piece.find("endoftext") != std::string::npos) {
        LOG_INFO("Stop token/fragment detected: %s", piece.c_str());
        generating = false;
        return false;
    }
    
    // Check for gibberish or repeated punctuation
    bool hasValidContent = false;
    for (char c : piece) {
        if (std::isalnum(c) || std::isspace(c)) {
            hasValidContent = true;
            break;
        }
    }
    
    // If piece is only punctuation/symbols and longer than 1 char, skip it
    if (!hasValidContent && piece.length() > 1) {
        LOG_INFO("Skipping gibberish/repeated symbols: %s", piece.substr(0, 10).c_str());
        // Prepare for next iteration (don't add to response, but continue generating)
        llama_batch batch = llama_batch_get_one(&new_token_id, 1);
        if (llama_decode(ctx, batch) != 0) {
            printf("Failed to decode token\n");
            generating = false;
            return false;
        }
        return true; // Continue generating, just skip this token
    }
    
    // Add to response
    currentResponse += piece;
    tokensGenerated++;
    
    // Send token to UI immediately
    if (onTokenCallback) {
        onTokenCallback(piece);
    }
    
    // Prepare for next iteration
    llama_batch batch = llama_batch_get_one(&new_token_id, 1);
    
    // Decode
    if (llama_decode(ctx, batch) != 0) {
        printf("Failed to decode token\n");
        generating = false;
        return false;
    }
    
    return true; // Continue generating
}

void LLM::stopGeneration() {
    generating = false;
}

bool LLM::isGenerating() const {
    return generating;
}

std::string LLM::getModelInfo() const {
    return modelInfo;
}

bool LLM::isUsingGPU() const {
    return usingGPU;
}
