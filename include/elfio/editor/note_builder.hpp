#pragma once

/// Mutable note builder for writing ELF note sections/segments.

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <elfio/core/endian.hpp>
#include <elfio/platform/schema.hpp>

namespace elfio {

class note_builder {
public:
    struct note_entry {
        std::string        name;
        Elf_Word           type = 0;
        std::vector<char>  desc;
    };

    void add(std::string name, Elf_Word type, const char* desc, std::size_t desc_size) {
        note_entry e;
        e.name = std::move(name);
        e.type = type;
        if (desc && desc_size > 0)
            e.desc.assign(desc, desc + desc_size);
        entries_.push_back(std::move(e));
    }

    void add(std::string name, Elf_Word type, const std::vector<char>& desc) {
        add(std::move(name), type, desc.data(), desc.size());
    }

    [[nodiscard]] std::size_t count() const noexcept { return entries_.size(); }

    [[nodiscard]] std::vector<char> build(const endian_convertor& conv) const {
        std::vector<char> out;

        for (const auto& e : entries_) {
            Elf_Nhdr nhdr{};
            auto namesz = static_cast<Elf_Word>(e.name.size() + 1); // include NUL
            auto descsz = static_cast<Elf_Word>(e.desc.size());
            nhdr.n_namesz = conv(namesz);
            nhdr.n_descsz = conv(descsz);
            nhdr.n_type   = conv(e.type);

            // Header
            auto pos = out.size();
            out.resize(pos + sizeof(Elf_Nhdr));
            std::memcpy(out.data() + pos, &nhdr, sizeof(Elf_Nhdr));

            // Name (including NUL terminator + padding to 4-byte alignment)
            out.insert(out.end(), e.name.begin(), e.name.end());
            out.push_back('\0');
            while (out.size() % 4 != 0) out.push_back('\0');

            // Desc + padding to 4-byte alignment
            out.insert(out.end(), e.desc.begin(), e.desc.end());
            while (out.size() % 4 != 0) out.push_back('\0');
        }

        return out;
    }

    void clear() { entries_.clear(); }

private:
    std::vector<note_entry> entries_;
};

} // namespace elfio
