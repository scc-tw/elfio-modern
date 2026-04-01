#pragma once

/// ELF dump/formatting utilities.
/// Converts ELF constants to human-readable strings.

#include <cstdint>
#include <string>
#include <string_view>
#include <elfio/platform/schema.hpp>

namespace elfio {

// ── String formatting for ELF constants ─────────────────────────

[[nodiscard]] inline std::string_view str_file_type(Elf_Half type) noexcept {
    switch (type) {
        case ET_NONE: return "NONE";
        case ET_REL:  return "REL (Relocatable)";
        case ET_EXEC: return "EXEC (Executable)";
        case ET_DYN:  return "DYN (Shared object)";
        case ET_CORE: return "CORE (Core file)";
        default:      return "UNKNOWN";
    }
}

[[nodiscard]] inline std::string_view str_class(unsigned char cls) noexcept {
    switch (cls) {
        case ELFCLASSNONE: return "NONE";
        case ELFCLASS32:   return "ELF32";
        case ELFCLASS64:   return "ELF64";
        default:           return "UNKNOWN";
    }
}

[[nodiscard]] inline std::string_view str_encoding(unsigned char enc) noexcept {
    switch (enc) {
        case ELFDATANONE: return "NONE";
        case ELFDATA2LSB: return "2's complement, little endian";
        case ELFDATA2MSB: return "2's complement, big endian";
        default:          return "UNKNOWN";
    }
}

[[nodiscard]] inline std::string_view str_os_abi(unsigned char abi) noexcept {
    switch (abi) {
        case ELFOSABI_NONE:       return "UNIX System V";
        case ELFOSABI_HPUX:       return "HP-UX";
        case ELFOSABI_NETBSD:     return "NetBSD";
        case ELFOSABI_LINUX:      return "Linux";
        case ELFOSABI_SOLARIS:    return "Solaris";
        case ELFOSABI_AIX:        return "AIX";
        case ELFOSABI_IRIX:       return "IRIX";
        case ELFOSABI_FREEBSD:    return "FreeBSD";
        case ELFOSABI_OPENBSD:    return "OpenBSD";
        case ELFOSABI_ARM:        return "ARM";
        case ELFOSABI_STANDALONE: return "Standalone";
        default:                  return "UNKNOWN";
    }
}

[[nodiscard]] inline std::string_view str_machine(Elf_Half machine) noexcept {
    switch (machine) {
        case EM_NONE:      return "None";
        case EM_386:       return "Intel 80386";
        case EM_ARM:       return "ARM";
        case EM_X86_64:    return "AMD x86-64";
        case EM_AARCH64:   return "AArch64";
        case EM_RISCV:     return "RISC-V";
        case EM_PPC:       return "PowerPC";
        case EM_PPC64:     return "PowerPC64";
        case EM_MIPS:      return "MIPS";
        case EM_S390:      return "IBM S/390";
        case EM_SPARC:     return "SPARC";
        case EM_SPARCV9:   return "SPARC v9";
        case EM_IA_64:     return "Intel IA-64";
        case EM_BPF:       return "Linux BPF";
        case EM_LOONGARCH: return "LoongArch";
        default:           return "UNKNOWN";
    }
}

[[nodiscard]] inline std::string_view str_section_type(Elf_Word type) noexcept {
    switch (type) {
        case SHT_NULL:          return "NULL";
        case SHT_PROGBITS:      return "PROGBITS";
        case SHT_SYMTAB:        return "SYMTAB";
        case SHT_STRTAB:        return "STRTAB";
        case SHT_RELA:          return "RELA";
        case SHT_HASH:          return "HASH";
        case SHT_DYNAMIC:       return "DYNAMIC";
        case SHT_NOTE:          return "NOTE";
        case SHT_NOBITS:        return "NOBITS";
        case SHT_REL:           return "REL";
        case SHT_DYNSYM:        return "DYNSYM";
        case SHT_INIT_ARRAY:    return "INIT_ARRAY";
        case SHT_FINI_ARRAY:    return "FINI_ARRAY";
        case SHT_PREINIT_ARRAY: return "PREINIT_ARRAY";
        case SHT_GROUP:         return "GROUP";
        case SHT_GNU_HASH:      return "GNU_HASH";
        case SHT_GNU_verdef:    return "VERDEF";
        case SHT_GNU_verneed:   return "VERNEED";
        case SHT_GNU_versym:    return "VERSYM";
        default:                return "UNKNOWN";
    }
}

[[nodiscard]] inline std::string_view str_segment_type(Elf_Word type) noexcept {
    switch (type) {
        case PT_NULL:         return "NULL";
        case PT_LOAD:         return "LOAD";
        case PT_DYNAMIC:      return "DYNAMIC";
        case PT_INTERP:       return "INTERP";
        case PT_NOTE:         return "NOTE";
        case PT_SHLIB:        return "SHLIB";
        case PT_PHDR:         return "PHDR";
        case PT_TLS:          return "TLS";
        case PT_GNU_EH_FRAME: return "GNU_EH_FRAME";
        case PT_GNU_STACK:    return "GNU_STACK";
        case PT_GNU_RELRO:    return "GNU_RELRO";
        case PT_GNU_PROPERTY: return "GNU_PROPERTY";
        default:              return "UNKNOWN";
    }
}

[[nodiscard]] inline std::string str_section_flags(uint64_t flags) {
    std::string s;
    if (flags & SHF_WRITE)      s += 'W';
    if (flags & SHF_ALLOC)      s += 'A';
    if (flags & SHF_EXECINSTR)  s += 'X';
    if (flags & SHF_MERGE)      s += 'M';
    if (flags & SHF_STRINGS)    s += 'S';
    if (flags & SHF_INFO_LINK)  s += 'I';
    if (flags & SHF_LINK_ORDER) s += 'L';
    if (flags & SHF_GROUP)      s += 'G';
    if (flags & SHF_TLS)        s += 'T';
    if (flags & SHF_COMPRESSED) s += 'C';
    return s;
}

[[nodiscard]] inline std::string str_segment_flags(Elf_Word flags) {
    std::string s;
    if (flags & PF_R) s += 'R';
    if (flags & PF_W) s += 'W';
    if (flags & PF_X) s += 'E';
    return s;
}

[[nodiscard]] inline std::string_view str_symbol_bind(unsigned char bind) noexcept {
    switch (bind) {
        case STB_LOCAL:  return "LOCAL";
        case STB_GLOBAL: return "GLOBAL";
        case STB_WEAK:   return "WEAK";
        default:         return "UNKNOWN";
    }
}

[[nodiscard]] inline std::string_view str_symbol_type(unsigned char type) noexcept {
    switch (type) {
        case STT_NOTYPE:  return "NOTYPE";
        case STT_OBJECT:  return "OBJECT";
        case STT_FUNC:    return "FUNC";
        case STT_SECTION: return "SECTION";
        case STT_FILE:    return "FILE";
        case STT_COMMON:  return "COMMON";
        case STT_TLS:     return "TLS";
        default:          return "UNKNOWN";
    }
}

[[nodiscard]] inline std::string_view str_symbol_visibility(unsigned char vis) noexcept {
    switch (vis) {
        case STV_DEFAULT:   return "DEFAULT";
        case STV_INTERNAL:  return "INTERNAL";
        case STV_HIDDEN:    return "HIDDEN";
        case STV_PROTECTED: return "PROTECTED";
        default:            return "UNKNOWN";
    }
}

/// Get default entry size for standard section types (same as original ELFIO).
template <typename Traits>
[[nodiscard]] constexpr uint64_t default_entry_size(Elf_Word section_type) noexcept {
    switch (section_type) {
        case SHT_RELA:    return sizeof(typename Traits::rela_type);
        case SHT_REL:     return sizeof(typename Traits::rel_type);
        case SHT_SYMTAB:
        case SHT_DYNSYM:  return sizeof(typename Traits::sym_type);
        case SHT_DYNAMIC: return sizeof(typename Traits::dyn_type);
        default:          return 0;
    }
}

} // namespace elfio
