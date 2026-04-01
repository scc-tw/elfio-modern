#pragma once

/// Zero-copy version symbol views.
///   - versym_range:        .gnu.version entries (uint16 per symbol)
///   - verneed_range:       .gnu.version_r entries
///   - verdef_range:        .gnu.version_d entries

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <elfio/core/byte_view.hpp>
#include <elfio/core/endian.hpp>
#include <elfio/platform/schema.hpp>
#include <elfio/views/string_table.hpp>

namespace elfio {

// ================================================================
//  versym_range — .gnu.version entries (one Elf_Half per symbol)
// ================================================================

class versym_ref {
    byte_view               data_;
    std::size_t             offset_;
    const endian_convertor* conv_;

public:
    versym_ref(byte_view data, std::size_t offset,
               const endian_convertor* conv) noexcept
        : data_(data), offset_(offset), conv_(conv) {}

    [[nodiscard]] Elf_Half value() const noexcept {
        auto r = data_.read<Elf_Half>(offset_);
        return r ? (conv_ ? (*conv_)(*r) : *r) : 0;
    }
};

class versym_range {
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
        using value_type        = versym_ref;
        using pointer           = void;
        using reference         = value_type;
        using iterator_category = std::input_iterator_tag;

        iterator(byte_view d, std::size_t off, std::size_t idx,
                 const endian_convertor* c) noexcept
            : data_(d), offset_(off), index_(idx), conv_(c) {}

        value_type operator*() const { return {data_, offset_, conv_}; }
        iterator& operator++() { offset_ += sizeof(Elf_Half); ++index_; return *this; }
        iterator operator++(int) { auto t = *this; ++(*this); return t; }
        bool operator==(const iterator& o) const noexcept { return index_ == o.index_; }
        bool operator!=(const iterator& o) const noexcept { return index_ != o.index_; }
    };

    versym_range(byte_view data, const endian_convertor* conv) noexcept
        : data_(data), count_(data.size() / sizeof(Elf_Half)), conv_(conv) {}

    [[nodiscard]] iterator    begin() const { return {data_, 0, 0, conv_}; }
    [[nodiscard]] iterator    end()   const { return {data_, 0, count_, conv_}; }
    [[nodiscard]] std::size_t size()  const noexcept { return count_; }
};

// ================================================================
//  verneed_ref / verneed_range — .gnu.version_r
// ================================================================

class verneed_ref {
    byte_view                data_;
    std::size_t              offset_;
    const string_table_view* strtab_;
    const endian_convertor*  conv_;

    template <typename T>
    [[nodiscard]] T cvt(T val) const noexcept {
        return conv_ ? (*conv_)(val) : val;
    }

public:
    verneed_ref(byte_view data, std::size_t offset,
                const string_table_view* strtab,
                const endian_convertor* conv) noexcept
        : data_(data), offset_(offset), strtab_(strtab), conv_(conv) {}

    [[nodiscard]] Elf_Half version() const noexcept {
        auto r = data_.read<Elfxx_Verneed>(offset_);
        return r ? cvt(r->vn_version) : 0;
    }
    [[nodiscard]] Elf_Half cnt() const noexcept {
        auto r = data_.read<Elfxx_Verneed>(offset_);
        return r ? cvt(r->vn_cnt) : 0;
    }
    [[nodiscard]] std::string_view file_name() const noexcept {
        auto r = data_.read<Elfxx_Verneed>(offset_);
        if (!r || !strtab_) return {};
        return strtab_->get(cvt(r->vn_file));
    }
    [[nodiscard]] Elf_Word aux_offset() const noexcept {
        auto r = data_.read<Elfxx_Verneed>(offset_);
        return r ? cvt(r->vn_aux) : 0;
    }
    [[nodiscard]] Elf_Word next_offset() const noexcept {
        auto r = data_.read<Elfxx_Verneed>(offset_);
        return r ? cvt(r->vn_next) : 0;
    }
};

class verneed_range {
    byte_view                data_;
    const string_table_view* strtab_;
    const endian_convertor*  conv_;

public:
    class iterator {
        byte_view                data_;
        std::size_t              offset_;
        const string_table_view* strtab_;
        const endian_convertor*  conv_;
        bool                     done_ = false;
    public:
        using difference_type   = std::ptrdiff_t;
        using value_type        = verneed_ref;
        using pointer           = void;
        using reference         = value_type;
        using iterator_category = std::input_iterator_tag;

        iterator(byte_view d, std::size_t off, const string_table_view* s,
                 const endian_convertor* c, bool done) noexcept
            : data_(d), offset_(off), strtab_(s), conv_(c), done_(done) {}

