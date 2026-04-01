#pragma once

/// elf_editor<Traits> — the mutable write API.
///
/// Usage:
///   elf_editor<elf64_traits> ed;
///   ed.create(ELFDATA2LSB, ET_EXEC, EM_X86_64);
///   auto& text = ed.add_section(".text", SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR);
///   text.set_data(code, sizeof(code));
///   auto result = ed.save();

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <elfio/core/byte_view.hpp>
#include <elfio/core/endian.hpp>
#include <elfio/core/result.hpp>
#include <elfio/core/safe_math.hpp>
#include <elfio/platform/schema.hpp>
#include <elfio/platform/traits.hpp>
#include <elfio/editor/string_table_builder.hpp>
#include <elfio/editor/section_entry.hpp>
#include <elfio/editor/segment_entry.hpp>
#include <elfio/editor/symbol_builder.hpp>
#include <elfio/editor/layout.hpp>

namespace elfio {

template <typename Traits>
class elf_editor {
    endian_convertor conv_;

    // Header fields
    unsigned char encoding_    = ELFDATA2LSB;
    unsigned char os_abi_      = ELFOSABI_NONE;
    unsigned char abi_version_ = 0;
    Elf_Half      type_        = ET_EXEC;
    Elf_Half      machine_     = EM_NONE;
    Elf_Word      flags_       = 0;
    uint64_t      entry_       = 0;

    // Sections and segments
    std::vector<section_entry<Traits>> sections_;
    std::vector<segment_entry<Traits>> segments_;
    string_table_builder               shstrtab_;

public:
    using traits_type = Traits;

    elf_editor() {
        // Add the null section at index 0 (required by ELF)
        sections_.emplace_back();
    }

    /// Initialize a new ELF file from scratch.
    void create(unsigned char encoding, Elf_Half type, Elf_Half machine) {
        encoding_ = encoding;
        type_     = type;
        machine_  = machine;
        conv_.setup(encoding == ELFDATA2LSB ? byte_order::little : byte_order::big);
    }

    // ── Header setters ──────────────────────────────────────────
    void set_os_abi(unsigned char v)      { os_abi_ = v; }
    void set_abi_version(unsigned char v)  { abi_version_ = v; }
    void set_type(Elf_Half t)             { type_ = t; }
    void set_machine(Elf_Half m)          { machine_ = m; }
    void set_flags(Elf_Word f)            { flags_ = f; }
    void set_entry(uint64_t e)            { entry_ = e; }
    void set_encoding(unsigned char e) {
        encoding_ = e;
        conv_.setup(e == ELFDATA2LSB ? byte_order::little : byte_order::big);
    }

    // ── Header getters ──────────────────────────────────────────
    [[nodiscard]] unsigned char encoding()    const noexcept { return encoding_; }
    [[nodiscard]] unsigned char os_abi()      const noexcept { return os_abi_; }
    [[nodiscard]] unsigned char abi_version() const noexcept { return abi_version_; }
    [[nodiscard]] Elf_Half      type()        const noexcept { return type_; }
    [[nodiscard]] Elf_Half      machine()     const noexcept { return machine_; }
    [[nodiscard]] Elf_Word      flags()       const noexcept { return flags_; }
    [[nodiscard]] uint64_t      entry()       const noexcept { return entry_; }

    // ── Section management ──────────────────────────────────────
    [[nodiscard]] std::vector<section_entry<Traits>>& sections() noexcept { return sections_; }
    [[nodiscard]] const std::vector<section_entry<Traits>>& sections() const noexcept { return sections_; }

    section_entry<Traits>& add_section(std::string name, Elf_Word type,
                                        uint64_t flags = 0) {
        sections_.emplace_back(std::move(name), type, flags);
        return sections_.back();
    }

    std::optional<std::size_t> find_section_index(std::string_view name) const noexcept {
        for (std::size_t i = 0; i < sections_.size(); ++i) {
            if (sections_[i].name() == name) return i;
        }
        return std::nullopt;
    }

    // ── Segment management ──────────────────────────────────────
    [[nodiscard]] std::vector<segment_entry<Traits>>& segments() noexcept { return segments_; }
    [[nodiscard]] const std::vector<segment_entry<Traits>>& segments() const noexcept { return segments_; }

