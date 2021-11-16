#ifndef AUTOGENERATED_H_
#define AUTOGENERATED_H_

#include <math.h>
#include <glpk.h>

#include "AbsSyn.h"
#include "Always.h"
#include "Atom.h"
#include "Conjunction.h"
#include "Disjunction.h"
#include "Eventually.h"
#include "Model.h"
#include "STL.h"
#include "Until.h"

#include "LinearSystem.h"

class AutoGenerated : public Model
{

private:
public:
  AutoGenerated(const AbsSyn::InputData &m);
  ~AutoGenerated();
};
#endif
