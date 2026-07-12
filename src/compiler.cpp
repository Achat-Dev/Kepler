// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "compiler.hpp"
#include "ast/ast_node.hpp"
#include "compilation_context.hpp"
#include "diagnostics/diagnostic_code.hpp"
#include "diagnostics/diagnostic_sink.hpp"
#include "diagnostics/severity.hpp"
#include "io/file.hpp"
#include "lexer/token.hpp"
#include "lexer/tokenizer.hpp"
#include "log.hpp"
#include "parser/parser.hpp"
#include <memory>
#include <print>
#include <vector>

namespace kepler {

    void compile_project(const CompilationContext& context) {
        log::config.should_log_verbose = context.log_verbose;
        log::verbose("Compiling project with the following context:\n{}-i (input file name): '{}'\n{}-o (output file name): '{}", log::styling::indented, context.input_file_path, log::styling::last_indented, context.input_file_path);

        const auto file = io::File::load(context.input_file_path);
        if (!file) {
            diagnostics::Severity severity = diagnostics::get_severity(file.error().code);
            std::println("{}{}", severity, file.error().message);
            exit(1);
        }

        diagnostics::DiagnosticSink diagnostic_sink;

        lexer::Tokenizer tokenizer(*file, diagnostic_sink);
        const std::vector<lexer::Token> tokens = tokenizer.tokenize();

        parser::Parser parser(tokens, (*file).path, diagnostic_sink);
        const std::vector<std::shared_ptr<ast::ASTNode>> ast_nodes = parser.parse();

        if (diagnostic_sink.get_error_count() > 0) {
            diagnostic_sink.flush();
            std::println("{}{}[ This one's on you ]{}{}: Compilation failed with {} error(s) and {} warning(s){}", log::styling::bold, log::styling::bg_red, log::styling::reset, log::styling::bg_red, diagnostic_sink.get_error_count(), diagnostic_sink.get_warning_count(), log::styling::reset);
        }

        /*
        for (const std::shared_ptr<ast::ASTNode> ast_node : *ast_nodes) {
            ast_node->codegen();
        }*/
    }

}
