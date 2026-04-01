# elfio-modern

A modern C++17 header-only library for parsing and editing ELF binary files.

Zero-copy parsing, compile-time polymorphism via traits, lazy evaluation, monadic error handling — no virtual functions, no exceptions, no external dependencies.

## Features

- **Header-only** — add `include/` to your include path, done
- **C++17, standard library only** — no Boost, no platform-specific APIs
- **Zero-copy parsing** — `byte_view` reads directly from the file buffer via `memcpy`, no intermediate allocations
- **Compile-time polymorphism** — `elf_file<elf32_traits>` / `elf_file<elf64_traits>` with zero virtual dispatch
- **No exceptions** — `result<T, E>` with monadic `map()` / `and_then()` for composable error handling
- **Lazy evaluation** — sections, segments, symbols iterated on demand, never materialized into vectors
- **Pipe syntax** — `file.sections() | filter(...) | transform(...)` in C++17
- **Overflow-safe** — all offset arithmetic through `checked_add` / `checked_mul`, safe against malicious inputs
- **Read/write split** — `elf_file<T>` for zero-copy reading, `elf_editor<T>` for mutation and serialization
- **Both endianness** — little-endian and big-endian ELF files fully supported

### Supported Formats

| Format | Traits | Description |
|--------|--------|-------------|
| ELF32 | `elf32_traits` | 32-bit ELF (x86, ARM, MIPS, etc.) |
| ELF64 | `elf64_traits` | 64-bit ELF (x86-64, AArch64, RISC-V, etc.) |

### Feature Coverage

| Feature | Read | Write |
|---------|------|-------|
| ELF header | `elf_file<T>` | `elf_editor<T>` |
| Sections | `section_ref<T>` / `section_range<T>` | `section_entry<T>` |
| Segments | `segment_ref<T>` / `segment_range<T>` | `segment_entry<T>` |
| Symbol tables | `symbol_ref<T>` / `symbol_range<T>` | `symbol_builder<T>` |
| Relocations (REL/RELA) | `rel_ref<T>` / `rela_ref<T>` | `relocation_builder<T>` |
| Dynamic section | `dynamic_ref<T>` / `dynamic_range<T>` | `dynamic_builder<T>` |
| Notes | `note_ref` / `note_range` | `note_builder` |
| String tables | `string_table_view` | `string_table_builder` |
| Version symbols | `versym_range` / `verneed_range` / `verdef_range` | `versym_builder` |
| Module info | `modinfo_range` | `modinfo_builder` |
| Array sections | `array_range<T>` | `array_builder<T>` |
| Hash functions | `elf_hash()` / `elf_gnu_hash()` | — |
| Dump/formatting | `str_file_type()`, `str_machine()`, etc. | — |

## Requirements

- C++17 compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.20+
- Google Test (for tests only — auto-fetched or system-installed)

## Getting Started

### Option 1: Add as a subdirectory

```bash
git clone https://github.com/scc-tw/elfio-modern.git
```

```cmake
add_subdirectory(elfio-modern)
target_link_libraries(your_target PRIVATE elfio::elfio-modern)
```

### Option 2: CMake FetchContent

```cmake
include(FetchContent)
FetchContent_Declare(
    elfio-modern
    GIT_REPOSITORY https://github.com/scc-tw/elfio-modern.git
    GIT_TAG        v1.0.0
)
FetchContent_MakeAvailable(elfio-modern)
target_link_libraries(your_target PRIVATE elfio::elfio-modern)
```

### Option 3: Copy the headers

Copy `include/elfio/` into your project and add the parent directory to your include path.

## Building Tests & Examples

```bash
cmake -B build
cmake --build build
ctest --test-dir build
```

If Google Test is not installed system-wide:

```bash
cmake -B build -DELFIO_FETCH_GTEST=ON
cmake --build build
```

| Option | Default | Description |
|--------|---------|-------------|
| `ELFIO_BUILD_TESTS` | `ON` | Build test suite |
| `ELFIO_BUILD_EXAMPLES` | `ON` | Build example programs |
| `ELFIO_FETCH_GTEST` | `OFF` | Auto-download Google Test if not found |

## Usage

### Parse an ELF file

