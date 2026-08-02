// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "compiler.hpp"
#include "assert.hpp"
#include "ast/abstract_syntax_tree.hpp"
#include "ast/ast_node.hpp"
#include "ast/ast_printer.hpp"
#include "compilation_context.hpp"
#include "diagnostics/diagnostic.hpp"
#include "diagnostics/diagnostic_sink.hpp"
#include "io/file.hpp"
#include "lexer/tokenizer.hpp"
#include "log.hpp"
#include "parser/parser.hpp"
#include "semantic_analysis/name_resolution_pass.hpp"
#include "semantic_analysis/symbol_table.hpp"
#include "type_system/type_check_pass.hpp"
#include "type_system/type_table.hpp"
#include <filesystem>
#include <memory>
#include <print>

namespace kepler {

    namespace {

#if NDEBUG
        void verify_ast(const AbstractSyntaxTree& ast) {
            return;
        }
#else
        void verify_ast(const AbstractSyntaxTree& ast) {
            for (const std::unique_ptr<ASTNode>& node : ast.nodes) {
                KPL_ASSERT(node->node_type == ASTNodeType::Extern || node->node_type == ASTNodeType::Function,
                    "Behold: I somehow managed to create a malformed AST with a node of type '{}' on the top level <(˘ ˘ ˘)>",
                    node->node_type);
            }
        }
#endif

    }

    void compile_project(const CompilationContext& context) {
        log::config.should_log_verbose = context.log_verbose;
        log::verbose("Compiling project with the following context:\n{}-i (input file name): '{}'\n{}-o (output file name): '{}",
            log::styling::indented,
            context.input_file_path,
            log::styling::last_indented,
            context.input_file_path);

        const auto file = File::load(context.input_file_path);
        if (!file) {
            DiagnosticSeverity severity = get_diagnostic_severity(file.error().code);
            std::println("{}{}", severity, file.error().message);
            exit(1);
        }

        DiagnosticSink diagnostic_sink;
        SymbolTable symbol_table;
        TypeTable type_table;

        // AST creation
        Tokenizer tokenizer(*file, diagnostic_sink, type_table);
        Parser parser(tokenizer.tokenize(), *file, diagnostic_sink, type_table);
        AbstractSyntaxTree ast = parser.parse();
        verify_ast(ast);
        ASTPrinter ast_printer(ast);
        ast_printer.run();

        // AST passes
        // Currently there are only builtin types so there is no need to do any type resolution before name resolution
        // Once user defined types are implemented the first pass should be a type resolution pass which creates the type objects for all user defined types
        NameResolutionPass name_resolution_pass(ast, diagnostic_sink, symbol_table, type_table);
        name_resolution_pass.run();
        ast_printer.run();
        TypeCheckPass type_check_pass(ast, diagnostic_sink, symbol_table, type_table);
        type_check_pass.run();

        if (diagnostic_sink.get_error_count() > 0) {
            diagnostic_sink.flush();
            std::println("{}{}[ This one's on you ]{}{}: Compilation failed with {} error(s) and {} warning(s){}",
                log::styling::bold,
                log::styling::bg_red,
                log::styling::reset,
                log::styling::bg_red,
                diagnostic_sink.get_error_count(),
                diagnostic_sink.get_warning_count(),
                log::styling::reset);
        }
    }

}
