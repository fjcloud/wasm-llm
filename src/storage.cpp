#include "storage.h"
#include <emscripten.h>

void Storage::save(const std::string& key, const std::string& value) {
    EM_ASM({
        try {
            localStorage.setItem(UTF8ToString($0), UTF8ToString($1));
        } catch(e) {
            console.error("Failed to save to localStorage:", e);
        }
    }, key.c_str(), value.c_str());
}

std::string Storage::load(const std::string& key) {
    char* result = (char*)EM_ASM_PTR({
        try {
            var value = localStorage.getItem(UTF8ToString($0));
            if (value === null) return 0;
            var len = lengthBytesUTF8(value) + 1;
            var ptr = _malloc(len);
            stringToUTF8(value, ptr, len);
            return ptr;
        } catch(e) {
            console.error("Failed to load from localStorage:", e);
            return 0;
        }
    }, key.c_str());
    
    if (result) {
        std::string str(result);
        free(result);
        return str;
    }
    return "";
}

void Storage::remove(const std::string& key) {
    EM_ASM({
        try {
            localStorage.removeItem(UTF8ToString($0));
        } catch(e) {
            console.error("Failed to remove from localStorage:", e);
        }
    }, key.c_str());
}

bool Storage::hasKey(const std::string& key) {
    return EM_ASM_INT({
        try {
            return localStorage.getItem(UTF8ToString($0)) !== null ? 1 : 0;
        } catch(e) {
            console.error("Failed to check localStorage:", e);
            return 0;
        }
    }, key.c_str());
}