```cpp
#include <elfio/elfio.hpp>
#include <fstream>
#include <iostream>
#include <vector>

std::vector<char> read_file(const char* path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return {};
    auto size = f.tellg();
    f.seekg(0);
    std::vector<char> buf(static_cast<std::size_t>(size));
    f.read(buf.data(), size);
    return buf;
}

int main() {
    auto buf = read_file("/usr/bin/ls");
    elfio::byte_view view{buf.data(), buf.size()};

    auto file = elfio::elf_file<elfio::elf64_traits>::from_view(view);
    if (!file) {
        std::cerr << elfio::to_string(file.error()) << "\n";
        return 1;
    }

    std::cout << "Type: " << elfio::str_file_type(file->type()) << "\n";
    std::cout << "Machine: " << elfio::str_machine(file->machine()) << "\n";

    for (auto sec : file->sections()) {
        std::cout << sec.name()
                  << "  [" << elfio::str_section_type(sec.type()) << "]"
                  << "  size=" << sec.size() << "\n";
    }
}
```

### Auto-detect architecture

```cpp
auto file = elfio::auto_load(view);  // returns result<variant<elf32, elf64>>
if (!file) { /* handle error */ }

std::visit([](auto& f) {
    for (auto sec : f.sections())
        std::cout << sec.name() << "\n";
}, *file);
```

### Filter and transform with pipes

```cpp
auto code_sections = file->sections()
    | elfio::filter([](auto s) { return s.is_execinstr(); })
    | elfio::transform([](auto s) { return s.name(); });

for (auto name : code_sections)
    std::cout << name << "\n";
```

### Read symbols

```cpp
for (auto sec : file->sections()) {
    if (!sec.is_symtab()) continue;

    auto strtab_sec = file->sections()[static_cast<uint16_t>(sec.link())];
    elfio::string_table_view strtab{strtab_sec.data()};

    for (auto sym : file->symbols_with_strtab(sec, strtab)) {
        if (sym.name().empty()) continue;
        std::cout << sym.name()
                  << "  value=0x" << std::hex << sym.value()
                  << "  " << elfio::str_symbol_type(sym.type())
                  << "  " << elfio::str_symbol_bind(sym.bind())
                  << "\n";
    }
}
```

### Read dynamic section

```cpp
auto dyn_sec = file->find_section(".dynamic");
if (dyn_sec) {
    auto dynstr_sec = file->sections()[static_cast<uint16_t>(dyn_sec->link())];
    elfio::string_table_view dynstr{dynstr_sec.data()};

    for (auto d : file->dynamics(*dyn_sec, dynstr)) {
        if (d.is_null()) break;
        if (d.tag() == elfio::DT_NEEDED)
            std::cout << "Needs: " << d.string_value() << "\n";
    }
}
```

### Read notes

```cpp
for (auto sec : file->sections()) {
    if (!sec.is_note()) continue;
    for (auto note : file->notes(sec)) {
        std::cout << "Note: " << note.name()
                  << "  type=" << note.type()
                  << "  desc_size=" << note.desc().size() << "\n";
    }
}
```

### Monadic error handling

```cpp
auto r = elfio::elf_file<elfio::elf64_traits>::from_view(view);
// Chain operations — errors propagate automatically:
auto machine = r.map([](auto& f) { return f.machine(); }).value_or(0);
```

### Create an ELF object file

```cpp
elfio::elf_editor<elfio::elf64_traits> ed;
ed.create(elfio::ELFDATA2LSB, elfio::ET_REL, elfio::EM_X86_64);
ed.set_os_abi(elfio::ELFOSABI_LINUX);

// Add .text section
unsigned char code[] = { 0x55, 0x48, 0x89, 0xe5, 0xc3 };
auto& text = ed.add_section(".text", elfio::SHT_PROGBITS,
                             elfio::SHF_ALLOC | elfio::SHF_EXECINSTR);
text.set_data(reinterpret_cast<const char*>(code), sizeof(code));
text.set_addr_align(16);

// Add .data section
const char msg[] = "Hello, elfio-modern!";
auto& data = ed.add_section(".data", elfio::SHT_PROGBITS,
                             elfio::SHF_ALLOC | elfio::SHF_WRITE);
data.set_data(msg, sizeof(msg));

// Save to binary (layout computed automatically)
auto result = ed.save();
if (!result) {
    std::cerr << elfio::to_string(result.error()) << "\n";
    return 1;
}

std::ofstream out("output.o", std::ios::binary);
out.write(result->data(), static_cast<std::streamsize>(result->size()));
```

### Build relocations

