// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "ast/abstract_syntax_tree.hpp"
#include "ast/ast_node.hpp"
#include "ast/ast_pass.hpp"
#include "ast/expressions/binary_expression.hpp"
#include "ast/expressions/call_expression.hpp"
#include "ast/expressions/cast_expression.hpp"
#include "ast/expressions/literals/boolean_literal_expression.hpp"
#include "ast/expressions/literals/floating_point_literal_expression.hpp"
#include "ast/expressions/literals/integer_literal_expression.hpp"
#include "ast/expressions/literals/string_literal_expression.hpp"
#include "ast/expressions/mathematical_negation_expression.hpp"
#include "ast/expressions/variable_expression.hpp"
#include "ast/function.hpp"
#include "ast/statements/assignment_statement.hpp"
#include "ast/statements/for_statement.hpp"
#include "ast/statements/if_statement.hpp"
#include "ast/statements/return_statement.hpp"
#include "ast/statements/variable_definition_statement.hpp"
#include "codegen/optimizer.hpp"
#include "semantic_analysis/symbol.hpp"
#include "semantic_analysis/symbol_table.hpp"
#include "type_system/type_table.hpp"
#include "utils/string_pool.hpp"
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/TargetParser/Triple.h>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

namespace kepler {

    struct CodegenResult {
        llvm::Value* llvm_value = nullptr;
        bool returns = false;
    };

    class CodegenPass : ASTPass<std::optional<std::vector<std::unique_ptr<llvm::Module>>>> {
    public:
        CodegenPass(SymbolTable& symbol_table,
            const TypeTable& type_table,
            llvm::LLVMContext& context,
            const llvm::Triple& target_triple,
            const llvm::DataLayout& data_layout,
            OptimizationLevel optimization_level)
            : symbol_table(symbol_table),
              type_table(type_table),
              target_triple(target_triple),
              data_layout(data_layout),
              optimization_level(optimization_level),
              context(context),
              builder(context) {}
        std::optional<std::vector<std::unique_ptr<llvm::Module>>> run(std::vector<AbstractSyntaxTree>& ast) override;

    private:
        SymbolTable& symbol_table;
        const TypeTable& type_table;
        const llvm::Triple& target_triple;
        const llvm::DataLayout& data_layout;
        const OptimizationLevel optimization_level;
        llvm::LLVMContext& context;
        llvm::IRBuilder<> builder;
        std::unique_ptr<llvm::Module> llvm_module = nullptr;
        std::unordered_map<SymbolId, llvm::Value*> llvm_values;

        void forward_declare_prototypes(const std::vector<std::unique_ptr<ASTNode>>& nodes);
        void codegen_forward_declaration(const Prototype* prototype);
        void codegen_nodes(const std::vector<std::unique_ptr<ASTNode>>& nodes);
        CodegenResult codegen_node(const ASTNode* node);
        void codegen_function(const Function* function);
        llvm::AllocaInst* create_entry_block_alloca(llvm::Function* function, llvm::Type* type, StringId identifier_id);
        CodegenResult codegen_assignment_statement(const AssignmentStatement* statement);
        CodegenResult codegen_for_statement(const ForStatement* statement);
        CodegenResult codegen_if_statement(const IfStatement* statement);
        CodegenResult codegen_return_statement(const ReturnStatement* statement);
        CodegenResult codegen_variable_definition_statement(const VariableDefinitionStatement* statement);
        CodegenResult codegen_boolean_literal_expression(const BooleanLiteralExpression* expression);
        CodegenResult codegen_floating_point_literal_expression(const FloatingPointLiteralExpression* expression);
        CodegenResult codegen_integer_literal_expression(const IntegerLiteralExpression* expression);
        CodegenResult codegen_string_literal_expression(const StringLiteralExpression* expression);
        CodegenResult codegen_binary_expression(const BinaryExpression* expression);
        CodegenResult codegen_call_expression(const CallExpression* expression);
        CodegenResult codegen_cast_expression(const CastExpression* expression);
        CodegenResult codegen_mathematical_negation_expression(const MathematicalNegationExpression* expression);
        CodegenResult codegen_variable_expression(const VariableExpression* expression);
    };

}
