#pragma once

/// elf_file<Traits> — the primary zero-copy read API.
///
/// Usage:
///   auto buf = /* memory-mapped or read file into vector */;
///   byte_view view{buf.data(), buf.size()};
///   auto file = elf_file<elf64_traits>::from_view(view);
///   if (!file) { /* handle error */ }
///   for (auto sec : file->sections()) { ... }

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <variant>
#include <elfio/core/byte_view.hpp>
#include <elfio/core/endian.hpp>
#include <elfio/core/result.hpp>
#include <elfio/platform/schema.hpp>
#include <elfio/platform/traits.hpp>
#include <elfio/views/string_table.hpp>
#include <elfio/views/section_view.hpp>
#include <elfio/views/segment_view.hpp>
#include <elfio/views/symbol_view.hpp>
#include <elfio/views/relocation_view.hpp>
#include <elfio/views/dynamic_view.hpp>
#include <elfio/views/note_view.hpp>

namespace elfio {

// ================================================================
//  elf_file<Traits> — zero-copy, stateless ELF reader
// ================================================================

template <typename Traits>
class elf_file {
    byte_view          data_;
    endian_convertor   conv_;
    string_table_view  shstrtab_;

    // Cached header fields (converted to native byte order)
    Elf_Half  type_     = 0;
    Elf_Half  machine_  = 0;
    Elf_Word  version_  = 0;
    Elf_Word  flags_    = 0;
    uint64_t  entry_    = 0;
    uint64_t  phoff_    = 0;
    uint64_t  shoff_    = 0;
    Elf_Half  phnum_    = 0;
    Elf_Half  shnum_    = 0;
    Elf_Half  shstrndx_ = 0;

    unsigned char os_abi_      = 0;
    unsigned char abi_version_ = 0;

    elf_file() = default;

public:
    using traits_type = Traits;
    using ehdr_type   = typename Traits::ehdr_type;
    using shdr_type   = typename Traits::shdr_type;
    using phdr_type   = typename Traits::phdr_type;

    /// Parse an ELF file from a byte_view. Zero-copy — the view must
    /// outlive this elf_file object.
    [[nodiscard]] static result<elf_file> from_view(byte_view data) noexcept {
        // Minimum size: ELF identification
        if (data.size() < EI_NIDENT)
            return error_code::file_too_small;

        // Validate magic
        auto ident = data.data();
        if (static_cast<unsigned char>(ident[EI_MAG0]) != ELFMAG0 ||
            static_cast<unsigned char>(ident[EI_MAG1]) != ELFMAG1 ||
            static_cast<unsigned char>(ident[EI_MAG2]) != ELFMAG2 ||
            static_cast<unsigned char>(ident[EI_MAG3]) != ELFMAG3)
            return error_code::invalid_magic;

        // Validate class
        auto file_class = static_cast<unsigned char>(ident[EI_CLASS]);
        if (file_class != Traits::file_class)
            return error_code::invalid_class;

        // Validate and setup encoding
        auto encoding = static_cast<unsigned char>(ident[EI_DATA]);
        if (encoding != ELFDATA2LSB && encoding != ELFDATA2MSB)
            return error_code::invalid_encoding;

        elf_file f;
        f.data_ = data;
        f.conv_.setup(encoding == ELFDATA2LSB ? byte_order::little : byte_order::big);

        // Read the full header
        auto ehdr = data.read<ehdr_type>(0);
        if (!ehdr) return ehdr.error();

        auto& h = *ehdr;
        f.type_        = f.conv_(h.e_type);
        f.machine_     = f.conv_(h.e_machine);
        f.version_     = f.conv_(h.e_version);
        f.entry_       = f.conv_(h.e_entry);
        f.phoff_       = f.conv_(h.e_phoff);
        f.shoff_       = f.conv_(h.e_shoff);
        f.flags_       = f.conv_(h.e_flags);
        f.phnum_       = f.conv_(h.e_phnum);
        f.shnum_       = f.conv_(h.e_shnum);
        f.shstrndx_    = f.conv_(h.e_shstrndx);
        f.os_abi_      = h.e_ident[EI_OSABI];
        f.abi_version_ = h.e_ident[EI_ABIVERSION];

        // Build section name string table
        if (f.shnum_ > 0 && f.shstrndx_ < f.shnum_) {
            auto strtab_off = static_cast<std::size_t>(f.shoff_)
                            + static_cast<std::size_t>(f.shstrndx_) * sizeof(shdr_type);
            auto strhdr = data.read<shdr_type>(strtab_off);
            if (strhdr) {
                auto str_offset = static_cast<std::size_t>(f.conv_(strhdr->sh_offset));
                auto str_size   = static_cast<std::size_t>(f.conv_(strhdr->sh_size));
                auto sv = data.subview(str_offset, str_size);
                if (sv) f.shstrtab_ = string_table_view{*sv};
            }
        }

        return f;
    }

