#pragma once

/// Lazy, zero-allocation section views.
///
///   section_ref<Traits>   — lightweight proxy to a single section header
///   section_range<Traits> — iterable range over all sections

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <elfio/core/byte_view.hpp>
#include <elfio/core/endian.hpp>
#include <elfio/platform/schema.hpp>
#include <elfio/views/string_table.hpp>

namespace elfio {

// ================================================================
//  section_ref<Traits> — proxy to one section header
// ================================================================

template <typename Traits>
class section_ref {
    byte_view                   file_;
    std::size_t                 offset_;   // offset of this Shdr in the file
    uint16_t                    index_;    // section index
    const string_table_view*    shstrtab_; // section name string table
    const endian_convertor*     conv_;

    using shdr_t = typename Traits::shdr_type;

    [[nodiscard]] result<shdr_t> hdr() const noexcept {
        return file_.read<shdr_t>(offset_);
    }

    template <typename T>
    [[nodiscard]] T cvt(T val) const noexcept {
        return conv_ ? (*conv_)(val) : val;
    }

public:
    section_ref(byte_view file, std::size_t offset, uint16_t index,
                const string_table_view* strtab,
                const endian_convertor* conv) noexcept
        : file_(file), offset_(offset), index_(index),
          shstrtab_(strtab), conv_(conv) {}

    [[nodiscard]] uint16_t index() const noexcept { return index_; }

    [[nodiscard]] std::string_view name() const noexcept {
        auto h = hdr();
        if (!h || !shstrtab_) return {};
        return shstrtab_->get(cvt(h->sh_name));
    }

    [[nodiscard]] Elf_Word  sh_name()      const noexcept { auto h = hdr(); return h ? cvt(h->sh_name)      : 0; }
    [[nodiscard]] Elf_Word  type()         const noexcept { auto h = hdr(); return h ? cvt(h->sh_type)      : 0; }
    [[nodiscard]] uint64_t  flags()        const noexcept { auto h = hdr(); return h ? cvt(h->sh_flags)     : 0; }
    [[nodiscard]] uint64_t  address()      const noexcept { auto h = hdr(); return h ? cvt(h->sh_addr)      : 0; }
    [[nodiscard]] uint64_t  offset()       const noexcept { auto h = hdr(); return h ? cvt(h->sh_offset)    : 0; }
    [[nodiscard]] uint64_t  size()         const noexcept { auto h = hdr(); return h ? cvt(h->sh_size)      : 0; }
    [[nodiscard]] Elf_Word  link()         const noexcept { auto h = hdr(); return h ? cvt(h->sh_link)      : 0; }
    [[nodiscard]] Elf_Word  info()         const noexcept { auto h = hdr(); return h ? cvt(h->sh_info)      : 0; }
    [[nodiscard]] uint64_t  addr_align()   const noexcept { auto h = hdr(); return h ? cvt(h->sh_addralign) : 0; }
    [[nodiscard]] uint64_t  entry_size()   const noexcept { auto h = hdr(); return h ? cvt(h->sh_entsize)   : 0; }

    /// Zero-copy view of this section's raw data.
    [[nodiscard]] byte_view data() const noexcept {
        auto off = offset();
        auto sz  = size();
        if (sz == 0 || type() == SHT_NOBITS) return {};
        auto v = file_.subview(static_cast<std::size_t>(off),
                               static_cast<std::size_t>(sz));
        return v ? *v : byte_view{};
    }

    // Convenience flag tests
    [[nodiscard]] bool is_alloc()      const noexcept { return (flags() & SHF_ALLOC)    != 0; }
    [[nodiscard]] bool is_write()      const noexcept { return (flags() & SHF_WRITE)    != 0; }
    [[nodiscard]] bool is_execinstr()  const noexcept { return (flags() & SHF_EXECINSTR) != 0; }
    [[nodiscard]] bool is_nobits()     const noexcept { return type() == SHT_NOBITS; }
    [[nodiscard]] bool is_progbits()   const noexcept { return type() == SHT_PROGBITS; }
    [[nodiscard]] bool is_symtab()     const noexcept { return type() == SHT_SYMTAB || type() == SHT_DYNSYM; }
    [[nodiscard]] bool is_strtab()     const noexcept { return type() == SHT_STRTAB; }
    [[nodiscard]] bool is_rel()        const noexcept { return type() == SHT_REL; }
    [[nodiscard]] bool is_rela()       const noexcept { return type() == SHT_RELA; }
    [[nodiscard]] bool is_dynamic()    const noexcept { return type() == SHT_DYNAMIC; }
    [[nodiscard]] bool is_note()       const noexcept { return type() == SHT_NOTE; }
};

// ================================================================
//  section_range<Traits> — lazy iterable over all section headers
// ================================================================

template <typename Traits>
class section_range {
    byte_view                file_;
    std::size_t              first_;    // file offset of first Shdr
    uint16_t                 count_;
    const string_table_view* shstrtab_;
    const endian_convertor*  conv_;

public:
    class iterator {
        byte_view                data_;
        std::size_t              offset_;
        uint16_t                 index_;
        const string_table_view* strtab_;
        const endian_convertor*  conv_;

    public:
        using difference_type   = std::ptrdiff_t;
        using value_type        = section_ref<Traits>;
        using pointer           = void;
        using reference         = value_type;
        using iterator_category = std::input_iterator_tag;

        iterator(byte_view d, std::size_t off, uint16_t idx,
                 const string_table_view* s, const endian_convertor* c) noexcept
            : data_(d), offset_(off), index_(idx), strtab_(s), conv_(c) {}

        value_type operator*() const { return {data_, offset_, index_, strtab_, conv_}; }
        iterator& operator++() {
            offset_ += sizeof(typename Traits::shdr_type);
            ++index_;
            return *this;
        }
        iterator operator++(int) { auto t = *this; ++(*this); return t; }
        bool operator==(const iterator& o) const noexcept { return index_ == o.index_; }
        bool operator!=(const iterator& o) const noexcept { return index_ != o.index_; }
    };

    section_range(byte_view data, std::size_t first, uint16_t count,
                  const string_table_view* strtab,
                  const endian_convertor* conv) noexcept
        : file_(data), first_(first), count_(count),
          shstrtab_(strtab), conv_(conv) {}

    [[nodiscard]] iterator    begin() const { return {file_, first_, 0, shstrtab_, conv_}; }
    [[nodiscard]] iterator    end()   const { return {file_, 0, count_, shstrtab_, conv_}; }
    [[nodiscard]] std::size_t size()  const noexcept { return count_; }
    [[nodiscard]] bool        empty() const noexcept { return count_ == 0; }

    [[nodiscard]] section_ref<Traits> operator[](uint16_t i) const noexcept {
        return {file_,
                first_ + static_cast<std::size_t>(i) * sizeof(typename Traits::shdr_type),
                i, shstrtab_, conv_};
    }
};

} // namespace elfio
