#include <cassert>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <utility>

#include "../log.hpp"
#include "../ast/prototype.hpp"
#include "function_registry.hpp"

namespace Kepler::FunctionRegistry {

    static std::shared_ptr<AST::Prototype> compiling_prototype;
    static std::map<std::string, std::shared_ptr<AST::Prototype>> prototypes;

    bool register_prototype(std::shared_ptr<AST::Prototype> prototype) {
        if (prototypes.find(prototype->get_name()) != prototypes.end()) {
            const std::vector<AST::ParameterData>& parameters = prototype->get_parameters();

            std::stringstream parameter_signature;
            parameter_signature << '(';
            for (int i = 0; i < parameters.size(); i++) {
                parameter_signature << parameters[i].type << ' ' << parameters[i].name;
                if (i < parameters.size() - 1) {
                    parameter_signature << ", ";
                }
            }
            parameter_signature << ')';


            log(LogStyle::ERROR, "[ Parsing error ]", LogStyle::DEFAULT, ": prototype signature '", prototype->get_type(), " ", prototype->get_name(), parameter_signature.str(), "' is already registered");
            return false;
        }
        prototypes[prototype->get_name()] = std::move(prototype);
        return true;
    }

    std::shared_ptr<AST::Prototype> get_registered_prototype(const std::string& name){
        if (prototypes.find(name) == prototypes.end()) {
            log(LogStyle::WARNING, "[ Compile warning ]", LogStyle::DEFAULT, ": prototype '", name, "' not registered");
            return nullptr;
        }
        return prototypes[name];
    }

    std::shared_ptr<AST::Prototype> get_compiling_prototype() {
        assert(compiling_prototype != nullptr && "[ Assertion ]: trying to access compiling prototype when it is not set");
        return compiling_prototype;
    }

    void set_compiling_prototype(std::shared_ptr<AST::Prototype> prototype) {
        assert((compiling_prototype == nullptr || prototype == nullptr) && "[ Assertion ]: trying to set current prototype when it is already set");
        compiling_prototype = std::move(prototype);
    }

}
