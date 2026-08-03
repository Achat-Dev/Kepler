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
#include "log.hpp"
#include <cassert>
#include <format>
#include <string>
#include <utility>

namespace kepler {

    enum class DiagnosticCode {
        // Usage
        HelpRequested = 100,
        NoInputFile,
        NoOutputFile,
        TooManyInputFiles,
        TooManyOutputFiles,
        CxxoptsException,

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
                                                               kepler::log::styling::bold,
                                                               kepler::log::styling::reset),
                    ctx);
            case kepler::DiagnosticSeverity::Warning:
                return std::formatter<std::string>::format(std::format("{}{}[ Warning ]{}: ",
                                                               kepler::log::styling::bold,
                                                               kepler::log::styling::bg_yellow,
                                                               kepler::log::styling::reset),
                    ctx);
            case kepler::DiagnosticSeverity::Error:
                return std::formatter<std::string>::format(std::format("{}{}[ Error ]{}: ",
                                                               kepler::log::styling::bold,
                                                               kepler::log::styling::bg_red,
                                                               kepler::log::styling::reset),
                    ctx);
            case kepler::DiagnosticSeverity::Unsupported:
                return std::formatter<std::string>::format(std::format("{}{}[ Unpaid developer error ]{}: ",
                                                               kepler::log::styling::bold,
                                                               kepler::log::styling::bg_magenta,
                                                               kepler::log::styling::reset),
                    ctx);
        }

        assert(false && "Missing format implementation for severity");
        std::unreachable();
    }
};
