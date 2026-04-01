#pragma once

/// Zero-copy relocation views for both REL and RELA entries.

#include <cstddef>
#include <cstdint>
#include <elfio/core/byte_view.hpp>
#include <elfio/core/endian.hpp>
#include <elfio/platform/schema.hpp>

namespace elfio {

// ================================================================
//  rel_ref / rela_ref — proxy to a single relocation entry
// ================================================================

template <typename Traits>
class rel_ref {
    byte_view               file_;
    std::size_t             offset_;
    const endian_convertor* conv_;

    using rel_t = typename Traits::rel_type;

    [[nodiscard]] result<rel_t> entry() const noexcept {
        return file_.read<rel_t>(offset_);
    }

    template <typename T>
    [[nodiscard]] T cvt(T val) const noexcept {
        return conv_ ? (*conv_)(val) : val;
    }

public:
    rel_ref(byte_view file, std::size_t offset,
            const endian_convertor* conv) noexcept
        : file_(file), offset_(offset), conv_(conv) {}

    [[nodiscard]] uint64_t r_offset() const noexcept { auto e = entry(); return e ? cvt(e->r_offset) : 0; }
    [[nodiscard]] uint64_t r_info()   const noexcept { auto e = entry(); return e ? static_cast<uint64_t>(cvt(e->r_info)) : 0; }
    [[nodiscard]] uint64_t symbol()   const noexcept {
        auto e = entry();
        if (!e) return 0;
        auto info = cvt(e->r_info);
        return static_cast<uint64_t>(Traits::r_sym(info));
    }
    [[nodiscard]] uint64_t type() const noexcept {
        auto e = entry();
        if (!e) return 0;
        auto info = cvt(e->r_info);
        return static_cast<uint64_t>(Traits::r_type(info));
    }
};

template <typename Traits>
class rela_ref {
    byte_view               file_;
    std::size_t             offset_;
    const endian_convertor* conv_;

    using rela_t = typename Traits::rela_type;

    [[nodiscard]] result<rela_t> entry() const noexcept {
        return file_.read<rela_t>(offset_);
    }

    template <typename T>
    [[nodiscard]] T cvt(T val) const noexcept {
        return conv_ ? (*conv_)(val) : val;
    }

public:
    rela_ref(byte_view file, std::size_t offset,
             const endian_convertor* conv) noexcept
        : file_(file), offset_(offset), conv_(conv) {}

    [[nodiscard]] uint64_t r_offset() const noexcept { auto e = entry(); return e ? cvt(e->r_offset) : 0; }
    [[nodiscard]] uint64_t r_info()   const noexcept { auto e = entry(); return e ? static_cast<uint64_t>(cvt(e->r_info)) : 0; }
    [[nodiscard]] int64_t  r_addend() const noexcept { auto e = entry(); return e ? cvt(e->r_addend) : 0; }
    [[nodiscard]] uint64_t symbol()   const noexcept { return Traits::r_sym(r_info()); }
    [[nodiscard]] uint64_t type()     const noexcept { return Traits::r_type(r_info()); }
};

// ================================================================
//  rel_range / rela_range — lazy iterables
// ================================================================

template <typename Traits>
class rel_range {
    byte_view               file_;
    std::size_t             first_;
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
        using value_type        = rel_ref<Traits>;
        using pointer           = void;
        using reference         = value_type;
        using iterator_category = std::input_iterator_tag;

        iterator(byte_view d, std::size_t off, std::size_t idx,
                 const endian_convertor* c) noexcept
            : data_(d), offset_(off), index_(idx), conv_(c) {}

        value_type operator*() const { return {data_, offset_, conv_}; }
        iterator& operator++() { offset_ += sizeof(typename Traits::rel_type); ++index_; return *this; }
        iterator operator++(int) { auto t = *this; ++(*this); return t; }
        bool operator==(const iterator& o) const noexcept { return index_ == o.index_; }
        bool operator!=(const iterator& o) const noexcept { return index_ != o.index_; }
    };

    rel_range(byte_view data, std::size_t first, std::size_t count,
              const endian_convertor* conv) noexcept
        : file_(data), first_(first), count_(count), conv_(conv) {}

    [[nodiscard]] iterator    begin() const { return {file_, first_, 0, conv_}; }
    [[nodiscard]] iterator    end()   const { return {file_, 0, count_, conv_}; }
    [[nodiscard]] std::size_t size()  const noexcept { return count_; }
    [[nodiscard]] bool        empty() const noexcept { return count_ == 0; }
};

template <typename Traits>
class rela_range {
    byte_view               file_;
    std::size_t             first_;
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
        using value_type        = rela_ref<Traits>;
        using pointer           = void;
        using reference         = value_type;
        using iterator_category = std::input_iterator_tag;

        iterator(byte_view d, std::size_t off, std::size_t idx,
                 const endian_convertor* c) noexcept
            : data_(d), offset_(off), index_(idx), conv_(c) {}

        value_type operator*() const { return {data_, offset_, conv_}; }
        iterator& operator++() { offset_ += sizeof(typename Traits::rela_type); ++index_; return *this; }
        iterator operator++(int) { auto t = *this; ++(*this); return t; }
        bool operator==(const iterator& o) const noexcept { return index_ == o.index_; }
        bool operator!=(const iterator& o) const noexcept { return index_ != o.index_; }
    };

    rela_range(byte_view data, std::size_t first, std::size_t count,
               const endian_convertor* conv) noexcept
        : file_(data), first_(first), count_(count), conv_(conv) {}

    [[nodiscard]] iterator    begin() const { return {file_, first_, 0, conv_}; }
    [[nodiscard]] iterator    end()   const { return {file_, 0, count_, conv_}; }
    [[nodiscard]] std::size_t size()  const noexcept { return count_; }
    [[nodiscard]] bool        empty() const noexcept { return count_ == 0; }
};

} // namespace elfio
