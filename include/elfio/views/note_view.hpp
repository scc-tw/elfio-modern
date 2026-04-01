#pragma once

/// Zero-copy note section/segment views.
/// Notes have variable length: { namesz, descsz, type, name[], desc[] }
/// with 4-byte alignment padding after name and desc.

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <elfio/core/byte_view.hpp>
#include <elfio/core/endian.hpp>
#include <elfio/core/safe_math.hpp>
#include <elfio/platform/schema.hpp>

namespace elfio {

class note_ref {
    byte_view               data_;     // view of the entire note section/segment data
    std::size_t             offset_;   // offset within data_ to this note's Nhdr
    const endian_convertor* conv_;

    template <typename T>
    [[nodiscard]] T cvt(T val) const noexcept {
        return conv_ ? (*conv_)(val) : val;
    }

    static constexpr std::size_t align4(std::size_t v) noexcept {
        return (v + 3) & ~std::size_t(3);
    }

public:
    note_ref(byte_view data, std::size_t offset,
             const endian_convertor* conv) noexcept
        : data_(data), offset_(offset), conv_(conv) {}

    [[nodiscard]] result<Elf_Nhdr> header() const noexcept {
        return data_.read<Elf_Nhdr>(offset_);
    }

    [[nodiscard]] Elf_Word namesz() const noexcept { auto h = header(); return h ? cvt(h->n_namesz) : 0; }
    [[nodiscard]] Elf_Word descsz() const noexcept { auto h = header(); return h ? cvt(h->n_descsz) : 0; }
    [[nodiscard]] Elf_Word type()   const noexcept { auto h = header(); return h ? cvt(h->n_type)   : 0; }

    [[nodiscard]] std::string_view name() const noexcept {
        auto nsz = namesz();
        if (nsz == 0) return {};
        std::size_t name_off = offset_ + sizeof(Elf_Nhdr);
        // name may include trailing NUL in namesz
        auto sv = data_.read_cstring(name_off, nsz);
        return sv;
    }

    [[nodiscard]] byte_view desc() const noexcept {
        auto nsz = namesz();
        auto dsz = descsz();
        if (dsz == 0) return {};
        std::size_t desc_off = offset_ + sizeof(Elf_Nhdr) + align4(nsz);
        auto v = data_.subview(desc_off, dsz);
        return v ? *v : byte_view{};
    }

    /// Total size of this note entry (header + padded name + padded desc).
    [[nodiscard]] std::size_t total_size() const noexcept {
        return sizeof(Elf_Nhdr) + align4(namesz()) + align4(descsz());
    }
};

/// Iterate over all notes in a note section/segment.
class note_range {
    byte_view               data_;
    const endian_convertor* conv_;

public:
    class iterator {
        byte_view               data_;
        std::size_t             offset_;
        const endian_convertor* conv_;
    public:
        using difference_type   = std::ptrdiff_t;
        using value_type        = note_ref;
        using pointer           = void;
        using reference         = value_type;
        using iterator_category = std::input_iterator_tag;

        iterator(byte_view d, std::size_t off, const endian_convertor* c) noexcept
            : data_(d), offset_(off), conv_(c) {}

        value_type operator*() const { return {data_, offset_, conv_}; }
        iterator& operator++() {
            note_ref n{data_, offset_, conv_};
            auto sz = n.total_size();
            offset_ += (sz > 0) ? sz : data_.size(); // guard against zero-size
            return *this;
        }
        iterator operator++(int) { auto t = *this; ++(*this); return t; }
        bool operator==(const iterator& o) const noexcept {
            // Both past-end, or same position
            bool this_end = offset_ >= data_.size();
            bool o_end    = o.offset_ >= o.data_.size();
            if (this_end && o_end) return true;
            return offset_ == o.offset_;
        }
        bool operator!=(const iterator& o) const noexcept { return !(*this == o); }
    };

    note_range(byte_view data, const endian_convertor* conv) noexcept
        : data_(data), conv_(conv) {}

    [[nodiscard]] iterator begin() const { return {data_, 0, conv_}; }
    [[nodiscard]] iterator end()   const { return {data_, data_.size(), conv_}; }
    [[nodiscard]] bool     empty() const noexcept { return data_.empty(); }
};

} // namespace elfio
