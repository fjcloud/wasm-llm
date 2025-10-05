FROM emscripten/emsdk:4.0.15-arm64

# Install git for cloning dependencies
RUN apt-get update && apt-get install -y git cmake && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Clone Dear ImGui
RUN git clone --depth 1 --branch v1.91.5 https://github.com/ocornut/imgui.git

# Clone llama.cpp
RUN git clone https://github.com/ggerganov/llama.cpp.git && \
    cd llama.cpp && \
    git checkout master

# Copy source files
COPY src/ /app/src/
COPY web/shell.html /app/shell.html

# Build script
RUN echo '#!/bin/bash\n\
set -e\n\
\n\
echo "Building llama.cpp for WASM using CMake..."\n\
cd /app/llama.cpp\n\
\n\
# Create build directory\n\
mkdir -p build-wasm\n\
cd build-wasm\n\
\n\
# Configure with CMake (Multi-threaded CPU)\n\
emcmake cmake .. \\\n\
    -DCMAKE_BUILD_TYPE=Release \\\n\
    -DCMAKE_C_FLAGS="-pthread" \\\n\
    -DCMAKE_CXX_FLAGS="-pthread" \\\n\
    -DGGML_BACKEND_DL=OFF \\\n\
    -DGGML_METAL=OFF \\\n\
    -DGGML_CUDA=OFF \\\n\
    -DGGML_VULKAN=OFF \\\n\
    -DGGML_KOMPUTE=OFF \\\n\
    -DGGML_RPC=OFF \\\n\
    -DGGML_SYCL=OFF \\\n\
    -DBUILD_SHARED_LIBS=OFF \\\n\
    -DLLAMA_CURL=OFF \\\n\
    -DLLAMA_BUILD_TESTS=OFF \\\n\
    -DLLAMA_BUILD_EXAMPLES=OFF \\\n\
    -DLLAMA_BUILD_SERVER=OFF\n\
\n\
# Build llama.cpp\n\
emmake make -j4 llama ggml\n\
\n\
echo "llama.cpp library built successfully"\n\
cd /app\n\
\n\
echo "Compiling ImGui sources..."\n\
emcc -c imgui/imgui.cpp -o imgui.o -Iimgui -O3 -pthread\n\
emcc -c imgui/imgui_demo.cpp -o imgui_demo.o -Iimgui -O3 -pthread\n\
emcc -c imgui/imgui_draw.cpp -o imgui_draw.o -Iimgui -O3 -pthread\n\
emcc -c imgui/imgui_tables.cpp -o imgui_tables.o -Iimgui -O3 -pthread\n\
emcc -c imgui/imgui_widgets.cpp -o imgui_widgets.o -Iimgui -O3 -pthread\n\
emcc -c imgui/backends/imgui_impl_sdl2.cpp -o imgui_impl_sdl2.o -Iimgui -s USE_SDL=2 -O3 -pthread\n\
emcc -c imgui/backends/imgui_impl_opengl3.cpp -o imgui_impl_opengl3.o -Iimgui -s USE_SDL=2 -O3 -pthread\n\
\n\
echo "Compiling application modules..."\n\
emcc -c src/message.cpp -o message.o -Isrc -Iimgui -s USE_SDL=2 -O3 -pthread\n\
emcc -c src/chat.cpp -o chat.o -Isrc -Iimgui -s USE_SDL=2 -O3 -pthread\n\
emcc -c src/storage.cpp -o storage.o -Isrc -Iimgui -s USE_SDL=2 -O3 -pthread\n\
emcc -c src/llm.cpp -o llm.o \\\n\
    -Isrc -Iimgui \\\n\
    -I/app/llama.cpp/include \\\n\
    -I/app/llama.cpp/ggml/include \\\n\
    -I/app/llama.cpp/build-wasm/ggml/include \\\n\
    -I/app/llama.cpp/src \\\n\
    -s USE_SDL=2 -O3 -std=c++17 -pthread\n\
emcc -c src/ui_core.cpp -o ui_core.o -Isrc -Iimgui -Iimgui/backends -s USE_SDL=2 -O3 -pthread\n\
emcc -c src/ui_chat.cpp -o ui_chat.o -Isrc -Iimgui -Iimgui/backends -s USE_SDL=2 -O3 -pthread\n\
emcc -c src/main.cpp -o main.o -Isrc -Iimgui -Iimgui/backends -s USE_SDL=2 -O3 -pthread\n\
\n\
echo "Linking everything..."\n\
emcc -o /app/dist/index.html \\\n\
    main.o message.o chat.o storage.o llm.o \\\n\
    ui_core.o ui_chat.o \\\n\
    imgui.o imgui_demo.o imgui_draw.o imgui_tables.o imgui_widgets.o \\\n\
    imgui_impl_sdl2.o imgui_impl_opengl3.o \\\n\
    /app/llama.cpp/build-wasm/src/libllama.a \\\n\
    /app/llama.cpp/build-wasm/ggml/src/libggml.a \\\n\
    /app/llama.cpp/build-wasm/ggml/src/libggml-base.a \\\n\
    /app/llama.cpp/build-wasm/ggml/src/libggml-cpu.a \\\n\
    -pthread \\\n\
    -s USE_SDL=2 \\\n\
    -s USE_WEBGL2=1 \\\n\
    -s USE_WEBGPU=1 \\\n\
    -s FULL_ES3=1 \\\n\
    -s WASM=1 \\\n\
    -s ALLOW_MEMORY_GROWTH=1 \\\n\
    -s INITIAL_MEMORY=512MB \\\n\
    -s MAXIMUM_MEMORY=2GB \\\n\
    -s NO_EXIT_RUNTIME=1 \\\n\
    -s ASSERTIONS=1 \\\n\
    -s PTHREAD_POOL_SIZE=4 \\\n\
    -s ASYNCIFY \\\n\
    -s ASYNCIFY_STACK_SIZE=24576 \\\n\
    -s EXPORTED_FUNCTIONS="[\"_main\",\"_malloc\",\"_free\",\"_loadModelFromFS\",\"_showLoadingMessage\"]" \\\n\
    -s EXPORTED_RUNTIME_METHODS="[\"FS\",\"ccall\",\"cwrap\"]" \\\n\
    -s FORCE_FILESYSTEM=1 \\\n\
    -s FETCH=1 \\\n\
    --shell-file /app/shell.html \\\n\
    -O3\n\
\n\
echo "Build complete! Output files in /app/dist/"\n\
ls -lh /app/dist/\n\
' > /app/build.sh && chmod +x /app/build.sh

# Create dist and models directory
RUN mkdir -p /app/dist /app/models

# Run the build
RUN /app/build.sh

WORKDIR /app/dist
