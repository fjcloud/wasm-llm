#pragma once
#include <string>

class Storage {
public:
    static void save(const std::string& key, const std::string& value);
    static std::string load(const std::string& key);
    static void remove(const std::string& key);
    static bool hasKey(const std::string& key);
};