        value_type operator*() const { return {data_, offset_, strtab_, conv_}; }
        iterator& operator++() {
            verneed_ref r{data_, offset_, strtab_, conv_};
            auto next = r.next_offset();
            if (next == 0) { done_ = true; }
            else { offset_ += next; }
            return *this;
        }
        iterator operator++(int) { auto t = *this; ++(*this); return t; }
        bool operator==(const iterator& o) const noexcept { return done_ == o.done_; }
        bool operator!=(const iterator& o) const noexcept { return !(*this == o); }
    };

    verneed_range(byte_view data, const string_table_view* strtab,
                  const endian_convertor* conv) noexcept
        : data_(data), strtab_(strtab), conv_(conv) {}

    [[nodiscard]] iterator begin() const { return {data_, 0, strtab_, conv_, data_.empty()}; }
    [[nodiscard]] iterator end()   const { return {data_, 0, strtab_, conv_, true}; }
};

// ================================================================
//  verdef_ref / verdef_range — .gnu.version_d
// ================================================================

class verdef_ref {
    byte_view                data_;
    std::size_t              offset_;
    const string_table_view* strtab_;
    const endian_convertor*  conv_;

    template <typename T>
    [[nodiscard]] T cvt(T val) const noexcept {
        return conv_ ? (*conv_)(val) : val;
    }

public:
    verdef_ref(byte_view data, std::size_t offset,
               const string_table_view* strtab,
               const endian_convertor* conv) noexcept
        : data_(data), offset_(offset), strtab_(strtab), conv_(conv) {}

    [[nodiscard]] Elf_Half version() const noexcept {
        auto r = data_.read<Elfxx_Verdef>(offset_);
        return r ? cvt(r->vd_version) : 0;
    }
    [[nodiscard]] Elf_Half flags() const noexcept {
        auto r = data_.read<Elfxx_Verdef>(offset_);
        return r ? cvt(r->vd_flags) : 0;
    }
    [[nodiscard]] Elf_Half ndx() const noexcept {
        auto r = data_.read<Elfxx_Verdef>(offset_);
        return r ? cvt(r->vd_ndx) : 0;
    }
    [[nodiscard]] Elf_Word hash() const noexcept {
        auto r = data_.read<Elfxx_Verdef>(offset_);
        return r ? cvt(r->vd_hash) : 0;
    }
    [[nodiscard]] Elf_Word next_offset() const noexcept {
        auto r = data_.read<Elfxx_Verdef>(offset_);
        return r ? cvt(r->vd_next) : 0;
    }
    /// Get the name from the first aux entry.
    [[nodiscard]] std::string_view name() const noexcept {
        auto r = data_.read<Elfxx_Verdef>(offset_);
        if (!r || !strtab_) return {};
        auto aux_off = offset_ + cvt(r->vd_aux);
        auto aux = data_.read<Elfxx_Verdaux>(aux_off);
        if (!aux) return {};
        return strtab_->get(cvt(aux->vda_name));
    }
};

class verdef_range {
    byte_view                data_;
    const string_table_view* strtab_;
    const endian_convertor*  conv_;

public:
    class iterator {
        byte_view                data_;
        std::size_t              offset_;
        const string_table_view* strtab_;
        const endian_convertor*  conv_;
        bool                     done_ = false;
    public:
        using difference_type   = std::ptrdiff_t;
        using value_type        = verdef_ref;
        using pointer           = void;
        using reference         = value_type;
        using iterator_category = std::input_iterator_tag;

        iterator(byte_view d, std::size_t off, const string_table_view* s,
                 const endian_convertor* c, bool done) noexcept
            : data_(d), offset_(off), strtab_(s), conv_(c), done_(done) {}

        value_type operator*() const { return {data_, offset_, strtab_, conv_}; }
        iterator& operator++() {
            verdef_ref r{data_, offset_, strtab_, conv_};
            auto next = r.next_offset();
            if (next == 0) { done_ = true; }
            else { offset_ += next; }
            return *this;
        }
        iterator operator++(int) { auto t = *this; ++(*this); return t; }
        bool operator==(const iterator& o) const noexcept { return done_ == o.done_; }
        bool operator!=(const iterator& o) const noexcept { return !(*this == o); }
    };

    verdef_range(byte_view data, const string_table_view* strtab,
                 const endian_convertor* conv) noexcept
        : data_(data), strtab_(strtab), conv_(conv) {}

    [[nodiscard]] iterator begin() const { return {data_, 0, strtab_, conv_, data_.empty()}; }
    [[nodiscard]] iterator end()   const { return {data_, 0, strtab_, conv_, true}; }
};

} // namespace elfio
