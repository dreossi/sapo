#include "InputData.h"

#include <glpk.h>

using namespace std;

namespace AbsSyn
{

InputData::InputData():
    problem(problemType::P_UNDEF), varMode(modeType::M_UNDEF),
    paramMode(modeType::M_UNDEF), iterations(0), iter_set(false),
    max_param_splits(0), presplits(false),
    max_bundle_magnitude(std::numeric_limits<double>::max()), vars(), params(),
    consts(), defs(), assumptions(), spec(NULL), directions(),
    templateMatrix(), paramDirections(), trans(transType::T_UNDEF),
    decomp(false), decomp_defined(false), alpha(0.5), alphaDefined(false),
    compose_dynamic(false), dynamic_degree(1)
{
}

InputData::~InputData()
{
  for (auto it = std::begin(vars); it != std::end(vars); ++it)
    delete *it;

  for (auto it = std::begin(params); it != std::end(params); ++it)
    delete *it;

  for (auto it = std::begin(consts); it != std::end(consts); ++it)
    delete *it;

  for (auto it = std::begin(defs); it != std::end(defs); ++it)
    delete *it;

  for (auto it = std::begin(assumptions); it != std::end(assumptions); ++it)
    delete *it;

  for (auto it = std::begin(directions); it != std::end(directions); ++it)
    delete *it;

  for (auto it = std::begin(paramDirections); it != std::end(paramDirections);
       ++it)
    delete *it;

  spec.reset();
}
ostream &operator<<(ostream &os, const InputData &m)
{
  os << "Problem: " << m.problem << endl;
  os << "Iterations: " << m.iterations << endl;

  os << endl;
  os << "Variables: " << endl;
  for (unsigned i = 0; i < m.vars.size(); i++) {
    os << "\t" << m.vars[i]->getName() << ": " << m.vars[i]->getDynamic()
       << endl;
  }
  os << endl;

  os << "Parameters: " << endl;
  for (unsigned i = 0; i < m.params.size(); i++) {
    os << "\t" << m.params[i]->getName() << endl;
  }
  os << endl;

  os << "Constants: " << endl;
  for (unsigned i = 0; i < m.consts.size(); i++) {
    os << "\t" << m.consts[i]->getName() << " = " << m.consts[i]->getValue()
       << endl;
  }
  os << endl;

  os << "Defines: " << endl;
  for (unsigned i = 0; i < m.defs.size(); i++) {
    os << "\t" << m.defs[i]->getName() << " = " << m.defs[i]->getValue()
       << endl;
  }
  os << endl;

  os << "spec: ";
  if (m.spec == NULL)
    os << "NULL" << endl;
  else
    os << *(m.spec) << endl;
  os << endl;

  os << "assumptions: ";
  for (unsigned i = 0; i < m.assumptions.size(); i++)
    os << *(m.assumptions[i]) << endl;
  os << endl;

  os << endl;
  os << "Directions:" << endl << "{" << endl;
  for (unsigned i = 0; i < m.directions.size(); i++) {
    os << "\t" << *(m.directions[i]) << endl;
  }
  //    os << "\t<" << m.directions[i] << "> in [" << m.LBoffsets[i] << ", "
  //       << m.UBoffsets[i] << "]" << endl;
  os << "}" << endl;

  os << endl;
  os << "Template:" << endl << "{" << endl;
  for (unsigned i = 0; i < m.templateMatrix.size(); i++)
    os << "\t{" << m.templateMatrix[i] << "}" << endl;
  os << "}" << endl;

  os << endl;
  os << "Parameter directions:" << endl << "{" << endl;
  for (unsigned i = 0; i < m.paramDirections.size(); i++) {
    os << *(m.paramDirections[i])
       << (i == m.paramDirections.size() - 1 ? "" : ",") << endl;
  }
  os << "}" << endl;

  return os;
}

bool InputData::isVarDefined(const string &name) const
{
  for (unsigned i = 0; i < vars.size(); i++)
    if (vars[i]->getName() == name)
      return true;

  return false;
}

bool InputData::isParamDefined(const string &name) const
{
  for (unsigned i = 0; i < params.size(); i++)
    if (params[i]->getName() == name)
      return true;

  return false;
}

bool InputData::isConstDefined(const string &name) const
{
  for (unsigned i = 0; i < consts.size(); i++)
    if (consts[i]->getName() == name)
      return true;

  return false;
}

bool InputData::isDefDefined(const string &name) const
{
  for (unsigned i = 0; i < defs.size(); i++)
    if (defs[i]->getName() == name)
      return true;

  return false;
}

bool InputData::isDirectionDefined(const std::string &name) const
{
  for (unsigned i = 0; i < directions.size(); i++) {
    if (directions[i]->getName() == name) {
      return true;
    }
  }
  return false;
}

bool InputData::isSymbolDefined(const string &name) const
{
  return isVarDefined(name) || isParamDefined(name) || isConstDefined(name)
         || isDefDefined(name) || isDirectionDefined(name);
}

SymbolicAlgebra::Symbol<> InputData::getSymbol(std::string name) const
{
  for (unsigned i = 0; i < vars.size(); i++) {
    if (vars[i]->getName() == name) {
      return vars[i]->getSymbol();
    }
  }

  for (unsigned i = 0; i < params.size(); i++) {
    if (params[i]->getName() == name) {
      return params[i]->getSymbol();
    }
  }

  for (unsigned i = 0; i < consts.size(); i++) {
    if (consts[i]->getName() == name) {
      return consts[i]->getSymbol();
    }
  }

  for (unsigned i = 0; i < defs.size(); i++) {
    if (defs[i]->getName() == name) {
      return defs[i]->getSymbol();
    }
  }

  for (unsigned i = 0; i < directions.size(); i++) {
    if (directions[i]->getName() == name) {
      return *directions[i]->getSymbol();
    }
  }

  for (unsigned i = 0; i < paramDirections.size(); i++) {
    if (paramDirections[i]->getName() == name) {
      return *paramDirections[i]->getSymbol();
    }
  }

  throw std::runtime_error("No symbol named \"" + name + "\"");
}

const Variable *InputData::getVar(const string &name) const
{
  for (unsigned i = 0; i < vars.size(); i++)
    if (vars[i]->getName() == name)
      return vars[i];

  return NULL;
}

Variable *InputData::getVar(const string &name)
{
  for (unsigned i = 0; i < vars.size(); i++)
    if (vars[i]->getName() == name)
      return vars[i];

  return NULL;
}

int InputData::getVarPos(const string &name) const
{
  for (unsigned i = 0; i < vars.size(); i++)
    if (vars[i]->getName() == name)
      return i;

  return -1;
}

std::vector<SymbolicAlgebra::Symbol<>> InputData::getVarSymbols() const
{
  std::vector<SymbolicAlgebra::Symbol<>> res{};

  for (unsigned i = 0; i < vars.size(); i++) {
    res.push_back(vars[i]->getSymbol());
  }

  return res;
}

const Parameter *InputData::getParam(const string &name) const
{
  for (unsigned i = 0; i < params.size(); i++)
    if (params[i]->getName() == name)
      return params[i];

  return NULL;
}

int InputData::getParamPos(const string &name) const
{
  for (unsigned i = 0; i < params.size(); i++)
    if (params[i]->getName() == name)
      return i;

  return -1;
}

std::vector<SymbolicAlgebra::Symbol<>> InputData::getParamSymbols() const
{
  std::vector<SymbolicAlgebra::Symbol<>> res{};

  for (unsigned i = 0; i < params.size(); i++) {
    res.push_back(params[i]->getSymbol());
  }

  return res;
}

const Constant *InputData::getConst(const string &name) const
{
  for (unsigned i = 0; i < consts.size(); i++)
    if (consts[i]->getName() == name)
      return consts[i];

  return NULL;
}

const Definition *InputData::getDef(const string &name) const
{
  // TODO: replaced this and analoguous code by using maps
  for (unsigned i = 0; i < defs.size(); i++)
    if (defs[i]->getName() == name)
      return defs[i];

  return NULL;
}

int InputData::getDefPos(const string &name) const
{
  for (unsigned i = 0; i < defs.size(); i++)
    if (defs[i]->getName() == name)
      return i;

  return -1;
}

int find(std::vector<Direction *> M, Direction *v)
{
  unsigned pos = 0;
  while (pos < M.size()) {
    if (M[pos]->compare(v)) {
      return pos;
    } else {
      pos++;
    }
  }
  // not found
  return -1;
}

void InputData::addDirectionConstraint(Direction *d, bool isVar)
{
  Direction *new_dir = d;
  Direction *negated_dir = new_dir->getComplementary();

  std::vector<AbsSyn::Direction *> *dirs;
  if (isVar) {
    dirs = &directions;
  } else {
    dirs = &paramDirections;
  }

  // check if new direction is already present
  int pos = find(*dirs, new_dir);
  int negated_pos = find(*dirs, negated_dir);

  if (pos != -1) {

    // direction already present
    if (!(*dirs)[pos]->hasUB() || new_dir->getUB() < (*dirs)[pos]->getUB()) {
      (*dirs)[pos]->setUB(new_dir->getUB());
      (*dirs)[pos]->setSymbol(new_dir->getSymbol());
    }
    if (!(*dirs)[pos]->hasLB() || new_dir->getLB() > (*dirs)[pos]->getLB()) {
      (*dirs)[pos]->setLB(new_dir->getLB());
      (*dirs)[pos]->setSymbol(new_dir->getSymbol());
    }
    delete (new_dir);
    delete (negated_dir);

  } else if (negated_pos != -1) {

    // negation of direction already present
    if (!(*dirs)[negated_pos]->hasUB()
        || negated_dir->getUB() < (*dirs)[negated_pos]->getUB()) {
      (*dirs)[negated_pos]->setUB(negated_dir->getUB());
      (*dirs)[negated_pos]->setSymbol(negated_dir->getSymbol());
    }
    if (!(*dirs)[negated_pos]->hasLB()
        || negated_dir->getLB() > (*dirs)[negated_pos]->getLB()) {
      (*dirs)[negated_pos]->setLB(negated_dir->getLB());
      (*dirs)[negated_pos]->setSymbol(negated_dir->getSymbol());
    }
    delete (new_dir);
    delete (negated_dir);

  } else {

    // new direction
    (*dirs).push_back(new_dir);

    if (isVar) {
      // cover variables
      for (unsigned i = 0; i < vars.size(); i++) {
        if (new_dir->covers(vars[i]->getSymbol())) {
          vars[i]->setCovered();
        }
      }
    } else {
      // cover parameter
      for (unsigned i = 0; i < params.size(); i++) {
        if (new_dir->covers(params[i]->getSymbol())) {
          params[i]->setCovered();
        }
      }
    }

    delete (negated_dir);
  }
}

void InputData::addVarDirectionConstraint(Direction *d)
{
  return this->addDirectionConstraint(d, true);
}

int InputData::findDirectionPos(const std::string &name) const
{
  for (unsigned i = 0; i < directions.size(); i++) {
    if (directions[i]->getName() == name) {
      return i;
    }
  }
  return -1;
}

void InputData::addParamDirectionConstraint(Direction *d)
{
  return this->addDirectionConstraint(d, false);
}

double typeCoeff(const Direction::Type &type)
{
  if (type == AbsSyn::Direction::Type::GE
      || type == AbsSyn::Direction::Type::GT) {
    return -1;
  }

  return 1;
}

/**
 * @brief Build a constraints linear system from a set of constraints
 *
 * @tparam T is the type of the symbol domain
 * @param constraints the set of constraints
 * @param symbols the order of the constraint symbols in the linear system
 * @return A linear system representing the provided linear systems
 */
template<typename T>
LinearSystem
getConstraintsSystem(const std::vector<Direction *> &constraints,
                     const std::vector<SymbolicAlgebra::Symbol<T>> &symbols)
{
  using namespace LinearAlgebra;

  vector<vector<T>> A{};
  vector<T> b{};

  for (auto dir_it = std::cbegin(constraints);
       dir_it != std::cend(constraints); ++dir_it) {
    const AbsSyn::Direction &dir = **dir_it;

    double coeff = typeCoeff(dir.getType());
    auto systemRow = dir.getConstraintVector(symbols);
    // add only bounded constraints
    if (dir.hasUB()) {
      A.push_back(coeff * systemRow);
      b.push_back(coeff * dir.getUB());
    }
    if (dir.hasLB()) {
      A.push_back(-coeff * systemRow);
      b.push_back(-coeff * dir.getLB());
    }
  }
  return LinearSystem(A, b);
}

/**
 * @brief Optimize the boundaries of a constraints set
 *
 * @tparam T is the type of the symbol domain
 * @param constraints the constraint set whose boundaries we want to be
 * optimized
 * @param symbols the array of the symbols in the constraint set
 */
template<typename T>
void optimizeConstraintsBoundaries(
    std::vector<Direction *> &constraints,
    const std::vector<SymbolicAlgebra::Symbol<>> &symbols)
{
  using namespace LinearAlgebra;

  // get the linear system associated to the constraint set
  LinearSystem constrSystem = getConstraintsSystem(constraints, symbols);

  // for each constraint
  for (auto c_it = std::begin(constraints); c_it != std::end(constraints);
       ++c_it) {
    AbsSyn::Direction &constr = **c_it;

    // coeff belongs to {1,-1} and depends on the
    // constrain type
    const double coeff = typeCoeff(constr.getType());

    std::vector<double> constrVector
        = coeff * constr.getConstraintVector(symbols);

    OptimizationResult<double> opt_res = constrSystem.minimize(constrVector);

    if (opt_res.status() == GLP_NOFEAS) {
      throw std::domain_error("Infeasable system");
    }

    constr.setLB(opt_res.optimum());

    opt_res = constrSystem.maximize(constrVector);

    if (opt_res.status() == GLP_NOFEAS) {
      throw std::domain_error("Infeasable system");
    }
    constr.setUB(opt_res.optimum());
  }
}

void InputData::optimize_boundaries()
{
  // optimize initial set
  try {
    optimizeConstraintsBoundaries<double>(directions, this->getVarSymbols());
  } catch (std::domain_error &e) {
    // the input set is empty
    // this condition is admitted
  }

  // optimize initial parameter set
  // if the parameter set is empty a domain_error exception is thrown
  // as this condition is not admitted
  optimizeConstraintsBoundaries<double>(paramDirections,
                                        this->getParamSymbols());
}

/**
 * @brief Check whether the symbols are bounded by the constraints
 *
 * @tparam T
 * @param what is a textual description of the investigated symbols
 * @param constraints is a set of constraints
 * @param symbols is the set of symbols in the constraints
 * @return true if and only if all the symbols are bounded
 */
template<typename T>
bool checkFiniteBounds(const char *what, std::vector<Direction *> &constraints,
                       const std::vector<SymbolicAlgebra::Symbol<>> &symbols)
{
  if (constraints.size() == 0) {
    return true;
  }

  bool result = true;
  // get the linear system associated to the constraint set
  LinearSystem constrSystem = getConstraintsSystem(constraints, symbols);

  std::vector<T> sym_array(symbols.size(), 0);

  // for each symbol
  for (unsigned int i = 0; i < sym_array.size(); ++i) {
    sym_array[i] = 1;

    // check whether it is lower bounded
    if (constrSystem.minimize(sym_array).optimum()
        == -std::numeric_limits<T>::infinity()) {
      std::cerr << what << " " << symbols[i] << " has no finite lower bound"
                << std::endl;
      result = false;
    }

    // check whether it is bounded
    if (constrSystem.maximize(sym_array).optimum()
        == std::numeric_limits<T>::infinity()) {
      std::cerr << what << " " << symbols[i] << " has no finite upper bound"
                << std::endl;
      result = false;
    }

    sym_array[i] = 0;
  }

  return result;
}

bool InputData::check()
{
  bool res = true;

  if (!isProblemDefined()) {
    cerr << "Problem type must be defined" << endl;
    res = false;
  }

  if (!isIterationSet()) {
    cerr << "Number of iterations is mandatory" << endl;
    res = false;
  }

  // each variable must have its dynamic
  for (unsigned i = 0; i < vars.size(); i++) {
    if (!vars[i]->isDynamicDefined()) {
      cerr << "Variable " << vars[i]->getName() << " has not a dynamic"
           << endl;
      res = false;
    }
  }

  auto var_symbols = this->getVarSymbols();
  if (!checkFiniteBounds<double>("Variable", directions, var_symbols)) {
    res = false;
  }

  if (!checkFiniteBounds<double>("Parameter", paramDirections,
                                 this->getParamSymbols())) {
    res = false;
  }

  // each template row must be bounded
  {
    for (unsigned i = 0; i < templateMatrix.size(); i++) {
      vector<vector<double>> M{};
      for (unsigned j = 0; j < templateMatrix[i].size(); j++) {
        M.push_back(directions[templateMatrix[i][j]]->getConstraintVector(
            var_symbols));
      }

      if (LinearAlgebra::Dense::rank(M) != templateMatrix[i].size()) {
        // directions are dependent, parallelotope is not bounded
        cerr << "Template row " << templateMatrix[i]
             << " defines an unbounded parallelotope" << endl;
        res = false;
      }
    }
  }

  // specs
  if (problem == problemType::SYNTH && spec == NULL) {
    cerr << "If problem is synthesis, a formula must be provided as spec"
         << endl;
    res = false;
  }

  // trans type (AFO or OFO), if undefined set to AFO
  if (trans == transType::T_UNDEF) {
    trans = transType::AFO;
  }

  return res;
}

} // end namespace AbsSyn
