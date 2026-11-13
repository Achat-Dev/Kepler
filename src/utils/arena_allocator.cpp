// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "utils/arena_allocator.hpp"
#include "utils/assert.h"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <utility>

namespace kepler {

    ArenaAllocator::ArenaAllocator(size_t block_size, std::string label)
        : block_size(block_size), label(std::move(label)) {
        KPL_ASSERT_THAT(block_size > 0);
        blocks.emplace_back(block_size);
        current_block = &blocks.back();
    }

    // clang-format off
    ArenaAllocator::ArenaAllocator(ArenaAllocator&& other)
        : block_size(other.block_size),
          label(std::move(other.label)),
          blocks(std::move(other.blocks)),
          destructors(std::move(other.destructors)),
          current_block(other.current_block)
    {
        // clang-format on
        other.current_block = nullptr;
    }

    ArenaAllocator::~ArenaAllocator() {
        free();
    }

    void ArenaAllocator::free() {
        if (blocks.empty()) {
            return;
        }

        for (auto it = destructors.rbegin(); it != destructors.rend(); it++) {
            it->destructor(it->obj);
        }
        destructors.clear();
        blocks.clear();
    }

    ArenaAllocator::Block::Block(size_t size)
        : size(size), offset(0) {
        data = static_cast<uint8_t*>(std::malloc(size));
    }

    ArenaAllocator::Block::Block(Block&& other)
        : size(other.size), offset(other.offset), data(other.data) {
        other.data = nullptr;
        other.size = 0;
        other.offset = 0;
    }

    ArenaAllocator::Block& ArenaAllocator::Block::operator=(Block&& other) {
        if (this != &other) {
            if (data != nullptr) {
                std::free(data);
            }
            data = other.data;
            size = other.size;
            offset = other.offset;

            other.data = nullptr;
            other.size = 0;
            other.offset = 0;
        }
        return *this;
    }

    ArenaAllocator::Block::~Block() {
        if (data != nullptr) {
            std::free(data);
        }
        data = nullptr;
    }

    void* ArenaAllocator::Block::allocate(size_t size, size_t alignment) {
        // offset % alignment gives the misalignment
        // alignment - that gives the padding
        // If offset % alignment = 0 -> alignment - 0 -> alignment, which means that we pad one alignment when we don't need any padding at all. The final & alignment fixes that
        size_t padding = (alignment - offset % alignment) % alignment;
        if (offset > this->size - (size + padding)) {
            return nullptr;
        }

        offset += padding;
        void* result = reinterpret_cast<void*>(data + offset);
        offset += size;
        return result;
    }

    void ArenaAllocator::Block::free() {
        if (data != nullptr) {
            std::free(data);
        }
        data = nullptr;
    }

}
