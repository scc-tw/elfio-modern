#pragma once

/// Mutable segment entry for the ELF editor.

#include <cstddef>
#include <cstdint>
#include <vector>
#include <elfio/platform/schema.hpp>

namespace elfio {

template <typename Traits>
class segment_entry {
    Elf_Word   type_      = PT_NULL;
    Elf_Word   flags_     = 0;
    uint64_t   vaddr_     = 0;
    uint64_t   paddr_     = 0;
    uint64_t   filesz_    = 0;
    uint64_t   memsz_     = 0;
    uint64_t   align_     = 0;

    // Section indices associated with this segment
    std::vector<uint16_t> section_indices_;

    // Set during layout
    uint64_t   offset_    = 0;
    uint16_t   index_     = 0;

public:
    segment_entry() = default;

    segment_entry(Elf_Word type, Elf_Word flags = 0)
        : type_(type), flags_(flags) {}

    // ── Accessors ───────────────────────────────────────────────
    [[nodiscard]] Elf_Word   type()      const noexcept { return type_; }
    [[nodiscard]] Elf_Word   flags()     const noexcept { return flags_; }
    [[nodiscard]] uint64_t   vaddr()     const noexcept { return vaddr_; }
    [[nodiscard]] uint64_t   paddr()     const noexcept { return paddr_; }
    [[nodiscard]] uint64_t   filesz()    const noexcept { return filesz_; }
    [[nodiscard]] uint64_t   memsz()     const noexcept { return memsz_; }
    [[nodiscard]] uint64_t   align()     const noexcept { return align_; }
    [[nodiscard]] uint64_t   offset()    const noexcept { return offset_; }
    [[nodiscard]] uint16_t   index()     const noexcept { return index_; }

    [[nodiscard]] const std::vector<uint16_t>& section_indices() const noexcept {
        return section_indices_;
    }

    // ── Mutators ────────────────────────────────────────────────
    void set_type(Elf_Word t)       { type_ = t; }
    void set_flags(Elf_Word f)      { flags_ = f; }
    void set_vaddr(uint64_t v)      { vaddr_ = v; }
    void set_paddr(uint64_t p)      { paddr_ = p; }
    void set_filesz(uint64_t s)     { filesz_ = s; }
    void set_memsz(uint64_t s)      { memsz_ = s; }
    void set_align(uint64_t a)      { align_ = a; }

    void add_section_index(uint16_t idx) { section_indices_.push_back(idx); }
    void clear_section_indices()         { section_indices_.clear(); }

    // ── Layout helpers ──────────────────────────────────────────
    void set_offset(uint64_t o)     { offset_ = o; }
    void set_index(uint16_t i)      { index_ = i; }
};

} // namespace elfio
