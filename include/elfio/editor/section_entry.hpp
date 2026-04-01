#pragma once

/// Mutable section entry for the ELF editor.
/// Owns a copy of section data and header fields.

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <elfio/platform/schema.hpp>

namespace elfio {

template <typename Traits>
class section_entry {
    std::string        name_;
    Elf_Word           type_       = SHT_NULL;
    uint64_t           flags_      = 0;
    uint64_t           address_    = 0;
    Elf_Word           link_       = 0;
    Elf_Word           info_       = 0;
    uint64_t           addr_align_ = 1;
    uint64_t           entry_size_ = 0;
    std::vector<char>  data_;

    // Set during layout
    uint64_t           offset_ = 0;
    Elf_Word           name_index_ = 0;
    uint16_t           index_ = 0;

public:
    section_entry() = default;

    section_entry(std::string name, Elf_Word type, uint64_t flags = 0)
        : name_(std::move(name)), type_(type), flags_(flags) {}

    // ── Accessors ───────────────────────────────────────────────
    [[nodiscard]] const std::string& name()       const noexcept { return name_; }
    [[nodiscard]] Elf_Word           type()       const noexcept { return type_; }
    [[nodiscard]] uint64_t           flags()      const noexcept { return flags_; }
    [[nodiscard]] uint64_t           address()    const noexcept { return address_; }
    [[nodiscard]] Elf_Word           link()       const noexcept { return link_; }
    [[nodiscard]] Elf_Word           info()       const noexcept { return info_; }
    [[nodiscard]] uint64_t           addr_align() const noexcept { return addr_align_; }
    [[nodiscard]] uint64_t           entry_size() const noexcept { return entry_size_; }
    [[nodiscard]] uint64_t           offset()     const noexcept { return offset_; }
    [[nodiscard]] Elf_Word           name_index() const noexcept { return name_index_; }
    [[nodiscard]] uint16_t           index()      const noexcept { return index_; }

    [[nodiscard]] const std::vector<char>& data()     const noexcept { return data_; }
    [[nodiscard]] std::size_t              data_size() const noexcept {
        return type_ == SHT_NOBITS ? 0 : data_.size();
    }
    [[nodiscard]] uint64_t size() const noexcept { return data_.size(); }

    // ── Mutators ────────────────────────────────────────────────
    void set_name(std::string n)       { name_ = std::move(n); }
    void set_type(Elf_Word t)          { type_ = t; }
    void set_flags(uint64_t f)         { flags_ = f; }
    void set_address(uint64_t a)       { address_ = a; }
    void set_link(Elf_Word l)          { link_ = l; }
    void set_info(Elf_Word i)          { info_ = i; }
    void set_addr_align(uint64_t a)    { addr_align_ = a; }
    void set_entry_size(uint64_t e)    { entry_size_ = e; }

    void set_data(const char* d, std::size_t sz) {
        data_.assign(d, d + sz);
    }
    void set_data(const std::vector<char>& d) { data_ = d; }
    void set_data(std::vector<char>&& d)      { data_ = std::move(d); }
    void append_data(const char* d, std::size_t sz) {
        data_.insert(data_.end(), d, d + sz);
    }

    // ── Layout helpers (called by layout engine) ────────────────
    void set_offset(uint64_t o)       { offset_ = o; }
    void set_name_index(Elf_Word ni)  { name_index_ = ni; }
    void set_index(uint16_t i)        { index_ = i; }
};

} // namespace elfio
