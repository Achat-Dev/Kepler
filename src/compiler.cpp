// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "compiler.hpp"
#include "ast/abstract_syntax_tree.hpp"
#include "compilation_context.hpp"
#include "diagnostics/diagnostic_code.hpp"
#include "diagnostics/diagnostic_sink.hpp"
#include "diagnostics/severity.hpp"
#include "io/file.hpp"
#include "lexer/token.hpp"
#include "lexer/tokenizer.hpp"
#include "log.hpp"
#include "parser/parser.hpp"
#include "semantic_analysis/semantic_analysis_pass.hpp"
#include "semantic_analysis/symbol_table.hpp"
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

        parser::Parser parser(tokens, *file, diagnostic_sink);
        const ast::AbstractSyntaxTree ast = parser.parse();

        semantic_analysis::SymbolTable symbol_table;
        semantic_analysis::SemanticAnalysisPass semantic_analysis_pass(ast, diagnostic_sink, symbol_table);
        semantic_analysis_pass.run();

        if (diagnostic_sink.get_error_count() > 0) {
            diagnostic_sink.flush();
            std::println("{}{}[ This one's on you ]{}{}: Compilation failed with {} error(s) and {} warning(s){}", log::styling::bold, log::styling::bg_red, log::styling::reset, log::styling::bg_red, diagnostic_sink.get_error_count(), diagnostic_sink.get_warning_count(), log::styling::reset);
        }
    }

}
