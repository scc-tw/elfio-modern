#pragma once

#include <cstdint>
#include <elfio/platform/schema.hpp>

namespace elfio {

/// Traits for 32-bit ELF files.
struct elf32_traits {
    using ehdr_type    = Elf32_Ehdr;
    using shdr_type    = Elf32_Shdr;
    using phdr_type    = Elf32_Phdr;
    using sym_type     = Elf32_Sym;
    using rel_type     = Elf32_Rel;
    using rela_type    = Elf32_Rela;
    using dyn_type     = Elf32_Dyn;
    using chdr_type    = Elf32_Chdr;
    using address_type = Elf32_Addr;  // uint32_t
    using offset_type  = Elf32_Off;   // uint32_t
    using xword_type   = Elf_Word;    // uint32_t  (sizes/flags are 32-bit)
    using sxword_type  = Elf_Sword;   // int32_t

    static constexpr unsigned char file_class = ELFCLASS32;
    static constexpr std::size_t   addr_size  = 4;

    // Relocation info helpers
    static constexpr Elf_Word r_sym(Elf_Word info)  noexcept { return elf32_r_sym(info); }
    static constexpr Elf_Word r_type(Elf_Word info) noexcept { return elf32_r_type(info); }
    static constexpr Elf_Word r_info(Elf_Word s, Elf_Word t) noexcept {
        return elf32_r_info(s, t);
    }
};

/// Traits for 64-bit ELF files.
struct elf64_traits {
    using ehdr_type    = Elf64_Ehdr;
    using shdr_type    = Elf64_Shdr;
    using phdr_type    = Elf64_Phdr;
    using sym_type     = Elf64_Sym;
    using rel_type     = Elf64_Rel;
    using rela_type    = Elf64_Rela;
    using dyn_type     = Elf64_Dyn;
    using chdr_type    = Elf64_Chdr;
    using address_type = Elf64_Addr;  // uint64_t
    using offset_type  = Elf64_Off;   // uint64_t
    using xword_type   = Elf_Xword;   // uint64_t (sizes/flags are 64-bit)
    using sxword_type  = Elf_Sxword;  // int64_t

    static constexpr unsigned char file_class = ELFCLASS64;
    static constexpr std::size_t   addr_size  = 8;

    // Relocation info helpers
    static constexpr Elf_Xword r_sym(Elf_Xword info)  noexcept { return elf64_r_sym(info); }
    static constexpr Elf_Xword r_type(Elf_Xword info) noexcept { return elf64_r_type(info); }
    static constexpr Elf_Xword r_info(Elf_Xword s, Elf_Xword t) noexcept {
        return elf64_r_info(s, t);
    }
};

/// Detected architecture after inspecting e_ident.
enum class detected_arch {
    unknown,
    elf32_lsb,
    elf32_msb,
    elf64_lsb,
    elf64_msb,
};

} // namespace elfio
