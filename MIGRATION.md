# Migration Guide: ELFIO to elfio-modern

This document covers every API change when migrating from the original ELFIO library to elfio-modern.

## Key Architectural Changes

| Aspect | ELFIO (original) | elfio-modern |
|--------|-----------------|-------------|
| Polymorphism | `virtual` classes (`elf_header`, `section`, `segment`) | Template traits (`elf_file<elf64_traits>`), zero virtual functions |
| Read/Write split | Single `elfio` class does both | `elf_file<T>` (read-only, zero-copy) + `elf_editor<T>` (mutable, owned) |
| Memory model | Eager allocation, `std::iostream` based | Zero-copy `byte_view` (read) / `std::vector<char>` (write) |
| Error handling | `bool` returns (no detail) | `result<T, error_code>` with monadic `.map()` / `.and_then()` |
| Iteration | `std::vector` + index | Lazy ranges + pipe `\|` syntax (`filter`, `transform`, `take_while`) |
| String returns | `std::string` (copies) | `std::string_view` (zero-copy, read path) |
| Header-only | Yes (was already header-only) | Yes (C++17, no external dependencies) |
| Exceptions | None (bool returns) | None (`result<T, E>` everywhere, 32+ error codes) |
| 32/64-bit dispatch | Runtime via abstract base classes + `unique_ptr` | Compile-time via traits, or runtime via `std::variant` with `auto_load()` |
| Endianness | Runtime `endianness_convertor` class | Same approach, plus constexpr `byte_swap` + `endian_val<T, Order>` wrapper |
| Accessor pattern | Separate accessor classes (e.g. `symbol_section_accessor`) | Integrated lazy range views (e.g. `symbol_range<Traits>`) |
| Macros | `ELF_ST_BIND`, `ELF32_R_SYM`, etc. | `constexpr` functions: `elf_st_bind()`, `elf32_r_sym()`, etc. |

## Include Change

```cpp
// Old
#include <elfio/elfio.hpp>

// New (single include, same path)
#include <elfio/elfio.hpp>
```

Namespace changes from `ELFIO::` to `elfio::`.

## 1. Loading Files

### Read-only (analysis, inspection)

```cpp
// ---- Old ----
ELFIO::elfio reader;
if (!reader.load("myfile.elf")) { /* failed, no reason */ }

// ---- New (explicit architecture) ----
auto buf = read_file("myfile.elf");  // your own file reader → vector<char>
elfio::byte_view view{buf.data(), buf.size()};
auto file = elfio::elf_file<elfio::elf64_traits>::from_view(view);
if (!file) { std::cerr << elfio::to_string(file.error()); return; }

// ---- New (auto-detect 32/64-bit) ----
auto file = elfio::auto_load(view);  // returns result<variant<elf32, elf64>>
std::visit([](auto& f) { /* use f */ }, *file);
```

### Read-write (editing, saving)

```cpp
// ---- Old ----
ELFIO::elfio editor;
editor.load("myfile.elf");
// ... modify ...
editor.save("modified.elf");

// ---- New ----
elfio::elf_editor<elfio::elf64_traits> ed;
ed.create(elfio::ELFDATA2LSB, elfio::ET_EXEC, elfio::EM_X86_64);
// ... build from scratch or populate from read data ...
auto result = ed.save();  // returns result<vector<char>>
// write result->data() to file
```

### Create from scratch

```cpp
// ---- Old ----
ELFIO::elfio writer;
writer.create(ELFCLASS64, ELFDATA2LSB);
writer.set_os_abi(ELFOSABI_LINUX);
writer.set_type(ET_EXEC);
writer.set_machine(EM_X86_64);

// ---- New ----
elfio::elf_editor<elfio::elf64_traits> ed;
ed.create(elfio::ELFDATA2LSB, elfio::ET_EXEC, elfio::EM_X86_64);
ed.set_os_abi(elfio::ELFOSABI_LINUX);
```