    // ── Header accessors ────────────────────────────────────────
    [[nodiscard]] unsigned char file_class()   const noexcept { return Traits::file_class; }
    [[nodiscard]] unsigned char encoding()     const noexcept {
        return conv_.order() == byte_order::little ? ELFDATA2LSB : ELFDATA2MSB;
    }
    [[nodiscard]] unsigned char os_abi()       const noexcept { return os_abi_; }
    [[nodiscard]] unsigned char abi_version()  const noexcept { return abi_version_; }
    [[nodiscard]] Elf_Half      type()         const noexcept { return type_; }
    [[nodiscard]] Elf_Half      machine()      const noexcept { return machine_; }
    [[nodiscard]] Elf_Word      version()      const noexcept { return version_; }
    [[nodiscard]] Elf_Word      flags()        const noexcept { return flags_; }
    [[nodiscard]] uint64_t      entry()        const noexcept { return entry_; }
    [[nodiscard]] Elf_Half      section_count() const noexcept { return shnum_; }
    [[nodiscard]] Elf_Half      segment_count() const noexcept { return phnum_; }

    [[nodiscard]] const byte_view&        raw_data()   const noexcept { return data_; }
    [[nodiscard]] const endian_convertor&  convertor()  const noexcept { return conv_; }

    // ── Section iteration ───────────────────────────────────────
    [[nodiscard]] section_range<Traits> sections() const noexcept {
        return {data_, static_cast<std::size_t>(shoff_), shnum_,
                &shstrtab_, &conv_};
    }

    // ── Segment iteration ───────────────────────────────────────
    [[nodiscard]] segment_range<Traits> segments() const noexcept {
        return {data_, static_cast<std::size_t>(phoff_), phnum_, &conv_};
    }

    // ── Convenience: find section by name ───────────────────────
    [[nodiscard]] std::optional<section_ref<Traits>> find_section(std::string_view name) const noexcept {
        for (auto sec : sections()) {
            if (sec.name() == name) return sec;
        }
        return std::nullopt;
    }

    // ── Convenience: symbol range from a symbol section ─────────
    [[nodiscard]] symbol_range<Traits> symbols(const section_ref<Traits>& symtab_sec) const noexcept {
        auto sec_data = symtab_sec.data();
        auto ent_size = symtab_sec.entry_size();
        if (ent_size == 0) ent_size = sizeof(typename Traits::sym_type);
        std::size_t count = sec_data.empty() ? 0 : sec_data.size() / ent_size;

        // Get linked string table
        auto link_idx = symtab_sec.link();
        string_table_view strtab;
        if (link_idx < shnum_) {
            auto linked = sections()[static_cast<uint16_t>(link_idx)];
            auto linked_data = linked.data();
            if (!linked_data.empty())
                strtab = string_table_view{linked_data};
        }

        // We need strtab_ to live somewhere stable — use the section data directly
        // For symbols, we store a local strtab and pass pointer to it.
        // Since this is a value-type range, we embed the strtab in the range.
        return {data_,
                static_cast<std::size_t>(symtab_sec.offset()),
                count, nullptr, &conv_};
    }

    // ── Convenience: symbol range with string table ─────────────
    [[nodiscard]] symbol_range<Traits> symbols_with_strtab(
            const section_ref<Traits>& symtab_sec,
            const string_table_view& strtab) const noexcept {
        auto sec_data = symtab_sec.data();
        auto ent_size = symtab_sec.entry_size();
        if (ent_size == 0) ent_size = sizeof(typename Traits::sym_type);
        std::size_t count = sec_data.empty() ? 0 : sec_data.size() / ent_size;
        return {data_,
                static_cast<std::size_t>(symtab_sec.offset()),
                count, &strtab, &conv_};
    }

