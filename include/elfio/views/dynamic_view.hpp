#pragma once

/// Zero-copy dynamic section views.

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <elfio/core/byte_view.hpp>
#include <elfio/core/endian.hpp>
#include <elfio/platform/schema.hpp>
#include <elfio/views/string_table.hpp>

namespace elfio {

template <typename Traits>
class dynamic_ref {
    byte_view                file_;
    std::size_t              offset_;
    const string_table_view* strtab_;
    const endian_convertor*  conv_;

    using dyn_t = typename Traits::dyn_type;

    [[nodiscard]] result<dyn_t> entry() const noexcept {
        return file_.read<dyn_t>(offset_);
    }

    template <typename T>
    [[nodiscard]] T cvt(T val) const noexcept {
        return conv_ ? (*conv_)(val) : val;
    }

public:
    dynamic_ref(byte_view file, std::size_t offset,
                const string_table_view* strtab,
                const endian_convertor* conv) noexcept
        : file_(file), offset_(offset), strtab_(strtab), conv_(conv) {}

    [[nodiscard]] int64_t  tag()   const noexcept { auto e = entry(); return e ? cvt(e->d_tag)    : 0; }
    [[nodiscard]] uint64_t value() const noexcept { auto e = entry(); return e ? cvt(e->d_un.d_val) : 0; }

    /// For DT_NEEDED, DT_SONAME, DT_RPATH, DT_RUNPATH — look up the string.
    [[nodiscard]] std::string_view string_value() const noexcept {
        if (!strtab_) return {};
        return strtab_->get(static_cast<std::size_t>(value()));
    }

    [[nodiscard]] bool is_null() const noexcept { return tag() == static_cast<int64_t>(DT_NULL); }
};

template <typename Traits>
class dynamic_range {
    byte_view                file_;
    std::size_t              first_;
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
        using value_type        = dynamic_ref<Traits>;
        using pointer           = void;
        using reference         = value_type;
        using iterator_category = std::input_iterator_tag;

        iterator(byte_view d, std::size_t off, std::size_t idx,
                 const string_table_view* s, const endian_convertor* c) noexcept
            : data_(d), offset_(off), index_(idx), strtab_(s), conv_(c) {}

        value_type operator*() const { return {data_, offset_, strtab_, conv_}; }
        iterator& operator++() { offset_ += sizeof(typename Traits::dyn_type); ++index_; return *this; }
        iterator operator++(int) { auto t = *this; ++(*this); return t; }
        bool operator==(const iterator& o) const noexcept { return index_ == o.index_; }
        bool operator!=(const iterator& o) const noexcept { return index_ != o.index_; }
    };

    dynamic_range(byte_view data, std::size_t first, std::size_t count,
                  const string_table_view* strtab,
                  const endian_convertor* conv) noexcept
        : file_(data), first_(first), count_(count),
          strtab_(strtab), conv_(conv) {}

    [[nodiscard]] iterator    begin() const { return {file_, first_, 0, strtab_, conv_}; }
    [[nodiscard]] iterator    end()   const { return {file_, 0, count_, strtab_, conv_}; }
    [[nodiscard]] std::size_t size()  const noexcept { return count_; }
    [[nodiscard]] bool        empty() const noexcept { return count_ == 0; }
};

} // namespace elfio
