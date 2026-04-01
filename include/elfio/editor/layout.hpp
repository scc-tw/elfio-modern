#pragma once

/// ELF layout engine — computes file offsets for all headers and sections.

#include <cstddef>
#include <cstdint>
#include <vector>
#include <elfio/core/safe_math.hpp>
#include <elfio/core/result.hpp>
#include <elfio/platform/schema.hpp>
#include <elfio/platform/traits.hpp>
#include <elfio/editor/section_entry.hpp>
#include <elfio/editor/segment_entry.hpp>

namespace elfio {

template <typename Traits>
struct layout_result {
    uint64_t phoff = 0;         // program header table offset
    uint64_t shoff = 0;         // section header table offset
    uint64_t total_size = 0;    // total file size
};

/// Compute file layout for an ELF file.
///
/// Layout order:
///   1. ELF header
///   2. Program header table (if segments exist)
///   3. Section data (each section aligned per addr_align)
///   4. Section header table
template <typename Traits>
[[nodiscard]] result<layout_result<Traits>> compute_layout(
        std::vector<section_entry<Traits>>& sections,
        std::vector<segment_entry<Traits>>& segments) noexcept
{
    using offset_t = uint64_t;
    layout_result<Traits> lr;

    offset_t cursor = sizeof(typename Traits::ehdr_type);

    // Program header table
    if (!segments.empty()) {
        lr.phoff = cursor;
        auto phdr_total = checked_mul<offset_t>(
            segments.size(), sizeof(typename Traits::phdr_type));
        if (!phdr_total) return error_code::layout_overflow;
        cursor += *phdr_total;
    }

    // Section data (skip index 0 — the null section)
    for (std::size_t i = 0; i < sections.size(); ++i) {
        auto& sec = sections[i];
        sec.set_index(static_cast<uint16_t>(i));

        if (i == 0 || sec.type() == SHT_NULL || sec.type() == SHT_NOBITS) {
            sec.set_offset(0);
            continue;
        }

        auto alignment = sec.addr_align();
        if (alignment > 1) {
            cursor = align_to(cursor, alignment);
        }

        sec.set_offset(cursor);
        cursor += sec.data_size();
    }

    // Section header table
    lr.shoff = cursor;
    auto shdr_total = checked_mul<offset_t>(
        sections.size(), sizeof(typename Traits::shdr_type));
    if (!shdr_total) return error_code::layout_overflow;
    cursor += *shdr_total;

    lr.total_size = cursor;

    // Update segment offsets/sizes based on their associated sections
    for (auto& seg : segments) {
        const auto& indices = seg.section_indices();
        if (indices.empty()) continue;

        uint64_t seg_start = UINT64_MAX;
        uint64_t seg_end   = 0;
        uint64_t mem_end   = 0;

        for (auto idx : indices) {
            if (idx >= sections.size()) continue;
            const auto& sec = sections[idx];
            auto sec_off = sec.offset();
            auto sec_sz  = sec.data_size();

            if (sec_off < seg_start) seg_start = sec_off;
            if (sec_off + sec_sz > seg_end)  seg_end = sec_off + sec_sz;
            if (sec.address() + sec.size() > mem_end)
                mem_end = sec.address() + sec.size();
        }

        if (seg_start != UINT64_MAX) {
            seg.set_offset(seg_start);
            seg.set_filesz(seg_end - seg_start);
            if (mem_end > 0 && !indices.empty()) {
                auto first_addr = sections[indices[0]].address();
                seg.set_memsz(mem_end - first_addr);
            }
        }
    }

    return lr;
}

} // namespace elfio
