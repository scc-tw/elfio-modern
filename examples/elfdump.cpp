/// Example: dump ELF file information using elfio-modern read API.

#include <elfio/elfio.hpp>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

static std::vector<char> read_file(const char* path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return {};
    auto size = f.tellg();
    f.seekg(0);
    std::vector<char> buf(static_cast<std::size_t>(size));
    f.read(buf.data(), size);
    return buf;
}

template <typename Traits>
static void dump(const elfio::elf_file<Traits>& file) {
    std::printf("  Class:      ELF%d\n", Traits::file_class == elfio::ELFCLASS32 ? 32 : 64);
    std::printf("  Encoding:   %s\n", file.encoding() == elfio::ELFDATA2LSB ? "LSB" : "MSB");
    std::printf("  Type:       %u\n", static_cast<unsigned>(file.type()));
    std::printf("  Machine:    %u\n", static_cast<unsigned>(file.machine()));
    std::printf("  Entry:      0x%llx\n", static_cast<unsigned long long>(file.entry()));
    std::printf("  Sections:   %u\n", static_cast<unsigned>(file.section_count()));
    std::printf("  Segments:   %u\n", static_cast<unsigned>(file.segment_count()));

    std::printf("\nSections:\n");
    std::printf("  [Nr] %-20s %-10s %-16s %-8s %-8s\n",
                "Name", "Type", "Address", "Offset", "Size");

    for (auto sec : file.sections()) {
        std::printf("  [%2u] %-20.20s 0x%08x %016llx %08llx %08llx\n",
                    static_cast<unsigned>(sec.index()),
                    std::string(sec.name()).c_str(),
                    static_cast<unsigned>(sec.type()),
                    static_cast<unsigned long long>(sec.address()),
                    static_cast<unsigned long long>(sec.offset()),
                    static_cast<unsigned long long>(sec.size()));
    }

    std::printf("\nSegments:\n");
    std::printf("  [Nr] %-10s %-16s %-16s %-10s %-10s %-5s\n",
                "Type", "Offset", "VirtAddr", "FileSize", "MemSize", "Flags");

    for (auto seg : file.segments()) {
        std::printf("  [%2u] 0x%08x %016llx %016llx %08llx %08llx %c%c%c\n",
                    static_cast<unsigned>(seg.index()),
                    static_cast<unsigned>(seg.type()),
                    static_cast<unsigned long long>(seg.offset_in_file()),
                    static_cast<unsigned long long>(seg.virtual_address()),
                    static_cast<unsigned long long>(seg.file_size()),
                    static_cast<unsigned long long>(seg.memory_size()),
                    seg.is_readable()   ? 'R' : '-',
                    seg.is_writable()   ? 'W' : '-',
                    seg.is_executable() ? 'X' : '-');
    }

    // Dump symbols from .symtab or .dynsym
    for (auto sec : file.sections()) {
        if (!sec.is_symtab()) continue;

        auto link_idx = sec.link();
        if (link_idx >= file.section_count()) continue;
        auto strtab_sec = file.sections()[static_cast<uint16_t>(link_idx)];
        auto strtab_data = strtab_sec.data();
        if (strtab_data.empty()) continue;

        elfio::string_table_view strtab{strtab_data};

        auto ent_size = sec.entry_size();
        std::size_t num = ent_size > 0 ? static_cast<std::size_t>(sec.size()) / static_cast<std::size_t>(ent_size) : 0;
        std::printf("\nSymbol table '%s' (%zu entries):\n",
                    std::string(sec.name()).c_str(), num);

        auto syms = file.symbols_with_strtab(sec, strtab);
        for (auto sym : syms) {
            auto name = sym.name();
            if (name.empty()) continue;
            std::printf("  %-30.30s 0x%016llx %5llu %s %s\n",
                        std::string(name).c_str(),
                        static_cast<unsigned long long>(sym.value()),
                        static_cast<unsigned long long>(sym.size()),
                        sym.is_func() ? "FUNC" : sym.is_object() ? "OBJ " : "    ",
                        sym.is_global() ? "GLOBAL" : sym.is_local() ? "LOCAL" : sym.is_weak() ? "WEAK" : "");
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::fprintf(stderr, "Usage: %s <elf-file>\n", argv[0]);
        return 1;
    }

    auto buf = read_file(argv[1]);
    if (buf.empty()) {
        std::fprintf(stderr, "Failed to read '%s'\n", argv[1]);
        return 1;
    }

    elfio::byte_view view{buf.data(), buf.size()};
    auto file = elfio::auto_load(view);
    if (!file) {
        std::fprintf(stderr, "Failed to parse ELF: %s\n",
                     std::string(elfio::to_string(file.error())).c_str());
        return 1;
    }

    std::printf("ELF Header:\n");
    std::visit([](const auto& f) { dump(f); }, *file);

    return 0;
}