    // ── Convenience: relocation ranges ──────────────────────────
    [[nodiscard]] rel_range<Traits> rels(const section_ref<Traits>& sec) const noexcept {
        auto d = sec.data();
        std::size_t count = d.empty() ? 0 : d.size() / sizeof(typename Traits::rel_type);
        return {data_, static_cast<std::size_t>(sec.offset()), count, &conv_};
    }

    [[nodiscard]] rela_range<Traits> relas(const section_ref<Traits>& sec) const noexcept {
        auto d = sec.data();
        std::size_t count = d.empty() ? 0 : d.size() / sizeof(typename Traits::rela_type);
        return {data_, static_cast<std::size_t>(sec.offset()), count, &conv_};
    }

    // ── Convenience: dynamic range ──────────────────────────────
    [[nodiscard]] dynamic_range<Traits> dynamics(
            const section_ref<Traits>& sec,
            const string_table_view& strtab) const noexcept {
        auto d = sec.data();
        std::size_t count = d.empty() ? 0 : d.size() / sizeof(typename Traits::dyn_type);
        return {data_, static_cast<std::size_t>(sec.offset()),
                count, &strtab, &conv_};
    }

    // ── Convenience: note range ─────────────────────────────────
    [[nodiscard]] note_range notes(const section_ref<Traits>& sec) const noexcept {
        return {sec.data(), &conv_};
    }

    [[nodiscard]] note_range notes(const segment_ref<Traits>& seg) const noexcept {
        return {seg.data(), &conv_};
    }
};

// ════════════════════════════════════════════════════════════════
//  Auto-detection: detect arch from raw bytes
// ════════════════════════════════════════════════════════════════

[[nodiscard]] inline result<detected_arch> detect_elf_arch(byte_view data) noexcept {
    if (data.size() < EI_NIDENT) return error_code::file_too_small;

    auto ident = data.data();
    if (static_cast<unsigned char>(ident[EI_MAG0]) != ELFMAG0 ||
        static_cast<unsigned char>(ident[EI_MAG1]) != ELFMAG1 ||
        static_cast<unsigned char>(ident[EI_MAG2]) != ELFMAG2 ||
        static_cast<unsigned char>(ident[EI_MAG3]) != ELFMAG3)
        return error_code::invalid_magic;

    auto file_class = static_cast<unsigned char>(ident[EI_CLASS]);
    auto encoding   = static_cast<unsigned char>(ident[EI_DATA]);

    if (file_class == ELFCLASS32 && encoding == ELFDATA2LSB) return detected_arch::elf32_lsb;
    if (file_class == ELFCLASS32 && encoding == ELFDATA2MSB) return detected_arch::elf32_msb;
    if (file_class == ELFCLASS64 && encoding == ELFDATA2LSB) return detected_arch::elf64_lsb;
    if (file_class == ELFCLASS64 && encoding == ELFDATA2MSB) return detected_arch::elf64_msb;

    return error_code::unsupported_class;
}

/// Auto-load: returns a variant of elf_file<elf32_traits> or elf_file<elf64_traits>.
using elf_file_variant = std::variant<elf_file<elf32_traits>, elf_file<elf64_traits>>;

[[nodiscard]] inline result<elf_file_variant> auto_load(byte_view data) noexcept {
    auto arch = detect_elf_arch(data);
    if (!arch) return arch.error();

    switch (*arch) {
    case detected_arch::elf32_lsb:
    case detected_arch::elf32_msb: {
        auto f = elf_file<elf32_traits>::from_view(data);
        if (!f) return f.error();
        return elf_file_variant{std::move(*f)};
    }
    case detected_arch::elf64_lsb:
    case detected_arch::elf64_msb: {
        auto f = elf_file<elf64_traits>::from_view(data);
        if (!f) return f.error();
        return elf_file_variant{std::move(*f)};
    }
    case detected_arch::unknown:
        break;
    }
    return error_code::unsupported_class;
}

} // namespace elfio
