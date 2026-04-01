#pragma once

/// Mutable array section builder for writing ELF array sections
/// (.init_array, .fini_array, .preinit_array, etc.).

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>
#include <elfio/core/endian.hpp>

namespace elfio {

template <typename T>
class array_builder {
public:
    void add(T value) { entries_.push_back(value); }

    [[nodiscard]] std::size_t count() const noexcept { return entries_.size(); }

    [[nodiscard]] std::vector<char> build(const endian_convertor& conv) const {
        std::vector<char> out(entries_.size() * sizeof(T), '\0');
        for (std::size_t i = 0; i < entries_.size(); ++i) {
            T val = conv(entries_[i]);
            std::memcpy(out.data() + i * sizeof(T), &val, sizeof(T));
        }
        return out;
    }

    void clear() { entries_.clear(); }

private:
    std::vector<T> entries_;
};

} // namespace elfio
