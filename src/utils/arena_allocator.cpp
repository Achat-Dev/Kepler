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
        KPL_ASSERT_THAT(block_size > 0, "Blocksize of ArenaAllocator has to be greater than 0");
        blocks.emplace_back(this, block_size);
        current_block = &blocks.back();
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

    ArenaAllocator::Block::Block(ArenaAllocator* parent, size_t size)
        : parent(parent), size(size), offset(0) {
        data = static_cast<uint8_t*>(std::malloc(size));
    }

    ArenaAllocator::Block::Block(Block&& other)
        : parent(other.parent), size(other.size), offset(other.offset), data(other.data) {
        other.parent = nullptr;
        other.data = nullptr;
        other.size = 0;
        other.offset = 0;
    }

    ArenaAllocator::Block& ArenaAllocator::Block::operator=(Block&& other) {
        if (this != &other) {
            if (data != nullptr) {
                std::free(data);
            }
            parent = other.parent;
            data = other.data;
            size = other.size;
            offset = other.offset;

            other.parent = nullptr;
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