## 2. Error Handling

```cpp
// ---- Old ----
if (!reader.load("file.elf")) {
    // No way to know WHY it failed
}

// ---- New ----
auto file = elfio::elf_file<elfio::elf64_traits>::from_view(data);
if (!file) {
    // Exact reason:
    std::cerr << elfio::to_string(file.error()) << "\n";
    // e.g. "invalid ELF magic number", "truncated header", "file too small"
}

// Monadic chaining:
auto entry = file->entry();  // direct access, no error wrapping for cached fields

// Result monad for sub-operations:
auto sym = view.read<Elf64_Sym>(offset);
auto name = sym.map([](auto s) { return s.st_name; }).value_or(0);
```

## 3. Header Access

```cpp
// ---- Old ----
unsigned char cls = reader.get_class();       // ELFCLASS32 or ELFCLASS64
Elf_Half type     = reader.get_type();
Elf_Half machine  = reader.get_machine();
Elf64_Addr entry  = reader.get_entry();
reader.set_entry(0x400000);

// ---- New (read-only) ----
unsigned char cls = file->file_class();       // compile-time known from Traits
Elf_Half type     = file->type();
Elf_Half machine  = file->machine();
uint64_t entry    = file->entry();

// ---- New (mutable) ----
ed.set_entry(0x400000);
ed.set_type(elfio::ET_EXEC);
ed.set_machine(elfio::EM_X86_64);
```

Note: Read path uses value types (no pointers). Fields are cached at parse time and returned directly.

## 4. Section Operations

### Iteration

```cpp
// ---- Old ----
for (int i = 0; i < reader.sections.size(); ++i) {
    ELFIO::section* sec = reader.sections[i];
    std::string name = sec->get_name();      // allocates
    const char* data = sec->get_data();
    Elf_Xword size   = sec->get_size();
}

// ---- New (zero-copy, lazy) ----
for (auto sec : file->sections()) {
    std::string_view name = sec.name();      // zero-copy
    elfio::byte_view data = sec.data();      // zero-copy
    uint64_t size = sec.size();
}
```

### Find by name

```cpp
// ---- Old ----
ELFIO::section* sec = reader.sections[".text"];

// ---- New ----
auto sec = file->find_section(".text");  // returns optional<section_ref>
if (sec) { /* use sec-> */ }

// Or with lazy find:
auto sec = elfio::find_first(file->sections(),
    [](auto s) { return s.name() == ".text"; });
```

### Filter with pipe syntax (new feature)

```cpp
// Find all executable sections:
for (auto sec : file->sections()
                | elfio::filter([](auto s) { return s.is_execinstr(); })) {
    std::cout << sec.name() << "\n";
}

// Get names of all allocated sections:
auto names = file->sections()
    | elfio::filter([](auto s) { return s.is_alloc(); })
    | elfio::transform([](auto s) { return s.name(); });
```

### Section convenience flags (new)

```cpp
// ---- Old ----
if (sec->get_flags() & SHF_EXECINSTR) { ... }
if (sec->get_type() == SHT_SYMTAB) { ... }

// ---- New ----
if (sec.is_execinstr()) { ... }
if (sec.is_symtab()) { ... }
if (sec.is_alloc()) { ... }
if (sec.is_write()) { ... }
if (sec.is_nobits()) { ... }
if (sec.is_dynamic()) { ... }
if (sec.is_note()) { ... }
```

### Section CRUD (editor)

```cpp
// ---- Old ----
ELFIO::section* text = writer.sections.add(".text");
text->set_type(SHT_PROGBITS);
text->set_flags(SHF_ALLOC | SHF_EXECINSTR);
text->set_data(code, code_size);
text->set_addr_align(16);

// ---- New ----
auto& text = ed.add_section(".text", elfio::SHT_PROGBITS,
                             elfio::SHF_ALLOC | elfio::SHF_EXECINSTR);
text.set_data(code, code_size);
text.set_addr_align(16);
```

