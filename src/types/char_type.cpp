// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "types/char_type.hpp"

#include "log.hpp"
#include "types/type_token.hpp"

#include <exception>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <string>

namespace Kepler::Type {

    std::string CharType::get_name() const {
        return "char";
    }

    llvm::Type* CharType::get_llvm_type() const {
        log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": type 'char' is not supported yet");
        std::terminate();
        return nullptr;
    }

    llvm::Value* CharType::cast(llvm::Value* value, TypeToken to) const {
        log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": type 'char' is not supported yet");
        std::terminate();
        return nullptr;
    }

    llvm::Value* CharType::create_add(llvm::Value* lhs, llvm::Value* rhs) const {
        log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": type 'char' is not supported yet");
        std::terminate();
        return nullptr;
    }

    llvm::Value* CharType::create_sub(llvm::Value* lhs, llvm::Value* rhs) const {
        log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": type 'char' is not supported yet");
        std::terminate();
        return nullptr;
    }

    llvm::Value* CharType::create_mul(llvm::Value* lhs, llvm::Value* rhs) const {
        log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": type 'char' is not supported yet");
        std::terminate();
        return nullptr;
    }

    llvm::Value* CharType::create_div(llvm::Value* lhs, llvm::Value* rhs) const {
        log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": type 'char' is not supported yet");
        std::terminate();
        return nullptr;
    }

    llvm::Value* CharType::create_less_than(llvm::Value* lhs, llvm::Value* rhs) const {
        log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": type 'char' is not supported yet");
        std::terminate();
        return nullptr;
    }

    llvm::Value* CharType::create_greater_than(llvm::Value* lhs, llvm::Value* rhs) const {
        log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": type 'char' is not supported yet");
        std::terminate();
        return nullptr;
    }

    llvm::Value* CharType::create_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": type 'char' is not supported yet");
        std::terminate();
        return nullptr;
    }

    llvm::Value* CharType::create_not_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": type 'char' is not supported yet");
        std::terminate();
        return nullptr;
    }

    llvm::Value* CharType::create_less_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": type 'char' is not supported yet");
        std::terminate();
        return nullptr;
    }

    llvm::Value* CharType::create_greater_equals(llvm::Value* lhs, llvm::Value* rhs) const {
        log(LogStyle::UNSUPPORTED, "[ Unpaid developer error ]", LogStyle::DEFAULT, ": type 'char' is not supported yet");
        std::terminate();
        return nullptr;
    }

}
