#pragma once

/// Zero-copy array section view (e.g. .init_array, .fini_array).
/// Reads an array of T elements with endianness conversion.

#include <cstddef>
#include <cstdint>
#include <elfio/core/byte_view.hpp>
#include <elfio/core/endian.hpp>

namespace elfio {

template <typename T>
class array_ref {
    byte_view               data_;
    std::size_t             offset_;
    const endian_convertor* conv_;

public:
    array_ref(byte_view data, std::size_t offset,
              const endian_convertor* conv) noexcept
        : data_(data), offset_(offset), conv_(conv) {}

    [[nodiscard]] T value() const noexcept {
        auto r = data_.read<T>(offset_);
        return r ? (conv_ ? (*conv_)(*r) : *r) : T{};
    }

    [[nodiscard]] explicit operator T() const noexcept { return value(); }
};

template <typename T>
class array_range {
    byte_view               data_;
    std::size_t             count_;
    const endian_convertor* conv_;

public:
    class iterator {
        byte_view               data_;
        std::size_t             offset_;
        std::size_t             index_;
        const endian_convertor* conv_;
    public:
        using difference_type   = std::ptrdiff_t;
        using value_type        = array_ref<T>;
        using pointer           = void;
        using reference         = value_type;
        using iterator_category = std::input_iterator_tag;

        iterator(byte_view d, std::size_t off, std::size_t idx,
                 const endian_convertor* c) noexcept
            : data_(d), offset_(off), index_(idx), conv_(c) {}

        value_type operator*() const { return {data_, offset_, conv_}; }
        iterator& operator++() { offset_ += sizeof(T); ++index_; return *this; }
        iterator operator++(int) { auto t = *this; ++(*this); return t; }
        bool operator==(const iterator& o) const noexcept { return index_ == o.index_; }
        bool operator!=(const iterator& o) const noexcept { return index_ != o.index_; }
    };

    array_range(byte_view data, const endian_convertor* conv) noexcept
        : data_(data), count_(data.size() / sizeof(T)), conv_(conv) {}

    [[nodiscard]] iterator    begin() const { return {data_, 0, 0, conv_}; }
    [[nodiscard]] iterator    end()   const { return {data_, 0, count_, conv_}; }
    [[nodiscard]] std::size_t size()  const noexcept { return count_; }
    [[nodiscard]] bool        empty() const noexcept { return count_ == 0; }

    [[nodiscard]] array_ref<T> operator[](std::size_t i) const noexcept {
        return {data_, i * sizeof(T), conv_};
    }
};

} // namespace elfio
