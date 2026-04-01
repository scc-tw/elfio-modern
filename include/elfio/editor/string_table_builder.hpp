#pragma once

/// Mutable string table builder for writing ELF files.
/// Accumulates strings and produces a packed NUL-terminated byte blob.

#include <cstddef>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>
#include <elfio/core/result.hpp>

namespace elfio {

class string_table_builder {
    std::vector<char> data_;

public:
    string_table_builder() {
        // ELF string tables start with a NUL byte (index 0 = empty string)
        data_.push_back('\0');
    }

    /// Add a string, return its offset in the table.
    [[nodiscard]] Elf_Word add(std::string_view str) {
        // Check if string already exists (simple linear search)
        if (str.empty()) return 0;

        // Simple dedup: search for exact match
        for (std::size_t i = 1; i < data_.size(); ) {
            std::string_view existing(&data_[i]);
            if (existing == str) return static_cast<Elf_Word>(i);
            i += existing.size() + 1;
        }

        auto offset = static_cast<Elf_Word>(data_.size());
        data_.insert(data_.end(), str.begin(), str.end());
        data_.push_back('\0');
        return offset;
    }

    [[nodiscard]] const std::vector<char>& data() const noexcept { return data_; }
    [[nodiscard]] std::size_t size() const noexcept { return data_.size(); }
    [[nodiscard]] bool empty() const noexcept { return data_.size() <= 1; }

    void clear() {
        data_.clear();
        data_.push_back('\0');
    }
};

} // namespace elfio
