#pragma once

/// Zero-copy string table view.
/// ELF string tables are NUL-terminated strings addressed by byte offset.

#include <cstddef>
#include <string_view>
#include <elfio/core/byte_view.hpp>

namespace elfio {

class string_table_view {
    byte_view data_;

public:
    constexpr string_table_view() noexcept = default;
    explicit string_table_view(byte_view data) noexcept : data_(data) {}

    /// Look up a string at the given byte offset within the table.
    [[nodiscard]] std::string_view get(std::size_t offset) const noexcept {
        if (offset >= data_.size()) return {};
        return data_.read_cstring(offset);
    }

    [[nodiscard]] byte_view        raw()   const noexcept { return data_; }
    [[nodiscard]] std::size_t      size()  const noexcept { return data_.size(); }
    [[nodiscard]] bool             empty() const noexcept { return data_.empty(); }
};

} // namespace elfio