    segment_entry<Traits>& add_segment(Elf_Word type, Elf_Word flags = 0) {
        segments_.emplace_back(type, flags);
        return segments_.back();
    }

    // ── Save to binary ──────────────────────────────────────────
    [[nodiscard]] result<std::vector<char>> save() {
        // Build section name string table
        shstrtab_.clear();
        for (auto& sec : sections_) {
            if (!sec.name().empty()) {
                sec.set_name_index(shstrtab_.add(sec.name()));
            }
        }

        // Find or create the .shstrtab section
        std::optional<std::size_t> shstrtab_idx;
        for (std::size_t i = 0; i < sections_.size(); ++i) {
            if (sections_[i].name() == ".shstrtab") {
                shstrtab_idx = i;
                break;
            }
        }
        if (!shstrtab_idx) {
            auto& sec = add_section(".shstrtab", SHT_STRTAB);
            sec.set_name_index(shstrtab_.add(".shstrtab"));
            shstrtab_idx = sections_.size() - 1;
        }
        // Update .shstrtab data
        sections_[*shstrtab_idx].set_data(shstrtab_.data());

        // Bounds checks: ELF headers use uint16_t for counts
        if (sections_.size() > 0xFFFF)
            return error_code::section_limit_exceeded;
        if (segments_.size() > 0xFFFF)
            return error_code::segment_limit_exceeded;

        // Assign indices
        for (std::size_t i = 0; i < segments_.size(); ++i)
            segments_[i].set_index(static_cast<uint16_t>(i));

        // Compute layout
        auto lr = compute_layout<Traits>(sections_, segments_);
        if (!lr) return lr.error();

        // Allocate output buffer
        std::vector<char> buf(static_cast<std::size_t>(lr->total_size), '\0');

        // Write ELF header
        write_header(buf, *lr, static_cast<Elf_Half>(*shstrtab_idx));

        // Write program headers
        if (!segments_.empty()) {
            write_segments(buf, lr->phoff);
        }

        // Write section data
        for (const auto& sec : sections_) {
            if (sec.data_size() > 0 && sec.offset() > 0) {
                std::memcpy(buf.data() + sec.offset(),
                            sec.data().data(), sec.data_size());
            }
        }

        // Write section headers
        write_section_headers(buf, lr->shoff);

        return buf;
    }

private:
    void write_header(std::vector<char>& buf,
                      const layout_result<Traits>& lr,
                      Elf_Half shstrndx) {
        using ehdr_t = typename Traits::ehdr_type;
        ehdr_t ehdr{};

        // Identification
        ehdr.e_ident[EI_MAG0]       = ELFMAG0;
        ehdr.e_ident[EI_MAG1]       = ELFMAG1;
        ehdr.e_ident[EI_MAG2]       = ELFMAG2;
        ehdr.e_ident[EI_MAG3]       = ELFMAG3;
        ehdr.e_ident[EI_CLASS]      = Traits::file_class;
        ehdr.e_ident[EI_DATA]       = encoding_;
        ehdr.e_ident[EI_VERSION]    = EV_CURRENT;
        ehdr.e_ident[EI_OSABI]      = os_abi_;
        ehdr.e_ident[EI_ABIVERSION] = abi_version_;

        ehdr.e_type      = conv_(type_);
        ehdr.e_machine   = conv_(machine_);
        ehdr.e_version   = conv_(static_cast<Elf_Word>(EV_CURRENT));
        ehdr.e_flags     = conv_(flags_);
        ehdr.e_ehsize    = conv_(static_cast<Elf_Half>(sizeof(ehdr_t)));
        ehdr.e_shstrndx  = conv_(shstrndx);

        // Entry, offsets, counts — need type cast for 32 vs 64 bit
        using addr_t = typename Traits::address_type;
        using off_t  = typename Traits::offset_type;
        ehdr.e_entry     = conv_(static_cast<addr_t>(entry_));
        ehdr.e_phoff     = conv_(static_cast<off_t>(lr.phoff));
        ehdr.e_shoff     = conv_(static_cast<off_t>(lr.shoff));

        ehdr.e_phentsize = conv_(static_cast<Elf_Half>(
            segments_.empty() ? 0 : sizeof(typename Traits::phdr_type)));
        ehdr.e_phnum     = conv_(static_cast<Elf_Half>(segments_.size()));
        ehdr.e_shentsize = conv_(static_cast<Elf_Half>(sizeof(typename Traits::shdr_type)));
        ehdr.e_shnum     = conv_(static_cast<Elf_Half>(sections_.size()));

        std::memcpy(buf.data(), &ehdr, sizeof(ehdr_t));
    }