```cpp
elfio::relocation_builder<elfio::elf64_traits> rb;
rb.add_rela(0x10, 1, elfio::R_X86_64_64, 0);     // offset, sym_idx, type, addend
rb.add_rela(0x20, 2, elfio::R_X86_64_PC32, -4);

elfio::endian_convertor conv(elfio::byte_order::little);
auto rela_data = rb.build_rela(conv);

auto& rela_sec = ed.add_section(".rela.text", elfio::SHT_RELA, elfio::SHF_INFO_LINK);
rela_sec.set_data(rela_data.data(), rela_data.size());
rela_sec.set_entry_size(sizeof(elfio::Elf64_Rela));
rela_sec.set_link(symtab_index);  // index of .symtab
rela_sec.set_info(text_index);    // index of .text

// REL (without addend) works the same way:
rb.add_rel(0x10, 1, elfio::R_X86_64_64);
auto rel_data = rb.build_rel(conv);
```

### Build dynamic section

```cpp
elfio::string_table_builder dynstr;
elfio::dynamic_builder<elfio::elf64_traits> db;

db.add_needed("libc.so.6");         // DT_NEEDED with string lookup
db.add_needed("libm.so.6");
db.add_soname("mylib.so");          // DT_SONAME
db.add_runpath("/opt/lib");         // DT_RUNPATH
db.add(elfio::DT_FLAGS, elfio::DF_BIND_NOW);  // raw tag+value

elfio::endian_convertor conv(elfio::byte_order::little);
auto dyn_data = db.build(dynstr, conv);  // strings resolved into dynstr

auto& dynstr_sec = ed.add_section(".dynstr", elfio::SHT_STRTAB, elfio::SHF_ALLOC);
dynstr_sec.set_data(dynstr.data());

auto& dyn_sec = ed.add_section(".dynamic", elfio::SHT_DYNAMIC,
                                elfio::SHF_ALLOC | elfio::SHF_WRITE);
dyn_sec.set_data(dyn_data.data(), dyn_data.size());
dyn_sec.set_entry_size(sizeof(elfio::Elf64_Dyn));
dyn_sec.set_link(dynstr_sec_index);
```

### Build notes

```cpp
elfio::note_builder nb;

// GNU build ID
std::vector<char> build_id = {'\x01', '\x02', '\x03', '\x04'};
nb.add("GNU", elfio::NT_GNU_BUILD_ID, build_id);

// Custom note
nb.add("MyApp", 1, "payload", 7);

elfio::endian_convertor conv(elfio::byte_order::little);
auto note_data = nb.build(conv);  // 4-byte aligned automatically

auto& note_sec = ed.add_section(".note.gnu.build-id", elfio::SHT_NOTE, elfio::SHF_ALLOC);
note_sec.set_data(note_data.data(), note_data.size());
note_sec.set_addr_align(4);
```

### Build array sections (.init_array, .fini_array)

```cpp
elfio::array_builder<uint64_t> ab;
ab.add(0x400100);  // address of init function 1
ab.add(0x400200);  // address of init function 2

elfio::endian_convertor conv(elfio::byte_order::little);
auto arr_data = ab.build(conv);

auto& sec = ed.add_section(".init_array", elfio::SHT_INIT_ARRAY,
                            elfio::SHF_ALLOC | elfio::SHF_WRITE);
sec.set_data(arr_data.data(), arr_data.size());
sec.set_entry_size(sizeof(uint64_t));
sec.set_addr_align(8);
```

### Build module info (.modinfo)

```cpp
elfio::modinfo_builder mb;
mb.add("license", "GPL");
mb.add("author", "John Doe");
mb.add("description", "My kernel module");

auto data = mb.build();  // NUL-terminated "key=value" pairs

auto& sec = ed.add_section(".modinfo", elfio::SHT_PROGBITS);
sec.set_data(data.data(), data.size());
```

### Build version symbols (.gnu.version)

```cpp
elfio::versym_builder vb;
vb.add(0);  // VER_NDX_LOCAL
vb.add(1);  // VER_NDX_GLOBAL
vb.add(2);  // user-defined version index

elfio::endian_convertor conv(elfio::byte_order::little);
auto data = vb.build(conv);

auto& sec = ed.add_section(".gnu.version", elfio::SHT_GNU_versym, elfio::SHF_ALLOC);
sec.set_data(data.data(), data.size());
sec.set_entry_size(sizeof(elfio::Elf_Half));
sec.set_link(dynsym_sec_index);
```

