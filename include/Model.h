/**
 * @file Model.h
 * Represent a discrete-time (eventually parameteric) dynamical system
 *
 * @author Tommaso Dreossi <tommasodreossi@berkeley.edu>
 * @version 0.1
 */

#ifndef MODEL_H_
#define MODEL_H_

#include "SymbolicAlgebra.h"
#include "Bundle.h"
#include "PolytopesUnion.h"
#include "STL.h"

class Model
{

protected:
  std::vector<SymbolicAlgebra::Symbol<>> vars;     //!< variables
  std::vector<SymbolicAlgebra::Symbol<>> params;   //!< parameters
  std::vector<SymbolicAlgebra::Expression<>> dyns; //!< dynamics

  Bundle *reachSet; // Initial reach set
  PolytopesUnion *paraSet;

  std::shared_ptr<STL> spec;

  std::string name;

public:
  const std::string &getName() const
  {
    return this->name;
  }

  const std::vector<SymbolicAlgebra::Symbol<>> &getVars() const
  {
    return this->vars;
  }

  const std::vector<SymbolicAlgebra::Symbol<>> &getParams() const
  {
    return this->params;
  }

  const std::vector<SymbolicAlgebra::Expression<>> &getDyns() const
  {
    return this->dyns;
  }

  const Bundle *getReachSet() const
  {
    return this->reachSet;
  }
  const PolytopesUnion *getParaSet() const
  {
    return this->paraSet;
  }
  const std::shared_ptr<STL> getSpec() const
  {
    return this->spec;
  }

  ~Model();
};

#endif /* MODEL_H_ */
