#pragma once

/// Mutable version symbol builder for writing .gnu.version sections.

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>
#include <elfio/core/endian.hpp>
#include <elfio/platform/schema.hpp>

namespace elfio {

class versym_builder {
public:
    void add(Elf_Half version) { entries_.push_back(version); }

    [[nodiscard]] std::size_t count() const noexcept { return entries_.size(); }

    [[nodiscard]] std::vector<char> build(const endian_convertor& conv) const {
        std::vector<char> out(entries_.size() * sizeof(Elf_Half), '\0');
        for (std::size_t i = 0; i < entries_.size(); ++i) {
            Elf_Half val = conv(entries_[i]);
            std::memcpy(out.data() + i * sizeof(Elf_Half), &val, sizeof(Elf_Half));
        }
        return out;
    }

    void clear() { entries_.clear(); }

private:
    std::vector<Elf_Half> entries_;
};

} // namespace elfio
