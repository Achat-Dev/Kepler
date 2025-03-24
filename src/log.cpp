#include "log.hpp"

namespace Kepler {

    const std::unique_ptr<AST::Expression> log_error(const std::string& message) {
        std::fprintf(stderr, "Error: %s\n", message.c_str());
        return nullptr;
    }

    const std::unique_ptr<AST::Prototype> log_errorp(const std::string& message) {
        log_error(message);
        return nullptr;
    }

    llvm::Value* log_errorv(const std::string& message) {
        log_error(message);
        return nullptr;
    }

}
