// Symbolic algebras routines (simple wrapper around SymEngine)
//  - takes a math expression as a string
//     - with list of variables
//     - with list of constants and their numeric values
//  - parses expression
//  - returns simplified expression with constants inlined as string
//  - returns differential of expression wrt a variable as string

#pragma once

#include <map>
#include <string>
#include <vector>

#include "symengine/basic.h"
#include "symengine/symbol.h"
#include "symengine/symengine_rcp.h"

namespace symbolic {

class Symbolic {
 public:
  Symbolic(const std::string &expression,
           const std::vector<std::string> &variables,
           const std::map<std::string, double> &constants);
  // simplify given expression
  std::string simplify() const;
  // differentiate given expression wrt a variable
  std::string diff(const std::string &var) const;
  // relabel variables as u_0, u_1, etc in order
  std::string relabel(const std::vector<std::string> &variables,
                      const std::string &label = "u_") const;

 private:
  SymEngine::RCP<const SymEngine::Basic> expr;
  std::map<std::string, SymEngine::RCP<const SymEngine::Symbol>> symbols;
  std::string toString(
      const SymEngine::RCP<const SymEngine::Basic> &expr) const;
};

}  // namespace symbolic