    void write_segments(std::vector<char>& buf, uint64_t phoff) {
        using phdr_t = typename Traits::phdr_type;
        using off_t  = typename Traits::offset_type;
        using addr_t = typename Traits::address_type;

        for (std::size_t i = 0; i < segments_.size(); ++i) {
            const auto& seg = segments_[i];
            phdr_t phdr{};

            phdr.p_type   = conv_(seg.type());
            phdr.p_flags  = conv_(seg.flags());
            phdr.p_offset = conv_(static_cast<off_t>(seg.offset()));
            phdr.p_vaddr  = conv_(static_cast<addr_t>(seg.vaddr()));
            phdr.p_paddr  = conv_(static_cast<addr_t>(seg.paddr()));

            if constexpr (std::is_same_v<Traits, elf32_traits>) {
                phdr.p_filesz = conv_(static_cast<Elf_Word>(seg.filesz()));
                phdr.p_memsz  = conv_(static_cast<Elf_Word>(seg.memsz()));
                phdr.p_align  = conv_(static_cast<Elf_Word>(seg.align()));
            } else {
                phdr.p_filesz = conv_(static_cast<Elf_Xword>(seg.filesz()));
                phdr.p_memsz  = conv_(static_cast<Elf_Xword>(seg.memsz()));
                phdr.p_align  = conv_(static_cast<Elf_Xword>(seg.align()));
            }

            std::memcpy(buf.data() + phoff + i * sizeof(phdr_t),
                        &phdr, sizeof(phdr_t));
        }
    }

    void write_section_headers(std::vector<char>& buf, uint64_t shoff) {
        using shdr_t = typename Traits::shdr_type;

        for (std::size_t i = 0; i < sections_.size(); ++i) {
            const auto& sec = sections_[i];
            shdr_t shdr{};

            shdr.sh_name = conv_(sec.name_index());

            if constexpr (std::is_same_v<Traits, elf32_traits>) {
                shdr.sh_type      = conv_(sec.type());
                shdr.sh_flags     = conv_(static_cast<Elf_Word>(sec.flags()));
                shdr.sh_addr      = conv_(static_cast<Elf32_Addr>(sec.address()));
                shdr.sh_offset    = conv_(static_cast<Elf32_Off>(sec.offset()));
                shdr.sh_size      = conv_(static_cast<Elf_Word>(sec.size()));
                shdr.sh_link      = conv_(sec.link());
                shdr.sh_info      = conv_(sec.info());
                shdr.sh_addralign = conv_(static_cast<Elf_Word>(sec.addr_align()));
                shdr.sh_entsize   = conv_(static_cast<Elf_Word>(sec.entry_size()));
            } else {
                shdr.sh_type      = conv_(sec.type());
                shdr.sh_flags     = conv_(static_cast<Elf_Xword>(sec.flags()));
                shdr.sh_addr      = conv_(static_cast<Elf64_Addr>(sec.address()));
                shdr.sh_offset    = conv_(static_cast<Elf64_Off>(sec.offset()));
                shdr.sh_size      = conv_(static_cast<Elf_Xword>(sec.size()));
                shdr.sh_link      = conv_(sec.link());
                shdr.sh_info      = conv_(sec.info());
                shdr.sh_addralign = conv_(static_cast<Elf_Xword>(sec.addr_align()));
                shdr.sh_entsize   = conv_(static_cast<Elf_Xword>(sec.entry_size()));
            }

            std::memcpy(buf.data() + shoff + i * sizeof(shdr_t),
                        &shdr, sizeof(shdr_t));
        }
    }
};

} // namespace elfio
