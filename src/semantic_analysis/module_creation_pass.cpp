// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#include "semantic_analysis/module_creation_pass.hpp"
#include "ast/abstract_syntax_tree.hpp"
#include "ast/statements/import_statement.hpp"
#include "diagnostics/diagnostic.hpp"
#include "diagnostics/source_location.hpp"
#include "semantic_analysis/module.hpp"
#include "utils/assert.h"
#include "utils/string_pool.hpp"
#include <cstddef>
#include <format>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace kepler {

    void ModuleCreationPass::run(std::vector<AbstractSyntaxTree>& asts) {
        KPL_ASSERT_THAT(!asts.empty());
        // Create modules
        for (AbstractSyntaxTree& ast : asts) {
            KPL_ASSERT_NOT_NULLPTR(ast.module_statement);
            KPL_ASSERT_THAT(!ast.module_statement->module_path.part_identifier_ids.empty());
            KPL_ASSERT_THAT(ast.module_statement->module_id == ModuleId::invalid());
            const ModuleId module_id = symbol_table.create_module(ast.module_statement->module_path);
            ast.module_statement->module_id = module_id;
        }

        // Register imported modules
        for (AbstractSyntaxTree& ast : asts) {
            const std::vector<StringId>& module_part_identifier_ids = ast.module_statement->module_path.part_identifier_ids;
            std::vector<std::vector<StringId>> parent_module_paths;
            if (module_part_identifier_ids.size() > 1) {
                for (size_t i = 0; i < module_part_identifier_ids.size() - 1; i++) {
                    // +1 because i is the index and the vector creation needs the size
                    parent_module_paths.push_back(std::vector<StringId>(module_part_identifier_ids.begin(), module_part_identifier_ids.begin() + i + 1));
                }
            }

            // Register explicit imports
            std::vector<size_t> import_indices_to_remove;
            for (size_t i = 0; i < ast.import_statements.size(); i++) {
                const std::unique_ptr<ImportStatement>& import_statement = ast.import_statements[i];
                KPL_ASSERT_NOT_NULLPTR(import_statement);
                KPL_ASSERT_THAT(!import_statement->module_path.part_identifier_ids.empty());
                bool is_implicit_import = false;
                for (const std::vector<StringId>& parent_module_path : parent_module_paths) {
                    if (parent_module_path == import_statement->module_path.part_identifier_ids) {
                        const std::string message = std::format("Module '{}' is implicitely imported because it's a parent module of '{}'. The explicit import is discarded, but consider removing it.",
                            get_full_module_identifier(import_statement->module_path),
                            get_full_module_identifier(ast.module_statement->module_path));
                        diagnostic_sink.report(DiagnosticCode::RedundantImport, std::move(message), import_statement->source_location);
                        import_indices_to_remove.push_back(i);
                        is_implicit_import = true;
                    }
                }
                if (is_implicit_import) {
                    continue;
                }

                const auto module_id = symbol_table.register_imported_module(ast.module_statement->module_id, import_statement->module_path);
                if (!module_id) {
                    diagnostic_sink.report(module_id.error().code, std::move(module_id.error().message), import_statement->source_location);
                    continue;
                }
            }

            // Remove redundant imports
            for (size_t import_index_to_remove : import_indices_to_remove) {
                ast.import_statements.erase(ast.import_statements.begin() + import_index_to_remove);
            }

            // Register implicit imports
            for (size_t i = 1; i < parent_module_paths.size(); i++) {
                const auto module_id = symbol_table.register_imported_module(ast.module_statement->module_id,
                    {.part_identifier_ids = std::move(parent_module_paths[i])});
                KPL_ASSERT_THAT(module_id.has_value());
            }
        }
    }

}
