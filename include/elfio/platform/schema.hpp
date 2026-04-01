#pragma once

/// Packed binary structures and constants for the ELF format.
/// All structs are trivially copyable and read via memcpy (no alignment UB).

#include <cstdint>

#pragma pack(push, 1)

namespace elfio {

// ── Type aliases ────────────────────────────────────────────────
using Elf_Half   = std::uint16_t;
using Elf_Word   = std::uint32_t;
using Elf_Sword  = std::int32_t;
using Elf_Xword  = std::uint64_t;
using Elf_Sxword = std::int64_t;

using Elf32_Addr = std::uint32_t;
using Elf32_Off  = std::uint32_t;
using Elf64_Addr = std::uint64_t;
using Elf64_Off  = std::uint64_t;

using Elf32_Word = Elf_Word;
using Elf64_Word = Elf_Word;

// ── File type (e_type) ──────────────────────────────────────────
constexpr Elf_Half ET_NONE   = 0;
constexpr Elf_Half ET_REL    = 1;
constexpr Elf_Half ET_EXEC   = 2;
constexpr Elf_Half ET_DYN    = 3;
constexpr Elf_Half ET_CORE   = 4;
constexpr Elf_Half ET_LOOS   = 0xFE00;
constexpr Elf_Half ET_HIOS   = 0xFEFF;
constexpr Elf_Half ET_LOPROC = 0xFF00;
constexpr Elf_Half ET_HIPROC = 0xFFFF;

// ── Machine architectures (e_machine) ──────────────────────────
constexpr Elf_Half EM_NONE        = 0;
constexpr Elf_Half EM_M32         = 1;
constexpr Elf_Half EM_SPARC       = 2;
constexpr Elf_Half EM_386         = 3;
constexpr Elf_Half EM_68K         = 4;
constexpr Elf_Half EM_88K         = 5;
constexpr Elf_Half EM_860         = 7;
constexpr Elf_Half EM_MIPS        = 8;
constexpr Elf_Half EM_S370        = 9;
constexpr Elf_Half EM_PARISC      = 15;
constexpr Elf_Half EM_SPARC32PLUS = 18;
constexpr Elf_Half EM_PPC         = 20;
constexpr Elf_Half EM_PPC64       = 21;
constexpr Elf_Half EM_S390        = 22;
constexpr Elf_Half EM_ARM         = 40;
constexpr Elf_Half EM_SH          = 42;
constexpr Elf_Half EM_SPARCV9     = 43;
constexpr Elf_Half EM_IA_64       = 50;
constexpr Elf_Half EM_X86_64      = 62;
constexpr Elf_Half EM_AARCH64     = 183;
constexpr Elf_Half EM_RISCV       = 243;
constexpr Elf_Half EM_BPF         = 247;
constexpr Elf_Half EM_LOONGARCH   = 258;

// ── File version ────────────────────────────────────────────────
constexpr unsigned char EV_NONE    = 0;
constexpr unsigned char EV_CURRENT = 1;

// ── Identification indices ──────────────────────────────────────
constexpr unsigned char EI_MAG0       = 0;
constexpr unsigned char EI_MAG1       = 1;
constexpr unsigned char EI_MAG2       = 2;
constexpr unsigned char EI_MAG3       = 3;
constexpr unsigned char EI_CLASS      = 4;
constexpr unsigned char EI_DATA       = 5;
constexpr unsigned char EI_VERSION    = 6;
constexpr unsigned char EI_OSABI      = 7;
constexpr unsigned char EI_ABIVERSION = 8;
constexpr unsigned char EI_PAD        = 9;
constexpr unsigned char EI_NIDENT     = 16;

// ── Magic number ────────────────────────────────────────────────
constexpr unsigned char ELFMAG0 = 0x7F;
constexpr unsigned char ELFMAG1 = 'E';
constexpr unsigned char ELFMAG2 = 'L';
constexpr unsigned char ELFMAG3 = 'F';

// ── File class ──────────────────────────────────────────────────
constexpr unsigned char ELFCLASSNONE = 0;
constexpr unsigned char ELFCLASS32   = 1;
constexpr unsigned char ELFCLASS64   = 2;

// ── Data encoding ───────────────────────────────────────────────
constexpr unsigned char ELFDATANONE = 0;
constexpr unsigned char ELFDATA2LSB = 1;
constexpr unsigned char ELFDATA2MSB = 2;

// ── OS/ABI ──────────────────────────────────────────────────────
constexpr unsigned char ELFOSABI_NONE       = 0;
constexpr unsigned char ELFOSABI_HPUX       = 1;
constexpr unsigned char ELFOSABI_NETBSD     = 2;
constexpr unsigned char ELFOSABI_LINUX      = 3;
constexpr unsigned char ELFOSABI_HURD       = 4;
constexpr unsigned char ELFOSABI_SOLARIS    = 6;
constexpr unsigned char ELFOSABI_AIX        = 7;
constexpr unsigned char ELFOSABI_IRIX       = 8;
constexpr unsigned char ELFOSABI_FREEBSD    = 9;
constexpr unsigned char ELFOSABI_TRU64      = 10;
constexpr unsigned char ELFOSABI_MODESTO    = 11;
constexpr unsigned char ELFOSABI_OPENBSD    = 12;
constexpr unsigned char ELFOSABI_ARM        = 97;
constexpr unsigned char ELFOSABI_STANDALONE = 255;

// ── Section indices ─────────────────────────────────────────────
constexpr Elf_Word SHN_UNDEF     = 0;
constexpr Elf_Word SHN_LORESERVE = 0xFF00;
constexpr Elf_Word SHN_LOPROC    = 0xFF00;
constexpr Elf_Word SHN_HIPROC    = 0xFF1F;
constexpr Elf_Word SHN_LOOS      = 0xFF20;
constexpr Elf_Word SHN_HIOS      = 0xFF3F;
constexpr Elf_Word SHN_ABS       = 0xFFF1;
constexpr Elf_Word SHN_COMMON    = 0xFFF2;
constexpr Elf_Word SHN_XINDEX    = 0xFFFF;
constexpr Elf_Word SHN_HIRESERVE = 0xFFFF;

// ── Section types (sh_type) ─────────────────────────────────────
constexpr Elf_Word SHT_NULL           = 0;
constexpr Elf_Word SHT_PROGBITS       = 1;
constexpr Elf_Word SHT_SYMTAB         = 2;
constexpr Elf_Word SHT_STRTAB         = 3;
constexpr Elf_Word SHT_RELA           = 4;
constexpr Elf_Word SHT_HASH           = 5;
constexpr Elf_Word SHT_DYNAMIC        = 6;
constexpr Elf_Word SHT_NOTE           = 7;
constexpr Elf_Word SHT_NOBITS         = 8;
constexpr Elf_Word SHT_REL            = 9;
constexpr Elf_Word SHT_SHLIB          = 10;
constexpr Elf_Word SHT_DYNSYM         = 11;
constexpr Elf_Word SHT_INIT_ARRAY     = 14;
constexpr Elf_Word SHT_FINI_ARRAY     = 15;
constexpr Elf_Word SHT_PREINIT_ARRAY  = 16;
constexpr Elf_Word SHT_GROUP          = 17;
constexpr Elf_Word SHT_SYMTAB_SHNDX   = 18;
constexpr Elf_Word SHT_GNU_ATTRIBUTES = 0x6ffffff5;
constexpr Elf_Word SHT_GNU_HASH       = 0x6ffffff6;
constexpr Elf_Word SHT_GNU_LIBLIST    = 0x6ffffff7;
constexpr Elf_Word SHT_GNU_verdef     = 0x6ffffffd;
constexpr Elf_Word SHT_GNU_verneed    = 0x6ffffffe;
constexpr Elf_Word SHT_GNU_versym     = 0x6fffffff;
constexpr Elf_Word SHT_LOOS           = 0x60000000;
constexpr Elf_Word SHT_HIOS           = 0x6fffffff;
constexpr Elf_Word SHT_LOPROC         = 0x70000000;
constexpr Elf_Word SHT_HIPROC         = 0x7FFFFFFF;
constexpr Elf_Word SHT_LOUSER         = 0x80000000;
constexpr Elf_Word SHT_HIUSER         = 0xFFFFFFFF;

// ── Section flags (sh_flags) ────────────────────────────────────
constexpr Elf_Xword SHF_WRITE            = 0x1;
constexpr Elf_Xword SHF_ALLOC            = 0x2;
constexpr Elf_Xword SHF_EXECINSTR        = 0x4;
constexpr Elf_Xword SHF_MERGE            = 0x10;
constexpr Elf_Xword SHF_STRINGS          = 0x20;
constexpr Elf_Xword SHF_INFO_LINK        = 0x40;
constexpr Elf_Xword SHF_LINK_ORDER       = 0x80;
constexpr Elf_Xword SHF_OS_NONCONFORMING = 0x100;
constexpr Elf_Xword SHF_GROUP            = 0x200;
constexpr Elf_Xword SHF_TLS              = 0x400;
constexpr Elf_Xword SHF_COMPRESSED       = 0x800;
constexpr Elf_Xword SHF_MASKOS           = 0x0FF00000;
constexpr Elf_Xword SHF_MASKPROC         = 0xF0000000;

// ── Symbol binding ──────────────────────────────────────────────
constexpr unsigned char STB_LOCAL  = 0;
constexpr unsigned char STB_GLOBAL = 1;
constexpr unsigned char STB_WEAK   = 2;
constexpr unsigned char STB_LOOS   = 10;
constexpr unsigned char STB_HIOS   = 12;
constexpr unsigned char STB_LOPROC = 13;
constexpr unsigned char STB_HIPROC = 15;

// ── Symbol types ────────────────────────────────────────────────
constexpr unsigned char STT_NOTYPE  = 0;
constexpr unsigned char STT_OBJECT  = 1;
constexpr unsigned char STT_FUNC    = 2;
constexpr unsigned char STT_SECTION = 3;
constexpr unsigned char STT_FILE    = 4;
constexpr unsigned char STT_COMMON  = 5;
constexpr unsigned char STT_TLS     = 6;
constexpr unsigned char STT_LOOS    = 10;
constexpr unsigned char STT_HIOS    = 12;
constexpr unsigned char STT_LOPROC  = 13;
constexpr unsigned char STT_HIPROC  = 15;

// ── Symbol visibility ───────────────────────────────────────────
constexpr unsigned char STV_DEFAULT   = 0;
constexpr unsigned char STV_INTERNAL  = 1;
constexpr unsigned char STV_HIDDEN    = 2;
constexpr unsigned char STV_PROTECTED = 3;

constexpr Elf_Word STN_UNDEF = 0;

// ── Symbol info helpers (constexpr replacements for macros) ─────
constexpr unsigned char elf_st_bind(unsigned char i) noexcept { return i >> 4; }
constexpr unsigned char elf_st_type(unsigned char i) noexcept { return i & 0xf; }
constexpr unsigned char elf_st_info(unsigned char b, unsigned char t) noexcept {
    return static_cast<unsigned char>((b << 4) + (t & 0xf));
}
constexpr unsigned char elf_st_visibility(unsigned char o) noexcept { return o & 0x3; }

// ── Relocation info helpers ─────────────────────────────────────
constexpr Elf_Word  elf32_r_sym(Elf_Word i)  noexcept { return i >> 8; }
constexpr Elf_Word  elf32_r_type(Elf_Word i) noexcept { return static_cast<unsigned char>(i); }
constexpr Elf_Word  elf32_r_info(Elf_Word s, Elf_Word t) noexcept {
    return (s << 8) + static_cast<unsigned char>(t);
}
constexpr Elf_Xword elf64_r_sym(Elf_Xword i)  noexcept { return i >> 32; }
constexpr Elf_Xword elf64_r_type(Elf_Xword i) noexcept { return i & 0xffffffffULL; }
constexpr Elf_Xword elf64_r_info(Elf_Xword s, Elf_Xword t) noexcept {
    return (s << 32) + (t & 0xffffffffULL);
}

// ── Segment types (p_type) ──────────────────────────────────────
constexpr Elf_Word PT_NULL         = 0;
constexpr Elf_Word PT_LOAD         = 1;
constexpr Elf_Word PT_DYNAMIC      = 2;
constexpr Elf_Word PT_INTERP       = 3;
constexpr Elf_Word PT_NOTE         = 4;
constexpr Elf_Word PT_SHLIB        = 5;
constexpr Elf_Word PT_PHDR         = 6;
constexpr Elf_Word PT_TLS          = 7;
constexpr Elf_Word PT_LOOS         = 0x60000000;
constexpr Elf_Word PT_GNU_EH_FRAME = 0x6474E550;
constexpr Elf_Word PT_GNU_STACK    = 0x6474E551;
constexpr Elf_Word PT_GNU_RELRO    = 0x6474E552;
constexpr Elf_Word PT_GNU_PROPERTY = 0x6474E553;
constexpr Elf_Word PT_HIOS         = 0x6FFFFFFF;
constexpr Elf_Word PT_LOPROC       = 0x70000000;
constexpr Elf_Word PT_HIPROC       = 0x7FFFFFFF;

// ── Segment flags (p_flags) ─────────────────────────────────────
constexpr Elf_Word PF_X        = 1;
constexpr Elf_Word PF_W        = 2;
constexpr Elf_Word PF_R        = 4;
constexpr Elf_Word PF_MASKOS   = 0x0ff00000;
constexpr Elf_Word PF_MASKPROC = 0xf0000000;

// ── Dynamic tags (d_tag) ────────────────────────────────────────
constexpr Elf_Sword DT_NULL            = 0;
constexpr Elf_Sword DT_NEEDED          = 1;
constexpr Elf_Sword DT_PLTRELSZ        = 2;
constexpr Elf_Sword DT_PLTGOT          = 3;
constexpr Elf_Sword DT_HASH            = 4;
constexpr Elf_Sword DT_STRTAB          = 5;
constexpr Elf_Sword DT_SYMTAB          = 6;
constexpr Elf_Sword DT_RELA            = 7;
constexpr Elf_Sword DT_RELASZ          = 8;
constexpr Elf_Sword DT_RELAENT         = 9;
constexpr Elf_Sword DT_STRSZ           = 10;
constexpr Elf_Sword DT_SYMENT          = 11;
constexpr Elf_Sword DT_INIT            = 12;
constexpr Elf_Sword DT_FINI            = 13;
constexpr Elf_Sword DT_SONAME          = 14;
constexpr Elf_Sword DT_RPATH           = 15;
constexpr Elf_Sword DT_SYMBOLIC        = 16;
constexpr Elf_Sword DT_REL             = 17;
constexpr Elf_Sword DT_RELSZ           = 18;
constexpr Elf_Sword DT_RELENT          = 19;
constexpr Elf_Sword DT_PLTREL          = 20;
constexpr Elf_Sword DT_DEBUG           = 21;
constexpr Elf_Sword DT_TEXTREL         = 22;
constexpr Elf_Sword DT_JMPREL          = 23;
constexpr Elf_Sword DT_BIND_NOW        = 24;
constexpr Elf_Sword DT_INIT_ARRAY      = 25;
constexpr Elf_Sword DT_FINI_ARRAY      = 26;
constexpr Elf_Sword DT_INIT_ARRAYSZ    = 27;
constexpr Elf_Sword DT_FINI_ARRAYSZ    = 28;
constexpr Elf_Sword DT_RUNPATH         = 29;
constexpr Elf_Sword DT_FLAGS           = 30;
constexpr Elf_Sword DT_PREINIT_ARRAY   = 32;
constexpr Elf_Sword DT_PREINIT_ARRAYSZ = 33;

// GNU-specific dynamic tags
constexpr Elf_Word DT_GNU_HASH    = 0x6ffffef5;
constexpr Elf_Word DT_VERSYM      = 0x6ffffff0;
constexpr Elf_Word DT_RELACOUNT   = 0x6ffffff9;
constexpr Elf_Word DT_RELCOUNT    = 0x6ffffffa;
constexpr Elf_Word DT_FLAGS_1     = 0x6ffffffb;
constexpr Elf_Word DT_VERDEF      = 0x6ffffffc;
constexpr Elf_Word DT_VERDEFNUM   = 0x6ffffffd;
constexpr Elf_Word DT_VERNEED     = 0x6ffffffe;
constexpr Elf_Word DT_VERNEEDNUM  = 0x6fffffff;

// ── Note types ──────────────────────────────────────────────────
constexpr Elf_Word NT_PRSTATUS        = 1;
constexpr Elf_Word NT_PRPSINFO        = 3;
constexpr Elf_Word NT_GNU_ABI_TAG     = 1;
constexpr Elf_Word NT_GNU_BUILD_ID    = 3;
constexpr Elf_Word NT_GNU_GOLD_VERSION = 4;
constexpr Elf_Word NT_GNU_PROPERTY_TYPE_0 = 5;

// ── Common relocation types ─────────────────────────────────────
// x86
constexpr unsigned R_386_NONE     = 0;
constexpr unsigned R_386_32       = 1;
constexpr unsigned R_386_PC32     = 2;
constexpr unsigned R_386_GOT32    = 3;
constexpr unsigned R_386_PLT32    = 4;
constexpr unsigned R_386_COPY     = 5;
constexpr unsigned R_386_GLOB_DAT = 6;
constexpr unsigned R_386_JMP_SLOT = 7;
constexpr unsigned R_386_RELATIVE = 8;
// x86_64
constexpr unsigned R_X86_64_NONE      = 0;
constexpr unsigned R_X86_64_64        = 1;
constexpr unsigned R_X86_64_PC32      = 2;
constexpr unsigned R_X86_64_GOT32     = 3;
constexpr unsigned R_X86_64_PLT32     = 4;
constexpr unsigned R_X86_64_COPY      = 5;
constexpr unsigned R_X86_64_GLOB_DAT  = 6;
constexpr unsigned R_X86_64_JUMP_SLOT = 7;
constexpr unsigned R_X86_64_RELATIVE  = 8;
constexpr unsigned R_X86_64_GOTPCREL  = 9;
constexpr unsigned R_X86_64_32        = 10;
constexpr unsigned R_X86_64_32S       = 11;
// AArch64
constexpr unsigned R_AARCH64_NONE      = 0;
constexpr unsigned R_AARCH64_ABS64     = 257;
constexpr unsigned R_AARCH64_ABS32     = 258;
constexpr unsigned R_AARCH64_CALL26    = 283;
constexpr unsigned R_AARCH64_JUMP26    = 282;
constexpr unsigned R_AARCH64_COPY      = 1024;
constexpr unsigned R_AARCH64_GLOB_DAT  = 1025;
constexpr unsigned R_AARCH64_JUMP_SLOT = 1026;
constexpr unsigned R_AARCH64_RELATIVE  = 1027;
// ARM
constexpr unsigned R_ARM_NONE   = 0;
constexpr unsigned R_ARM_ABS32  = 2;
constexpr unsigned R_ARM_REL32  = 3;
constexpr unsigned R_ARM_CALL   = 28;
constexpr unsigned R_ARM_JUMP24 = 29;
// RISC-V
constexpr unsigned R_RISCV_NONE      = 0;
constexpr unsigned R_RISCV_32        = 1;
constexpr unsigned R_RISCV_64        = 2;
constexpr unsigned R_RISCV_RELATIVE  = 3;
constexpr unsigned R_RISCV_JUMP_SLOT = 5;

// ── DT_FLAGS values ─────────────────────────────────────────────
constexpr Elf_Word DF_ORIGIN     = 0x1;
constexpr Elf_Word DF_SYMBOLIC   = 0x2;
constexpr Elf_Word DF_TEXTREL    = 0x4;
constexpr Elf_Word DF_BIND_NOW   = 0x8;
constexpr Elf_Word DF_STATIC_TLS = 0x10;

// ════════════════════════════════════════════════════════════════
//  Binary structures (packed, trivially copyable)
// ════════════════════════════════════════════════════════════════

// ── ELF file header ─────────────────────────────────────────────

struct Elf32_Ehdr {
    unsigned char e_ident[EI_NIDENT];
    Elf_Half      e_type;
    Elf_Half      e_machine;
    Elf_Word      e_version;
    Elf32_Addr    e_entry;
    Elf32_Off     e_phoff;
    Elf32_Off     e_shoff;
    Elf_Word      e_flags;
    Elf_Half      e_ehsize;
    Elf_Half      e_phentsize;
    Elf_Half      e_phnum;
    Elf_Half      e_shentsize;
    Elf_Half      e_shnum;
    Elf_Half      e_shstrndx;
};

struct Elf64_Ehdr {
    unsigned char e_ident[EI_NIDENT];
    Elf_Half      e_type;
    Elf_Half      e_machine;
    Elf_Word      e_version;
    Elf64_Addr    e_entry;
    Elf64_Off     e_phoff;
    Elf64_Off     e_shoff;
    Elf_Word      e_flags;
    Elf_Half      e_ehsize;
    Elf_Half      e_phentsize;
    Elf_Half      e_phnum;
    Elf_Half      e_shentsize;
    Elf_Half      e_shnum;
    Elf_Half      e_shstrndx;
};

// ── Section header ──────────────────────────────────────────────

struct Elf32_Shdr {
    Elf_Word   sh_name;
    Elf_Word   sh_type;
    Elf_Word   sh_flags;
    Elf32_Addr sh_addr;
    Elf32_Off  sh_offset;
    Elf_Word   sh_size;
    Elf_Word   sh_link;
    Elf_Word   sh_info;
    Elf_Word   sh_addralign;
    Elf_Word   sh_entsize;
};

struct Elf64_Shdr {
    Elf_Word   sh_name;
    Elf_Word   sh_type;
    Elf_Xword  sh_flags;
    Elf64_Addr sh_addr;
    Elf64_Off  sh_offset;
    Elf_Xword  sh_size;
    Elf_Word   sh_link;
    Elf_Word   sh_info;
    Elf_Xword  sh_addralign;
    Elf_Xword  sh_entsize;
};

// ── Program (segment) header ────────────────────────────────────

struct Elf32_Phdr {
    Elf_Word   p_type;
    Elf32_Off  p_offset;
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;
    Elf_Word   p_filesz;
    Elf_Word   p_memsz;
    Elf_Word   p_flags;
    Elf_Word   p_align;
};

struct Elf64_Phdr {
    Elf_Word   p_type;
    Elf_Word   p_flags;    // Note: flags field position differs from 32-bit
    Elf64_Off  p_offset;
    Elf64_Addr p_vaddr;
    Elf64_Addr p_paddr;
    Elf_Xword  p_filesz;
    Elf_Xword  p_memsz;
    Elf_Xword  p_align;
};

// ── Symbol table entry ──────────────────────────────────────────

struct Elf32_Sym {
    Elf_Word      st_name;
    Elf32_Addr    st_value;
    Elf_Word      st_size;
    unsigned char st_info;
    unsigned char st_other;
    Elf_Half      st_shndx;
};

struct Elf64_Sym {
    Elf_Word      st_name;
    unsigned char st_info;     // Note: field order differs from 32-bit
    unsigned char st_other;
    Elf_Half      st_shndx;
    Elf64_Addr    st_value;
    Elf_Xword     st_size;
};

// ── Relocation entries ──────────────────────────────────────────

struct Elf32_Rel {
    Elf32_Addr r_offset;
    Elf_Word   r_info;
};

struct Elf32_Rela {
    Elf32_Addr r_offset;
    Elf_Word   r_info;
    Elf_Sword  r_addend;
};

struct Elf64_Rel {
    Elf64_Addr r_offset;
    Elf_Xword  r_info;
};

struct Elf64_Rela {
    Elf64_Addr r_offset;
    Elf_Xword  r_info;
    Elf_Sxword r_addend;
};

// ── Dynamic section entry ───────────────────────────────────────

struct Elf32_Dyn {
    Elf_Sword d_tag;
    union {
        Elf_Word   d_val;
        Elf32_Addr d_ptr;
    } d_un;
};

struct Elf64_Dyn {
    Elf_Sxword d_tag;
    union {
        Elf_Xword  d_val;
        Elf64_Addr d_ptr;
    } d_un;
};

// ── Version definitions ─────────────────────────────────────────

struct Elfxx_Verdef {
    Elf_Half vd_version;
    Elf_Half vd_flags;
    Elf_Half vd_ndx;
    Elf_Half vd_cnt;
    Elf_Word vd_hash;
    Elf_Word vd_aux;
    Elf_Word vd_next;
};

struct Elfxx_Verdaux {
    Elf_Word vda_name;
    Elf_Word vda_next;
};

struct Elfxx_Verneed {
    Elf_Half vn_version;
    Elf_Half vn_cnt;
    Elf_Word vn_file;
    Elf_Word vn_aux;
    Elf_Word vn_next;
};

struct Elfxx_Vernaux {
    Elf_Word vna_hash;
    Elf_Half vna_flags;
    Elf_Half vna_other;
    Elf_Word vna_name;
    Elf_Word vna_next;
};

// ── Compression header ──────────────────────────────────────────

struct Elf32_Chdr {
    Elf32_Word ch_type;
    Elf32_Word ch_size;
    Elf32_Word ch_addralign;
};

struct Elf64_Chdr {
    Elf64_Word ch_type;
    Elf64_Word ch_reserved;
    Elf_Xword  ch_size;
    Elf_Xword  ch_addralign;
};

// ── Note header (common to 32/64) ──────────────────────────────

struct Elf_Nhdr {
    Elf_Word n_namesz;
    Elf_Word n_descsz;
    Elf_Word n_type;
};

} // namespace elfio

#pragma pack(pop)
