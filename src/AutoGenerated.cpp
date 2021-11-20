#include "AutoGenerated.h"

#include "LinearAlgebra.h"

using namespace AbsSyn;

GiNaC::ex toEx(Expr *e, InputData &m, const GiNaC::lst &vars,
               const GiNaC::lst &params); // converts an Expr to a GiNaC ex

AutoGenerated::AutoGenerated(const InputData &m)
{
  using namespace std;
  using namespace GiNaC;

  this->name = "AutoGenerated";

  for (unsigned i = 0; i < m.getVarNum(); i++)
    this->vars.append(symbol(m.getVar(i)->getName()));

  for (unsigned i = 0; i < m.getParamNum(); i++)
    this->params.append(symbol(m.getParam(i)->getName()));

  for (unsigned i = 0; i < m.getVarNum(); i++)
    this->dyns.append(
        m.getVar(i)->getDynamic()->toEx(m, this->vars, this->params));

  this->reachSet
      = new Bundle(m.getDirections(), m.getUB(), -m.getLB(), m.getTemplate());

  // parameter directions
  if (m.paramDirectionsNum() != 0) {
    vector<vector<double>> pA(2 * m.paramDirectionsNum(),
                              vector<double>(m.getParamNum(), 0));
    for (unsigned i = 0; i < m.paramDirectionsNum(); i++) {
      pA[2 * i] = m.getParamDirection(i);
      for (unsigned j = 0; j < m.getParamNum(); j++)
        pA[2 * i + 1][j] = -pA[2 * i][j];
    }

    vector<double> pb(pA.size(), 0);
    for (unsigned i = 0; i < m.paramDirectionsNum(); i++) {
      pb[2 * i] = m.getParamUB()[i];
      pb[2 * i + 1] = -m.getParamLB()[i];
    }

    this->paraSet = new PolytopesUnion(Polytope(pA, pb));
  }

  // formula
  if (m.isSpecDefined()) {
    this->spec = m.getSpec()->toSTL(m, vars, params);
  } else {
    this->spec = NULL;
  }
}