## 5. Segment Operations

### Iteration

```cpp
// ---- Old ----
for (int i = 0; i < reader.segments.size(); ++i) {
    ELFIO::segment* seg = reader.segments[i];
    Elf_Word type = seg->get_type();
    Elf64_Addr vaddr = seg->get_virtual_address();
}

// ---- New (zero-copy, lazy) ----
for (auto seg : file->segments()) {
    Elf_Word type = seg.type();
    uint64_t vaddr = seg.virtual_address();
    if (seg.is_load()) { ... }
    if (seg.is_executable()) { ... }
}
```

### Segment CRUD (editor)

```cpp
// ---- Old ----
ELFIO::segment* seg = writer.segments.add();
seg->set_type(PT_LOAD);
seg->set_flags(PF_R | PF_X);
seg->add_section_index(text->get_index(), text->get_addr_align());

// ---- New ----
auto& seg = ed.add_segment(elfio::PT_LOAD, elfio::PF_R | elfio::PF_X);
seg.set_vaddr(0x400000);
seg.set_align(0x1000);
seg.add_section_index(1);  // section index
```

## 6. Symbol Operations

### Reading

```cpp
// ---- Old ----
ELFIO::symbol_section_accessor symbols(reader, symtab_sec);
for (Elf_Xword i = 0; i < symbols.get_symbols_num(); ++i) {
    std::string name;
    Elf64_Addr value;
    Elf_Xword size;
    unsigned char bind, type;
    Elf_Half shndx;
    unsigned char other;
    symbols.get_symbol(i, name, value, size, bind, type, shndx, other);
}

// ---- New (zero-copy, lazy, strongly typed) ----
auto strtab_data = file->sections()[symtab_sec.link()].data();
elfio::string_table_view strtab{strtab_data};
for (auto sym : file->symbols_with_strtab(symtab_sec, strtab)) {
    std::string_view name = sym.name();          // zero-copy
    uint64_t value        = sym.value();
    uint64_t size         = sym.size();
    unsigned char bind    = sym.bind();           // decomposed from st_info
    unsigned char type    = sym.type();           // decomposed from st_info
    unsigned char vis     = sym.visibility();     // from st_other
    Elf_Half shndx        = sym.shndx();

    if (sym.is_func() && sym.is_global()) { ... }
    if (sym.is_undef()) { ... }
}
```

### Writing

```cpp
// ---- Old ----
ELFIO::symbol_section_accessor syma(writer, symtab_sec);
syma.add_symbol(str_idx, 0x1000, 100, STB_GLOBAL, STT_FUNC, 0, text->get_index());

// ---- New ----
elfio::symbol_builder<elfio::elf64_traits> syms;
(void)syms.add("main", 0x1000, 100, elfio::STB_GLOBAL, elfio::STT_FUNC,
                elfio::STV_DEFAULT, text_section_index);

elfio::string_table_builder strtab;
elfio::endian_convertor conv(elfio::byte_order::little);
auto symdata = syms.build(strtab, conv);  // produces vector<char>
```

## 7. Relocation Operations

```cpp
// ---- Old ----
ELFIO::relocation_section_accessor rels(reader, rel_sec);
for (Elf_Xword i = 0; i < rels.get_entries_num(); ++i) {
    Elf64_Addr offset;
    Elf_Word symbol;
    unsigned type;
    Elf_Sxword addend;
    rels.get_entry(i, offset, symbol, type, addend);
}

// ---- New (SHT_RELA sections) ----
for (auto rel : file->relas(rela_sec)) {
    uint64_t offset = rel.r_offset();
    uint64_t symbol = rel.symbol();     // extracted from r_info
    uint64_t type   = rel.type();       // extracted from r_info
    int64_t addend  = rel.r_addend();
}

// ---- New (SHT_REL sections) ----
for (auto rel : file->rels(rel_sec)) {
    uint64_t offset = rel.r_offset();
    uint64_t symbol = rel.symbol();
    uint64_t type   = rel.type();
}
```

