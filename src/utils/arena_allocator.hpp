// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace kepler {

    inline constexpr size_t kibibyte_size = 1024;

    class ArenaAllocator {
    public:
        ArenaAllocator(size_t size, std::string label);
        ArenaAllocator(const ArenaAllocator&) = delete;
        ArenaAllocator& operator=(const ArenaAllocator&) = delete;
        ArenaAllocator(ArenaAllocator&&) = delete;
        ArenaAllocator& operator=(ArenaAllocator&&) = delete;
        ~ArenaAllocator();

        void free();

        template <typename T, typename... Args>
            requires(!std::is_array_v<T>)
        T* allocate(Args&&... args) {
            assert(blocks.size() > 0 && "Can't call 'allocate' on freed ArenaAllocator");
            assert(sizeof(T) <= block_size && "Can't allocate an object that is bigger than the blocksize of an ArenaAllocator");

            void* memory = current_block->allocate(sizeof(T), alignof(T));

            // Block doesn't have enough space left, so create new block and allocate again
            if (memory == nullptr) {
                blocks.emplace_back(this, block_size);
                current_block = &blocks.back();
                memory = current_block->allocate(sizeof(T), alignof(T));
            }

            if constexpr (std::is_destructible_v<T> && !std::is_trivially_destructible_v<T>) {
                destructors.push_back({memory, [](void* obj) {
                                           static_cast<T*>(obj)->~T();
                                       }});
            }
            return new (memory) T{std::forward<Args>(args)...};
        }

    private:
        struct Destructor {
            void* obj;
            void (*destructor)(void*);
        };

        struct Block {
            ArenaAllocator* parent;
            uint8_t* data;
            size_t size;
            size_t offset;

            Block(ArenaAllocator* parent, size_t size);
            Block(const Block&) = delete;
            Block& operator=(const Block&) = delete;
            Block(Block&& other);
            Block& operator=(Block&& other);
            ~Block();

            void* allocate(size_t size, size_t alignment);
            void free();
        };

        const size_t block_size;
        const std::string label;
        std::vector<Block> blocks;
        std::vector<Destructor> destructors;
        Block* current_block;

        void* allocate(size_t size, size_t alignment);
    };

}
