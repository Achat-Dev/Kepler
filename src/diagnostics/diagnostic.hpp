// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2026 Lion Schulz
 *
 * This file is part of the compiler for the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "diagnostics/source_location.hpp"
#include "utils/ansi_codes.hpp"
#include "utils/assert.h"
#include <format>
#include <string>

namespace kepler {

    enum class DiagnosticCode {
        // Usage
        HelpRequested = 100,
        NoInputFile,
        NoOutputFile,
        TooManyInputFiles,
        TooManyOutputFiles,
        UnknownOption,

        // I/O
        FileNotFound = 200,
        FileIsADirectory,
        NotARegularFile,
        FailedToCreateFileStream,

        // Lexer
        UnknownCharacter = 300,
        UnknownEscapeSequence,
        MultilineCommentNotClosed,

        // Parser
        UnexpectedToken = 400,
        InvalidCast,
        InvalidReturn,
        InvalidVariableType,
        InvalidLoopVariableType,
        InvalidMathematicalNegation,
        MissingEndKeyword,
        UsingStatementAsExpression,

        // Semantic analysis
        UndefinedSymbol = 500,
        SymbolAlreadyExists,
        UnknownType,
        InvalidFunctionCall,
        TypeMismatch,
        UnsupportedMathematicalOperation,
        RedundantCast,

        Unsupported = 999,
    };

    enum class DiagnosticSeverity {
        Note,
        Warning,
        Error,
        Unsupported,
    };

    struct Diagnostic {
        DiagnosticCode code;
        std::string message;
    };

    struct SourceDiagnostic {
        DiagnosticCode code;
        std::string message;
        SourceLocation source_location;
    };

    DiagnosticSeverity get_diagnostic_severity(DiagnosticCode diagnostic_code);

}

template <>
struct std::formatter<kepler::DiagnosticSeverity> : std::formatter<std::string> {
    auto format(const kepler::DiagnosticSeverity& severity, std::format_context& ctx) const {
        switch (severity) {
            case kepler::DiagnosticSeverity::Note:
                return std::formatter<std::string>::format(std::format("{}[ Note ]{}: ",
                                                               kepler::ansi_codes::bold,
                                                               kepler::ansi_codes::reset),
                    ctx);
            case kepler::DiagnosticSeverity::Warning:
                return std::formatter<std::string>::format(std::format("{}{}[ Warning ]{}: ",
                                                               kepler::ansi_codes::bold,
                                                               kepler::ansi_codes::bg_yellow,
                                                               kepler::ansi_codes::reset),
                    ctx);
            case kepler::DiagnosticSeverity::Error:
                return std::formatter<std::string>::format(std::format("{}{}[ Error ]{}: ",
                                                               kepler::ansi_codes::bold,
                                                               kepler::ansi_codes::bg_red,
                                                               kepler::ansi_codes::reset),
                    ctx);
            case kepler::DiagnosticSeverity::Unsupported:
                return std::formatter<std::string>::format(std::format("{}{}[ Unpaid developer error ]{}: ",
                                                               kepler::ansi_codes::bold,
                                                               kepler::ansi_codes::bg_magenta,
                                                               kepler::ansi_codes::reset),
                    ctx);
        }

        kepler::assert::unreachable(std::format("Missing format implementation for severity '{}'", static_cast<int>(severity)));
    }
};