### Zero-copy from mmap

```cpp
void* mapped = mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
elfio::byte_view data(mapped, file_size);
auto file = elfio::elf_file<elfio::elf64_traits>::from_view(data);
// All parsing reads directly from the mapped pages — zero allocations
```

## Architecture

```
elfio.hpp                          <- single-include entry point
|
+-- core/
|   +-- error.hpp                  <- error_code enum + to_string()
|   +-- result.hpp                 <- result<T,E> with map/and_then
|   +-- safe_math.hpp              <- checked_add/mul, align_to
|   +-- endian.hpp                 <- byte_swap, endian_convertor
|   +-- byte_view.hpp              <- non-owning byte span, memcpy reads
|   +-- lazy.hpp                   <- filter | transform | take_while
|   +-- hash.hpp                   <- elf_hash, elf_gnu_hash
|
+-- platform/
|   +-- schema.hpp                 <- packed ELF binary structs + constants
|   +-- traits.hpp                 <- elf32_traits / elf64_traits
|
+-- views/                         (read-only, lazy, zero-copy)
|   +-- string_table.hpp           <- NUL-terminated string lookup
|   +-- section_view.hpp           <- section_ref + section_range
|   +-- segment_view.hpp           <- segment_ref + segment_range
|   +-- symbol_view.hpp            <- symbol_ref + symbol_range
|   +-- relocation_view.hpp        <- rel_ref/rela_ref + ranges
|   +-- dynamic_view.hpp           <- dynamic_ref + dynamic_range
|   +-- note_view.hpp              <- note_ref + note_range
|   +-- versym_view.hpp            <- version symbol views
|   +-- modinfo_view.hpp           <- kernel module info parser
|   +-- array_view.hpp             <- generic typed array view
|   +-- dump.hpp                   <- formatting/string utilities
|   +-- elf_file.hpp               <- elf_file<T>, auto_load(), detect_elf_arch()
|
+-- editor/                        (mutable, owned data)
    +-- elf_editor.hpp             <- create / modify / save
    +-- section_entry.hpp          <- mutable section with data
    +-- segment_entry.hpp          <- mutable segment
    +-- symbol_builder.hpp         <- symbol table construction
    +-- relocation_builder.hpp     <- REL/RELA table construction
    +-- dynamic_builder.hpp        <- dynamic section construction
    +-- note_builder.hpp           <- note section construction
    +-- array_builder.hpp          <- init/fini array construction
    +-- modinfo_builder.hpp        <- kernel module info construction
    +-- versym_builder.hpp         <- version symbol construction
    +-- string_table_builder.hpp   <- string table with deduplication
    +-- layout.hpp                 <- file offset computation
```

### Design Choices

| Concern | Approach |
|---------|----------|
| Polymorphism | Traits templates, `if constexpr`, zero vtables |
| Memory (read) | `byte_view` — zero-copy, reads via `memcpy` |
| Memory (write) | `elf_editor` owns all data via `std::vector` |
| Errors | `result<T, error_code>` — no exceptions, 30 distinct codes |
| Alignment | `std::memcpy` everywhere — no `reinterpret_cast` on structs, no UB |
| Overflow | `checked_add` / `checked_mul` on all offset arithmetic |
| Iteration | Lazy ranges with pipe syntax, never allocates |
| String access | `std::string_view` into file buffer (read path) |
| Endianness | Runtime `endian_convertor` + compile-time `byte_swap` |

## Examples

Two example programs are included in `examples/`:

| Program | Description |
|---------|-------------|
| `elfdump` | Dump ELF headers, sections, segments, and symbols |
| `write_obj` | Create a minimal ELF relocatable object file |

```bash
./build/examples/elfdump /usr/bin/ls
./build/examples/write_obj
```

## Tests

4 test suites covering core, platform, views, and editor:

| Suite | Covers |
|-------|--------|
| `core_test` | result, byte_view, safe_math, endian, lazy ranges |
| `platform_test` | struct sizes, constants, relocation/symbol helpers, traits |
| `view_test` | section/segment iteration, find, auto_load, pipes, string table |
| `editor_test` | create/save roundtrip (32/64-bit, LE/BE), string dedup, symbol builder |

## Migrating from ELFIO

See [MIGRATION.md](MIGRATION.md) for a comprehensive side-by-side migration guide.

## License

MIT License. See [LICENSE](LICENSE) for details.
