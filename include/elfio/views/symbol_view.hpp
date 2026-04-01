#pragma once

/// Zero-copy symbol table views.
///
///   symbol_ref<Traits>   — proxy to a single symbol entry
///   symbol_range<Traits> — iterable range over a symbol table section

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <elfio/core/byte_view.hpp>
#include <elfio/core/endian.hpp>
#include <elfio/platform/schema.hpp>
#include <elfio/views/string_table.hpp>

namespace elfio {

template <typename Traits>
class symbol_ref {
    byte_view               file_;
    std::size_t             offset_;  // offset of this Sym entry in file
    const string_table_view* strtab_;
    const endian_convertor* conv_;

    using sym_t = typename Traits::sym_type;

    [[nodiscard]] result<sym_t> sym() const noexcept {
        return file_.read<sym_t>(offset_);
    }

    template <typename T>
    [[nodiscard]] T cvt(T val) const noexcept {
        return conv_ ? (*conv_)(val) : val;
    }

public:
    symbol_ref(byte_view file, std::size_t offset,
               const string_table_view* strtab,
               const endian_convertor* conv) noexcept
        : file_(file), offset_(offset), strtab_(strtab), conv_(conv) {}

    [[nodiscard]] std::string_view name() const noexcept {
        auto s = sym();
        if (!s || !strtab_) return {};
        return strtab_->get(cvt(s->st_name));
    }

    [[nodiscard]] uint64_t      value()      const noexcept { auto s = sym(); return s ? cvt(s->st_value) : 0; }
    [[nodiscard]] uint64_t      size()       const noexcept { auto s = sym(); return s ? cvt(s->st_size)  : 0; }
    [[nodiscard]] unsigned char info()       const noexcept { auto s = sym(); return s ? s->st_info       : 0; }
    [[nodiscard]] unsigned char other()      const noexcept { auto s = sym(); return s ? s->st_other      : 0; }
    [[nodiscard]] Elf_Half      shndx()      const noexcept { auto s = sym(); return s ? cvt(s->st_shndx) : 0; }

    [[nodiscard]] unsigned char bind()       const noexcept { return elf_st_bind(info()); }
    [[nodiscard]] unsigned char type()       const noexcept { return elf_st_type(info()); }
    [[nodiscard]] unsigned char visibility() const noexcept { return elf_st_visibility(other()); }

    [[nodiscard]] bool is_local()  const noexcept { return bind() == STB_LOCAL; }
    [[nodiscard]] bool is_global() const noexcept { return bind() == STB_GLOBAL; }
    [[nodiscard]] bool is_weak()   const noexcept { return bind() == STB_WEAK; }
    [[nodiscard]] bool is_func()   const noexcept { return type() == STT_FUNC; }
    [[nodiscard]] bool is_object() const noexcept { return type() == STT_OBJECT; }
    [[nodiscard]] bool is_undef()  const noexcept { return shndx() == SHN_UNDEF; }
};

template <typename Traits>
class symbol_range {
    byte_view                file_;
    std::size_t              first_;   // offset of first Sym
    std::size_t              count_;
    const string_table_view* strtab_;
    const endian_convertor*  conv_;

public:
    class iterator {
        byte_view                data_;
        std::size_t              offset_;
        std::size_t              index_;
        const string_table_view* strtab_;
        const endian_convertor*  conv_;

    public:
        using difference_type   = std::ptrdiff_t;
        using value_type        = symbol_ref<Traits>;
        using pointer           = void;
        using reference         = value_type;
        using iterator_category = std::input_iterator_tag;

        iterator(byte_view d, std::size_t off, std::size_t idx,
                 const string_table_view* s, const endian_convertor* c) noexcept
            : data_(d), offset_(off), index_(idx), strtab_(s), conv_(c) {}

        value_type operator*() const { return {data_, offset_, strtab_, conv_}; }
        iterator& operator++() {
            offset_ += sizeof(typename Traits::sym_type);
            ++index_;
            return *this;
        }
        iterator operator++(int) { auto t = *this; ++(*this); return t; }
        bool operator==(const iterator& o) const noexcept { return index_ == o.index_; }
        bool operator!=(const iterator& o) const noexcept { return index_ != o.index_; }
    };

    symbol_range(byte_view data, std::size_t first, std::size_t count,
                 const string_table_view* strtab,
                 const endian_convertor* conv) noexcept
        : file_(data), first_(first), count_(count),
          strtab_(strtab), conv_(conv) {}

    [[nodiscard]] iterator    begin() const { return {file_, first_, 0, strtab_, conv_}; }
    [[nodiscard]] iterator    end()   const { return {file_, 0, count_, strtab_, conv_}; }
    [[nodiscard]] std::size_t size()  const noexcept { return count_; }
    [[nodiscard]] bool        empty() const noexcept { return count_ == 0; }

    [[nodiscard]] symbol_ref<Traits> operator[](std::size_t i) const noexcept {
        return {file_,
                first_ + i * sizeof(typename Traits::sym_type),
                strtab_, conv_};
    }
};

} // namespace elfio
