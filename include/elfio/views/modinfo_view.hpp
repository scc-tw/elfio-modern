#pragma once

/// Zero-copy module info view for .modinfo sections (kernel module metadata).
/// Parses null-terminated "key=value" pairs.

#include <cstddef>
#include <string_view>
#include <elfio/core/byte_view.hpp>

namespace elfio {

class modinfo_ref {
    std::string_view key_;
    std::string_view value_;

public:
    modinfo_ref(std::string_view key, std::string_view value) noexcept
        : key_(key), value_(value) {}

    [[nodiscard]] std::string_view key()   const noexcept { return key_; }
    [[nodiscard]] std::string_view value() const noexcept { return value_; }
};

/// Iterate over key=value pairs in a .modinfo section.
class modinfo_range {
    byte_view data_;

public:
    class iterator {
        byte_view   data_;
        std::size_t offset_;

        void skip_to_valid() {
            // Skip past any NUL padding
            while (offset_ < data_.size() &&
                   static_cast<char>(data_[offset_]) == '\0')
                ++offset_;
        }

    public:
        using difference_type   = std::ptrdiff_t;
        using value_type        = modinfo_ref;
        using pointer           = void;
        using reference         = value_type;
        using iterator_category = std::input_iterator_tag;

        iterator(byte_view d, std::size_t off) noexcept
            : data_(d), offset_(off) { skip_to_valid(); }

        value_type operator*() const {
            auto sv = data_.read_cstring(offset_);
            auto eq = sv.find('=');
            if (eq == std::string_view::npos)
                return {sv, {}};
            return {sv.substr(0, eq), sv.substr(eq + 1)};
        }
        iterator& operator++() {
            auto sv = data_.read_cstring(offset_);
            offset_ += sv.size() + 1;  // skip past NUL
            skip_to_valid();
            return *this;
        }
        iterator operator++(int) { auto t = *this; ++(*this); return t; }
        bool operator==(const iterator& o) const noexcept {
            bool this_end = offset_ >= data_.size();
            bool o_end    = o.offset_ >= o.data_.size();
            if (this_end && o_end) return true;
            return offset_ == o.offset_;
        }
        bool operator!=(const iterator& o) const noexcept { return !(*this == o); }
    };

    explicit modinfo_range(byte_view data) noexcept : data_(data) {}

    [[nodiscard]] iterator begin() const { return {data_, 0}; }
    [[nodiscard]] iterator end()   const { return {data_, data_.size()}; }
    [[nodiscard]] bool     empty() const noexcept { return data_.empty(); }
};

} // namespace elfio
