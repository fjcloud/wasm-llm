.PHONY: build clean serve

IMAGE_NAME = wasm-chatbot-builder

build:
	@echo "Building WASM Chatbot..."
	@mkdir -p docs
	podman build -t $(IMAGE_NAME):latest .
	podman run --rm -v $(PWD)/docs:/output:Z $(IMAGE_NAME):latest sh -c "cp -r /app/dist/* /output/"
	@echo ""
	@echo "✓ Build complete! Files in docs/"
	@echo ""
	@echo "To serve: make serve"
	@echo "Or: cd docs && python3 -m http.server 8000"

clean:
	@echo "Cleaning up..."
	rm -f docs/index.html docs/index.js docs/index.wasm
	-podman rmi $(IMAGE_NAME):latest 2>/dev/null || true
	@echo "Cleanup complete (preserved CNAME and coi-serviceworker.min.js)."

serve:
	@cd docs && python3 ../serve.py