## 8. Dynamic Section

```cpp
// ---- Old ----
ELFIO::dynamic_section_accessor dyn(reader, dyn_sec);
for (Elf_Xword i = 0; i < dyn.get_entries_num(); ++i) {
    Elf_Xword tag, value;
    std::string str;
    dyn.get_entry(i, tag, value, str);
    if (tag == DT_NEEDED) std::cout << "Needs: " << str << "\n";
}

// ---- New ----
auto dynstr_data = file->sections()[dyn_sec.link()].data();
elfio::string_table_view dynstr{dynstr_data};
for (auto d : file->dynamics(dyn_sec, dynstr)) {
    if (d.tag() == elfio::DT_NEEDED)
        std::cout << "Needs: " << d.string_value() << "\n";
    if (d.is_null()) break;
}
```

## 9. Note Sections

```cpp
// ---- Old ----
ELFIO::note_section_accessor notes(reader, note_sec);
for (Elf_Word i = 0; i < notes.get_notes_num(); ++i) {
    Elf_Word type;
    std::string name;
    char* desc; Elf_Word desc_size;
    notes.get_note(i, type, name, desc, desc_size);
}

// ---- New (zero-copy) ----
for (auto note : file->notes(note_sec)) {
    Elf_Word type           = note.type();
    std::string_view name   = note.name();     // zero-copy
    elfio::byte_view desc   = note.desc();     // zero-copy
}

// Also works on segments:
for (auto seg : file->segments()) {
    if (seg.is_note()) {
        for (auto note : file->notes(seg)) { ... }
    }
}
```

## 10. Version Symbols (new feature: lazy views)

```cpp
// ---- Old ----
ELFIO::versym_section_accessor versym(reader, versym_sec);
Elf_Half value;
versym.get_entry(0, value);

// ---- New ----
elfio::versym_range vsyms{versym_sec.data(), &file->convertor()};
for (auto v : vsyms) {
    Elf_Half val = v.value();
}
```

## 11. Module Info

```cpp
// ---- Old ----
ELFIO::modinfo_section_accessor modinfo(reader, modinfo_sec);
std::string field, value;
modinfo.get_attribute(0, field, value);

// ---- New (zero-copy, range-based) ----
elfio::modinfo_range mi{modinfo_sec.data()};
for (auto attr : mi) {
    std::string_view key   = attr.key();
    std::string_view value = attr.value();
}
```

## 12. Array Sections (.init_array, .fini_array)

```cpp
// ---- Old ----
ELFIO::array_section_accessor<Elf64_Addr> arr(reader, init_array_sec);
Elf64_Addr addr;
arr.get_entry(0, addr);

// ---- New ----
elfio::array_range<uint64_t> arr{init_array_sec.data(), &file->convertor()};
for (auto entry : arr) {
    uint64_t addr = entry.value();
}
// Or by index:
uint64_t first = arr[0].value();
```

## 13. Hash Functions

```cpp
// ---- Old ----
ELFIO::elf_hash("symbol_name");
ELFIO::elf_gnu_hash("symbol_name");

// ---- New ----
elfio::elf_hash("symbol_name");
elfio::elf_gnu_hash("symbol_name");
```

## 14. Dump / Formatting Utilities

```cpp
// ---- Old ----
ELFIO::dump::header(std::cout, reader);
ELFIO::dump::section_headers(std::cout, reader);

// ---- New (string formatting functions) ----
elfio::str_file_type(file->type());         // "EXEC (Executable)"
elfio::str_class(file->file_class());       // "ELF64"
elfio::str_encoding(file->encoding());      // "2's complement, little endian"
elfio::str_machine(file->machine());        // "AMD x86-64"
elfio::str_section_type(sec.type());        // "PROGBITS"
elfio::str_segment_type(seg.type());        // "LOAD"
elfio::str_section_flags(sec.flags());      // "WAX"
elfio::str_segment_flags(seg.flags());      // "RWE"
elfio::str_symbol_bind(sym.bind());         // "GLOBAL"
elfio::str_symbol_type(sym.type());         // "FUNC"
elfio::str_os_abi(file->os_abi());          // "Linux"
elfio::default_entry_size<elfio::elf64_traits>(elfio::SHT_SYMTAB);  // 24
```

