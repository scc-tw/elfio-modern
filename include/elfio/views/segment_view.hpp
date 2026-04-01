#pragma once

/// Lazy, zero-allocation segment (program header) views.

#include <cstddef>
#include <cstdint>
#include <elfio/core/byte_view.hpp>
#include <elfio/core/endian.hpp>
#include <elfio/platform/schema.hpp>

namespace elfio {

// ================================================================
//  segment_ref<Traits> — proxy to one program header
// ================================================================

template <typename Traits>
class segment_ref {
    byte_view               file_;
    std::size_t             offset_;   // offset of this Phdr in file
    uint16_t                index_;
    const endian_convertor* conv_;

    using phdr_t = typename Traits::phdr_type;

    [[nodiscard]] result<phdr_t> hdr() const noexcept {
        return file_.read<phdr_t>(offset_);
    }

    template <typename T>
    [[nodiscard]] T cvt(T val) const noexcept {
        return conv_ ? (*conv_)(val) : val;
    }

public:
    segment_ref(byte_view file, std::size_t offset, uint16_t index,
                const endian_convertor* conv) noexcept
        : file_(file), offset_(offset), index_(index), conv_(conv) {}

    [[nodiscard]] uint16_t index()            const noexcept { return index_; }

    [[nodiscard]] Elf_Word  type()            const noexcept { auto h = hdr(); return h ? cvt(h->p_type)   : 0; }
    [[nodiscard]] Elf_Word  flags()           const noexcept { auto h = hdr(); return h ? cvt(h->p_flags)  : 0; }
    [[nodiscard]] uint64_t  offset_in_file()  const noexcept { auto h = hdr(); return h ? cvt(h->p_offset) : 0; }
    [[nodiscard]] uint64_t  virtual_address() const noexcept { auto h = hdr(); return h ? cvt(h->p_vaddr)  : 0; }
    [[nodiscard]] uint64_t  physical_address() const noexcept { auto h = hdr(); return h ? cvt(h->p_paddr)  : 0; }
    [[nodiscard]] uint64_t  file_size()       const noexcept { auto h = hdr(); return h ? cvt(h->p_filesz) : 0; }
    [[nodiscard]] uint64_t  memory_size()     const noexcept { auto h = hdr(); return h ? cvt(h->p_memsz)  : 0; }
    [[nodiscard]] uint64_t  align()           const noexcept { auto h = hdr(); return h ? cvt(h->p_align)  : 0; }

    /// Zero-copy view of this segment's file data.
    [[nodiscard]] byte_view data() const noexcept {
        auto off = offset_in_file();
        auto sz  = file_size();
        if (sz == 0) return {};
        auto v = file_.subview(static_cast<std::size_t>(off),
                               static_cast<std::size_t>(sz));
        return v ? *v : byte_view{};
    }

    // Convenience flag tests
    [[nodiscard]] bool is_load()    const noexcept { return type() == PT_LOAD; }
    [[nodiscard]] bool is_dynamic() const noexcept { return type() == PT_DYNAMIC; }
    [[nodiscard]] bool is_interp()  const noexcept { return type() == PT_INTERP; }
    [[nodiscard]] bool is_note()    const noexcept { return type() == PT_NOTE; }
    [[nodiscard]] bool is_phdr()    const noexcept { return type() == PT_PHDR; }
    [[nodiscard]] bool is_readable()   const noexcept { return (flags() & PF_R) != 0; }
    [[nodiscard]] bool is_writable()   const noexcept { return (flags() & PF_W) != 0; }
    [[nodiscard]] bool is_executable() const noexcept { return (flags() & PF_X) != 0; }
};

// ================================================================
//  segment_range<Traits> — lazy iterable over all program headers
// ================================================================

template <typename Traits>
class segment_range {
    byte_view               file_;
    std::size_t             first_;
    uint16_t                count_;
    const endian_convertor* conv_;

public:
    class iterator {
        byte_view               data_;
        std::size_t             offset_;
        uint16_t                index_;
        const endian_convertor* conv_;

    public:
        using difference_type   = std::ptrdiff_t;
        using value_type        = segment_ref<Traits>;
        using pointer           = void;
        using reference         = value_type;
        using iterator_category = std::input_iterator_tag;

        iterator(byte_view d, std::size_t off, uint16_t idx,
                 const endian_convertor* c) noexcept
            : data_(d), offset_(off), index_(idx), conv_(c) {}

        value_type operator*() const { return {data_, offset_, index_, conv_}; }
        iterator& operator++() {
            offset_ += sizeof(typename Traits::phdr_type);
            ++index_;
            return *this;
        }
        iterator operator++(int) { auto t = *this; ++(*this); return t; }
        bool operator==(const iterator& o) const noexcept { return index_ == o.index_; }
        bool operator!=(const iterator& o) const noexcept { return index_ != o.index_; }
    };

    segment_range(byte_view data, std::size_t first, uint16_t count,
                  const endian_convertor* conv) noexcept
        : file_(data), first_(first), count_(count), conv_(conv) {}

    [[nodiscard]] iterator    begin() const { return {file_, first_, 0, conv_}; }
    [[nodiscard]] iterator    end()   const { return {file_, 0, count_, conv_}; }
    [[nodiscard]] std::size_t size()  const noexcept { return count_; }
    [[nodiscard]] bool        empty() const noexcept { return count_ == 0; }

    [[nodiscard]] segment_ref<Traits> operator[](uint16_t i) const noexcept {
        return {file_,
                first_ + static_cast<std::size_t>(i) * sizeof(typename Traits::phdr_type),
                i, conv_};
    }
};

} // namespace elfio
