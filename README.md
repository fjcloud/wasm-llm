# Terminal Chatbot - WASM + WebGPU + Qwen2.5

A retro terminal-style chatbot powered by WebAssembly with llama.cpp and Qwen2.5-0.5B-Instruct. Features a classic phosphor green aesthetic with full browser-based LLM inference capabilities.

## Features

* **Terminal UI**: Classic green phosphor terminal aesthetic
* **WebAssembly**: Runs entirely in browser, no backend needed
* **Modular Architecture**: Clean separation of concerns (chat, storage, UI, LLM)
* **Chat Management**: Full message history with role-based coloring
* **Auto-save**: Messages persist in browser localStorage
* **WebGPU Ready**: Designed for llama.cpp WebGPU integration
* **Responsive Design**: Adapts to any window size
* **Real-time Streaming**: Token-by-token response generation (when integrated)

## Architecture

Following the design pattern from [wasm-calendar](https://github.com/fjcloud/wasm-calendar):

```
src/
├── main.cpp         # Entry point & SDL/ImGui setup
├── message.*        # Message data structures
├── chat.*           # Chat session management
├── storage.*        # localStorage persistence
├── llm.*            # LLM interface (placeholder for llama.cpp)
├── ui.h             # UI interface
├── ui_core.cpp      # Main rendering & terminal styling
└── ui_chat.cpp      # Chat view & input handling
```

**Modular Design**: 
- **message.cpp/h** - Message data structures with roles (USER, ASSISTANT, SYSTEM)
- **chat.cpp/h** - Chat session management and prompt building
- **storage.cpp/h** - localStorage persistence layer
- **llm.cpp/h** - LLM interface with placeholder for llama.cpp integration
- **ui_core.cpp** - Main UI rendering, terminal styling, header/footer
- **ui_chat.cpp** - Chat message list, input area, model dialog

## Quick Start

### Prerequisites

- Podman (or Docker)
- Python 3 (for local server)
- Make

### Build

```bash
# Build WASM using containerized Emscripten
make build
```

Outputs WASM files to `docs/` directory.

The build process:
1. Builds a container with Emscripten 4.0.15-arm64
2. Clones Dear ImGui v1.91.5
3. Compiles all modules individually
4. Links everything into WebAssembly
5. Copies output to `docs/`

### Download Model

Download the quantized Qwen2.5-0.5B model:

```bash
# From Hugging Face (requires git-lfs)
cd docs
wget https://huggingface.co/Qwen/Qwen2.5-0.5B-Instruct-GGUF/resolve/main/qwen2.5-0.5b-instruct-q4_k_m.gguf

# Or download manually and place in docs/
```

**Model**: [Qwen2.5-0.5B-Instruct](https://huggingface.co/Qwen/Qwen2.5-0.5B-Instruct) by Alibaba Cloud  
**Format**: GGUF (Q4_K_M quantization)  
**Size**: ~330MB (0.5B parameters)  
**Speed**: Very fast inference on CPU/WebGPU

### Serve

```bash
make serve
# Opens http://localhost:8000
```

**Note**: Must use `make serve` (not plain `python -m http.server`) because it adds required COOP/COEP headers for multi-threading support.

## Usage

1. **Start the Server**: Run `make serve` 
2. **Open Browser**: Navigate to http://localhost:8000
3. **Load Model**: Click "LOAD MODEL" button (loads Qwen2.5-0.5B automatically)
4. **Start Chatting**: Type your message and press SEND or Enter
5. **Clear Chat**: Click CLEAR CHAT button to reset conversation

### Keyboard Shortcuts

- `Enter` - Send message
- `Shift+Enter` - New line in input

## Integrating llama.cpp

This is a placeholder implementation. To enable real LLM inference:

### 1. Add llama.cpp WASM

```bash
# Clone llama.cpp with WebGPU support
git clone https://github.com/ggerganov/llama.cpp.git
cd llama.cpp
# Build WASM with WebGPU
# Follow llama.cpp WASM build instructions
```

### 2. Update llm.cpp

Replace placeholder methods in `src/llm.cpp`:

```cpp
bool LLM::loadModel(const std::string& modelPath) {
    // Call llama.cpp's model loading API
    // Return success status
}

void LLM::generate(const std::string& prompt, 
                   std::function<void(const std::string&)> onToken) {
    // Call llama.cpp's generation API
    // Stream tokens via onToken callback
}
```

### 3. Update Makefile

Add llama.cpp libraries and link flags:

```makefile
CXXFLAGS += -I/path/to/llama.cpp/include
LDFLAGS += -L/path/to/llama.cpp/lib -llama
```

### 4. Provide Model Files

Host GGUF model files and load them via:
- IndexedDB for local storage
- Fetch API for remote URLs
- File picker for user uploads

## Development

### Project Structure

```
wasm-llm/
├── src/             # C++ source files
├── web/             # HTML shell template
├── docs/            # Build output (WASM files)
├── Makefile         # Build configuration
└── README.md        # This file
```

### Making Changes

```bash
# Edit source files in src/
vim src/ui_chat.cpp

# Rebuild (clean + build recommended for source changes)
make clean && make build

# Test
make serve
```

### Clean Build

```bash
# Remove all build artifacts and container image
make clean
```

### Adding Features

The modular architecture makes it easy to extend:

- **New UI views**: Add to `ui_*.cpp` files
- **Chat features**: Extend `chat.cpp`
- **Storage backends**: Modify `storage.cpp`
- **Model management**: Enhance `llm.cpp`

## Tech Stack

* **C++17** - Modern C++ with modular architecture
* **Dear ImGui** - Immediate mode GUI library
* **SDL2** - Window management and input
* **Emscripten** - WebAssembly compilation
* **OpenGL ES 2.0** - Graphics rendering
* **llama.cpp** - LLM inference (integration needed)

## Browser Support

Modern browsers with WebAssembly and WebGPU:

* Chrome/Edge 113+ (WebGPU stable)
* Firefox 121+ (WebGPU enabled in about:config)
* Safari 18+ (WebGPU preview)

## Performance Tips

1. **Model Size**: Use quantized GGUF models (Q4_K_M recommended)
2. **Context Length**: Limit context to 2048 tokens for browser
3. **Batch Size**: Adjust based on available GPU memory
4. **Caching**: Enable KV cache in llama.cpp for faster generation

## Troubleshooting

### Build Errors

- Ensure Podman is installed: `podman --version`
- Check container build logs for Emscripten errors
- Verify source files are in `src/` directory
- Try clean build: `make clean && make build`

### Runtime Errors

- Check browser console for WebGL/WebGPU errors
- Verify WASM files are served with correct MIME types
- Ensure sufficient memory for model loading

### Model Loading Issues

- Verify GGUF file format compatibility
- Check CORS headers if loading from remote URL
- Monitor browser memory usage during load

## Roadmap

- [ ] Real llama.cpp WebGPU integration
- [ ] Model file picker with drag-and-drop
- [ ] Streaming token animation
- [ ] Temperature and sampling controls
- [ ] Export chat history
- [ ] Multiple chat sessions
- [ ] Custom system prompts UI
- [ ] Mobile responsive layout
- [ ] Syntax highlighting for code blocks

## License

MIT

## Credits

* [Dear ImGui](https://github.com/ocornut/imgui)
* [Emscripten](https://emscripten.org/)
* [SDL2](https://www.libsdl.org/)
* [llama.cpp](https://github.com/ggerganov/llama.cpp)
* Inspired by [wasm-calendar](https://github.com/fjcloud/wasm-calendar)

## Contributing

Contributions welcome! Please feel free to submit a Pull Request.

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

