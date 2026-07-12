// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (c) 2025 Lion Schulz
 *
 * This file is part of the Kepler programming language, which is licensed under the GNU Affero General Public License version 3 or later.
 * You should have received a copy of the license along with this program.
 * If not, see <https://www.gnu.org/licenses/>
 */

#pragma once

#include "diagnostics/severity.hpp"

namespace kepler::diagnostics {

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
        InvalidReturnExpression,
        InvalidVariableType,
        InvalidLoopVariableType,
        InvalidMathematicalNegation,
        MissingEndKeyword,
        UsingStatementAsExpression,

        // Semantic analysis
        UndefinedSymbol = 500,
        SymbolAlreadyExists,

        Unsupported = 999,
    };

    Severity get_severity(DiagnosticCode diagnostic_code);

}
