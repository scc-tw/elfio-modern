#pragma once

/// Mutable module info builder for writing .modinfo sections.
/// Produces NUL-terminated "key=value" pairs.

#include <cstddef>
#include <string>
#include <vector>

namespace elfio {

class modinfo_builder {
public:
    void add(std::string key, std::string value) {
        entries_.push_back({std::move(key), std::move(value)});
    }

    [[nodiscard]] std::size_t count() const noexcept { return entries_.size(); }

    [[nodiscard]] std::vector<char> build() const {
        std::vector<char> out;
        for (const auto& e : entries_) {
            // "key=value\0"
            out.insert(out.end(), e.first.begin(), e.first.end());
            out.push_back('=');
            out.insert(out.end(), e.second.begin(), e.second.end());
            out.push_back('\0');
        }
        return out;
    }

    void clear() { entries_.clear(); }

private:
    std::vector<std::pair<std::string, std::string>> entries_;
};

} // namespace elfio