## 15. Constant / Macro Changes

| ELFIO | elfio-modern |
|-------|-------------|
| `ELFIO::` namespace | `elfio::` namespace |
| `ELF_ST_BIND(i)` macro | `elfio::elf_st_bind(i)` constexpr function |
| `ELF_ST_TYPE(i)` macro | `elfio::elf_st_type(i)` constexpr function |
| `ELF_ST_INFO(b, t)` macro | `elfio::elf_st_info(b, t)` constexpr function |
| `ELF32_R_SYM(i)` macro | `elfio::elf32_r_sym(i)` constexpr function |
| `ELF64_R_INFO(s, t)` macro | `elfio::elf64_r_info(s, t)` constexpr function |
| `Elf_Half`, `Elf_Word`, etc. | Same names, same types in `elfio::` namespace |
| `Elf32_Ehdr`, `Elf64_Ehdr`, etc. | Same names, packed structs in `elfio::` namespace |
| All constants (`ET_EXEC`, `SHT_PROGBITS`, etc.) | Same names in `elfio::` namespace |

## 16. Complete Example: Read-Modify-Write

```cpp
#include <elfio/elfio.hpp>
#include <fstream>
#include <iostream>
#include <vector>

int main() {
    // Create a new ELF64 relocatable object
    elfio::elf_editor<elfio::elf64_traits> ed;
    ed.create(elfio::ELFDATA2LSB, elfio::ET_REL, elfio::EM_X86_64);
    ed.set_os_abi(elfio::ELFOSABI_LINUX);

    // Add .text section
    unsigned char code[] = { 0x55, 0x48, 0x89, 0xe5, 0xc3 };  // push rbp; mov rbp,rsp; ret
    auto& text = ed.add_section(".text", elfio::SHT_PROGBITS,
                                 elfio::SHF_ALLOC | elfio::SHF_EXECINSTR);
    text.set_data(reinterpret_cast<const char*>(code), sizeof(code));
    text.set_addr_align(16);

    // Add .data section
    const char msg[] = "Hello, elfio-modern!";
    auto& data = ed.add_section(".data", elfio::SHT_PROGBITS,
                                 elfio::SHF_ALLOC | elfio::SHF_WRITE);
    data.set_data(msg, sizeof(msg));

    // Save to binary
    auto result = ed.save();
    if (!result) {
        std::cerr << elfio::to_string(result.error()) << "\n";
        return 1;
    }

    // Write to file
    std::ofstream out("output.o", std::ios::binary);
    out.write(result->data(), static_cast<std::streamsize>(result->size()));

    // Read it back and inspect
    elfio::byte_view view{result->data(), result->size()};
    auto file = elfio::elf_file<elfio::elf64_traits>::from_view(view);
    if (!file) {
        std::cerr << elfio::to_string(file.error()) << "\n";
        return 1;
    }

    std::cout << "Type: " << elfio::str_file_type(file->type()) << "\n";
    std::cout << "Machine: " << elfio::str_machine(file->machine()) << "\n";

    for (auto sec : file->sections()) {
        std::cout << sec.name()
                  << " [" << elfio::str_section_type(sec.type()) << "]"
                  << " flags=" << elfio::str_section_flags(sec.flags())
                  << " size=" << sec.size() << "\n";
    }

    // Use pipe syntax to find code sections:
    for (auto sec : file->sections()
                    | elfio::filter([](auto s) { return s.is_execinstr(); })) {
        std::cout << "Code section: " << sec.name() << "\n";
    }

    return 0;
}
```
