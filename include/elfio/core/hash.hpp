#pragma once

/// Standard ELF and GNU hash functions.

#include <cstdint>
#include <string_view>

namespace elfio {

/// Standard ELF hash (SysV).
[[nodiscard]] inline uint32_t elf_hash(std::string_view name) noexcept {
    uint32_t h = 0;
    for (char ch : name) {
        auto c = static_cast<uint32_t>(static_cast<unsigned char>(ch));
        h = (h << 4) + c;
        uint32_t g = h & 0xf0000000;
        if (g != 0) h ^= g >> 24;
        h &= ~g;
    }
    return h;
}

/// GNU hash (used by .gnu.hash sections).
[[nodiscard]] inline uint32_t elf_gnu_hash(std::string_view name) noexcept {
    uint32_t h = 5381;
    for (char ch : name) {
        h = h * 33 + static_cast<uint32_t>(static_cast<unsigned char>(ch));
    }
    return h;
}

} // namespace elfio
