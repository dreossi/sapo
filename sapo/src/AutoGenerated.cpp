#include "AutoGenerated.h"

#include "SymbolicAlgebra.h"
#include "LinearAlgebra.h"

using namespace AbsSyn;

// ALL THE CODE IN THIS FILE MUST BE REVIEWED

// creates the bundle defined by the model, adding directions and template rows
// according to assumptions
Bundle *getBundleWithAssumptions(const InputData &id);

// return i s.t. M[i] = v, or -1 if M does not contain v
int find(const std::vector<std::vector<double>> M,
         const std::vector<double> v);

// computes a vector having the same direction and length 1
std::vector<double> normalize(std::vector<double> v);

// computes the multiplicative coefficient to normalize vector
double getNormalizationCoefficient(std::vector<double> v);

// given a vector v1 and an offset val, return the new offset of the same
// constraint rescaled wrt v2
double rescale(double val, std::vector<double> v1, std::vector<double> v2);

// checks if two vectors are equal
bool compare(std::vector<double> v1, std::vector<double> v2, double tol);

// adds to lp the constraints that each polytope has exactly as many directions
// as the number of vars
void paralCardConstraints(glp_prob **lp,
                          const std::vector<std::vector<double>> A,
                          const std::vector<std::vector<double>> C,
                          unsigned *startingIndex, unsigned Pn);

// adds to lp the constraints that each variable is covered in each polytope
void varCoverConstraints(glp_prob **lp,
                         const std::vector<std::vector<double>> A,
                         const std::vector<std::vector<double>> C,
                         unsigned *startingIndex, unsigned Pn);

// adds to lp the constraints that each direction is used
void directionUseConstraints(glp_prob **lp,
                             const std::vector<std::vector<double>> A,
                             const std::vector<std::vector<double>> C,
                             unsigned *startingIndex, unsigned Pn);

// adds to lp the constraints that fix directions in parallelotopes according
// to old template
void oldTemplateConstraints(glp_prob **lp,
                            const std::vector<std::vector<double>> A,
                            const std::vector<std::vector<double>> C,
                            unsigned *startingIndex,
                            std::vector<std::vector<int>> oldTemplate,
                            unsigned Pn);

// adds to lp the constraints that each parallelotope has linearly independent
// rows
void independenceConstraints(glp_prob **lp,
                             const std::vector<std::vector<double>> A,
                             const std::vector<std::vector<double>> C,
                             unsigned *startingIndex, unsigned Pn);

// map_parals a direction and a Parallelotope to the corresponding variable
int map_paral(int d, int P, int Pn)
{
  return d * (Pn) + P;
}

double findDirectionBound(std::vector<std::vector<double>> A,
                          std::vector<double> LB, std::vector<double> UB,
                          std::vector<double> dir, bool minimize);

/*
 * This method computes a new template for the polytope.
 * The idea is that we want each parallelotope to contain directions covering
 * each variable, so that they are not singular. Moreover, each direction
 * should be used at least once.
 *
 * We use a LP problem to compute which direction goes in which parallelotope.
 * We define boolean variables X_i_j where i is a direction and j is a
 * parallelotope, meaning that the direction i is contained in the
 * parallelotope j.
 *
 * We add four families of constraints:
 *  - we bound each varaible to be in [0, 1]
 *  - we require that each parallelotope has the correct number of directions
 *  - we require that each variable v of the dynamical system is covered in
 * each parallelotope
 *  - we require that each direction d appears in at least one parallelotope.
 *
 * We also require that the user-specified template is included in the
 * solution.
 *
 * The objective function is simply the sum of all variables, since we are
 * interested in satisfiability and not in optimization
 */
std::vector<std::vector<int>>
computeTemplate(const std::vector<std::vector<double>> A,
                const std::vector<std::vector<double>> C,
                const std::vector<std::vector<int>> oldTemplate);

AutoGenerated::AutoGenerated(const InputData &id): Model()
{
  using namespace std;
  using namespace SymbolicAlgebra;

  this->name = "AutoGenerated";

  for (unsigned i = 0; i < id.getVarNum(); i++) {
    this->vars.push_back(id.getVar(i)->getSymbol());
  }

  for (unsigned i = 0; i < id.getParamNum(); i++) {
    this->params.push_back(id.getParam(i)->getSymbol());
  }

  // compose dynamics
  if (id.isDynamicCompositionEnabled() && id.getDynamicDegree() > 1) {
    // new dynamics, start as a copy of the originals
    std::vector<SymbolicAlgebra::Expression<>> new_dyns{};
    for (unsigned v = 0; v < id.getVarNum(); v++) {
      new_dyns.push_back(simplify(id.getVar(v)->getDynamic()));
    }

    // new symbols for variables, to perform replacement
    std::vector<SymbolicAlgebra::Symbol<>> new_var_symbols{};
    for (unsigned v = 0; v < vars.size(); v++) {
      std::string name
          = SymbolicAlgebra::Symbol<>::get_symbol_name(vars[v].get_id());
      name += "_2";
      SymbolicAlgebra::Symbol<> s(name);
      new_var_symbols.push_back(SymbolicAlgebra::Symbol<>(s));
    }

    for (unsigned i = 1; i < id.getDynamicDegree(); i++) {
      for (unsigned v = 0; v < id.getVarNum(); v++) {

        // replace each variable with its new symbol
        SymbolicAlgebra::Expression<>::replacement_type rep_symb{};
        for (unsigned var = 0; var < vars.size(); var++) {
          rep_symb[(SymbolicAlgebra::Symbol<>)vars[var]]
              = new_var_symbols[var];
        }
        new_dyns[v].replace(rep_symb);

        // replace new symbol with dynamic for variable
        SymbolicAlgebra::Expression<>::replacement_type rep_dyn{};
        for (unsigned var = 0; var < vars.size(); var++) {
          rep_dyn[new_var_symbols[var]] = id.getVar(var)->getDynamic();
        }
        new_dyns[v].replace(rep_dyn).expand();
        // new_dyns[v] = simplify(new_dyns[v]);
      }
    }

    for (unsigned v = 0; v < id.getVarNum(); v++) {
      new_dyns[v] = simplify(new_dyns[v]);
    }

    this->dyns = new_dyns;
  } else {
    for (unsigned v = 0; v < id.getVarNum(); v++) {
      SymbolicAlgebra::Expression<> e = id.getVar(v)->getDynamic();
      e.expand();
      e = simplify(e);

      this->dyns.push_back(e);
    }
  }

  this->reachSet = getBundleWithAssumptions(id);

  // parameter directions
  if (id.paramDirectionsNum() != 0) {
    vector<vector<double>> pA(2 * id.paramDirectionsNum(),
                              vector<double>(id.getParamNum(), 0));
    for (unsigned i = 0; i < id.paramDirectionsNum(); i++) {
      pA[2 * i]
          = id.getParamDirection(i)->getConstraintVector(id.getParamSymbols());
      for (unsigned j = 0; j < id.getParamNum(); j++)
        pA[2 * i + 1][j] = -pA[2 * i][j];
    }

    vector<double> pb(pA.size(), 0);
    for (unsigned i = 0; i < id.paramDirectionsNum(); i++) {
      pb[2 * i] = id.getParamDirection(i)->getUB();
      pb[2 * i + 1] = -id.getParamDirection(i)->getLB();
    }

    this->paraSet = new PolytopesUnion(Polytope(pA, pb));
  }

  // formula
  if (id.isSpecDefined()) {
    this->spec = id.getSpec();
  } else {
    this->spec = NULL;
  }
}

void trim_unused_directions(std::vector<std::vector<double>> &directions,
                            std::vector<double> &LB, std::vector<double> &UB,
                            std::vector<std::vector<int>> &template_matrix)
{
  /* init new_pos by assigning 1 to any useful direction and
   * 0 to those not mentioned by any template */
  std::vector<int> new_pos(directions.size(), 0);
  for (const std::vector<int> &T: template_matrix) {
    for (const int &dir: T) {
      new_pos[dir] = 1;
    }
  }

  /* ala CountingSort we sum the content of all the cells
   * with the previous partial sum to get the new position
   * of the used direction */
  for (unsigned int i = 1; i < directions.size(); i++) {
    new_pos[i] += new_pos[i - 1];
  }

  /* The overall number of useful directions is the last new position */
  unsigned int num_of_directions = new_pos[directions.size() - 1];

  for (unsigned int i = 0; i < directions.size(); i++) {

    /* if i is a used direction */
    if ((i == 0 && new_pos[i] == 1) || (new_pos[i] != new_pos[i - 1])) {

      /* reassign i to its new position */
      const int new_i = new_pos[i] - 1;
      directions[new_i] = directions[i];
      LB[new_i] = LB[i];
      UB[new_i] = UB[i];
    }
  }

  /* resize all the vectors */
  directions.resize(num_of_directions);
  LB.resize(num_of_directions);
  UB.resize(num_of_directions);

  /* re-map the template matrix */
  for (std::vector<int> &T: template_matrix) {
    for (int &dir: T) {
      dir = new_pos[dir] - 1;
    }
  }
}

std::vector<std::vector<double>> getDirections(const InputData &id)
{
  std::vector<std::vector<double>> directions;

  for (unsigned i = 0; i < id.getDirectionsNum(); i++) {
    directions.push_back(
        id.getDirection(i)->getConstraintVector(id.getVarSymbols()));
  }

  return directions;
}

Bundle *getBundleWithAssumptions(const InputData &id)
{
  using namespace LinearAlgebra;

  std::vector<std::vector<double>> directions;
  std::vector<double> LB, UB;

  // get directions and boundaries from input data
  for (unsigned i = 0; i < id.getDirectionsNum(); i++) {
    directions.push_back(
        id.getDirection(i)->getConstraintVector(id.getVarSymbols()));
    UB.push_back(id.getDirection(i)->getUB());
    LB.push_back(id.getDirection(i)->getLB());
  }
  std::vector<std::vector<int>> template_matrix = id.getTemplate();

  /* if users have specified at least one template, ... */
  if (template_matrix.size() > 0) {
    /* ... they really want to exclusively use those templates.
       Thus, trim the unused directions. */

    /* Why can't simply uses the template themselve?
       Because Bundle:transform in AFO mode assumes that
       each of the directions belongs to a template at least. */
    trim_unused_directions(directions, LB, UB, template_matrix);
  }

  /* MISTERY (actually, something about assumptions) */

  // directions affected by constraints, and their offsets
  std::vector<std::vector<double>> constrDirs{};
  std::vector<double> constrOffsets{};

  // new directions to be added
  std::vector<std::vector<double>> C{};

  for (unsigned i = 0; i < id.getAssumptionsNumber(); i++) {

    std::vector<double> new_dir
        = id.getAssumption(i)->getConstraintVector(id.getVarSymbols());
    std::vector<double> negated_dir = -new_dir;

    int pos_dir = find(directions, new_dir);
    int pos_negated_dir = find(directions, negated_dir);

    if (pos_dir != -1) { // constrain a direction which is in the L matrix

      constrDirs.push_back(directions[pos_dir]);
      constrOffsets.push_back(
          rescale(id.getAssumption(i)->getOffset(), new_dir, constrDirs[i]));
      UB[pos_dir] = std::min(UB[pos_dir], constrOffsets[i]);

    } else if (pos_negated_dir
               != -1) { // constrain a direction opposite of one in L matrix

      constrDirs.push_back(-directions[pos_negated_dir]);
      constrOffsets.push_back(
          rescale(id.getAssumption(i)->getOffset(), new_dir, constrDirs[i]));
      LB[pos_negated_dir] = std::max(LB[pos_negated_dir], constrOffsets[i]);

    } else { // constrain a direction not in L matrix

      int C_pos = find(C, new_dir);
      int C_negated_pos = find(C, negated_dir);

      if (C_pos
          != -1) { // direction is already constrained, change offset and UB

        constrOffsets[directions.size() + C_pos]
            = std::min(constrOffsets[directions.size() + C_pos],
                       id.getAssumption(i)->getOffset());
        UB[directions.size() + C_pos]
            = constrOffsets[directions.size() + C_pos];

      } else if (C_negated_pos != -1) { // negated direction is constrained,
                                        // change LB and add new direction

        constrDirs.push_back(new_dir);
        constrOffsets.push_back(id.getAssumption(i)->getOffset());
        LB[directions.size() + C_negated_pos]
            = -id.getAssumption(i)->getOffset();

      } else { // new direction

        constrDirs.push_back(new_dir);
        constrOffsets.push_back(id.getAssumption(i)->getOffset());
        C.push_back(new_dir);

        double min_val = findDirectionBound(directions, LB, UB, new_dir, true);
        LB.push_back(min_val);
        UB.push_back(constrOffsets[i]);
      }
    } // end else dir_pos
  }

  std::vector<std::vector<int>> templ
      = computeTemplate(directions, C, template_matrix);

  // add C to directions
  for (unsigned i = 0; i < C.size(); i++) {
    directions.push_back(C[i]);
  }

  /* END OF MYSTERY */

  return new Bundle(directions, LB, UB, templ, constrDirs, constrOffsets);
}

/**!
 * Check whether two vectors lay on the same line
 *
 * @param[in] v1 is the first vector
 * @param[in] v2 is the second vector
 * @return true if and only if the two vectors lay on the same line
 * @throw domain_error if the two vectors do not belong to the same space
 */
bool on_the_same_line(const std::vector<double> &v1,
                      const std::vector<double> &v2)
{
  if (v1.size() != v2.size()) {
    throw domain_error("The two vectors must belong to the same space");
  }

  if (v1.size() == 0) {
    return true;
  }

  // find the first non-null element in v1, i.e., v1[nn_i]
  size_t nn_i = 0;
  while (nn_i < v1.size() && v1[nn_i] == 0) {

    // if v1[nn_i]==0 and v2[nn_i] != 0, then they are not on the same line
    if (v2[nn_i] != 0) {
      return false;
    }
    nn_i++;
  }

  for (size_t i = nn_i; i < v1.size(); ++i) {
    // if the product of the i-th elements of v1 and v2[nn_i] differs from
    // the product of the i-th elements of v2 and v1[nn_i]
    if (v2[i] * v1[nn_i] != v1[i] * v2[nn_i]) {
      // v2 != v2[nn_i]/v1[nn_i] * v1
      return false;
    }
  }

  return true;
}

/**!
 * Search in a vector for a direction which lay on the same line as a vector.
 *
 * @param[in] M is a vector of directions
 * @param[in] v is a vector
 * @return the index in `M` of a direction which lays on the same line as `v`
 * if it exists; an invalid index for `M`, otherwise
 */

size_t find_on_the_same_line(const std::vector<std::vector<double>> &M,
                             const std::vector<double> &v)
{
  for (size_t i = 0; i < M.size(); ++i) {
    if (on_the_same_line(M[i], v)) {
      return i;
    }
  }
  return M.size();
}

int find(const std::vector<std::vector<double>> M, const std::vector<double> v)
{
  std::vector<double> v_norm = normalize(v);
  for (unsigned i = 0; i < M.size(); i++) {
    std::vector<double> M_norm = normalize(M[i]);
    if (compare(v_norm, M_norm, 0.00001)) {
      return i;
    }
  }
  return -1;
}

double getNormalizationCoefficient(std::vector<double> v)
{
  double res = 0;
  for (unsigned i = 0; i < v.size(); i++) {
    res += v[i] * v[i];
  }
  return 1.0 / sqrt(res);
}

std::vector<double> normalize(std::vector<double> v)
{
  double coeff = getNormalizationCoefficient(v);
  std::vector<double> res{};
  for (unsigned i = 0; i < v.size(); i++) {
    res.push_back(coeff * v[i]);
  }
  return res;
}

bool compare(std::vector<double> v1, std::vector<double> v2, double tol)
{
  if (v1.size() != v2.size()) {
    return false;
  }

  for (unsigned i = 0; i < v1.size(); i++) {
    if (abs(v1[i] - v2[i]) > tol) {
      return false;
    }
  }

  return true;
}

double rescale(double val, std::vector<double> v1, std::vector<double> v2)
{
  return val * getNormalizationCoefficient(v1)
         / getNormalizationCoefficient(v2);
}

std::vector<std::vector<int>>
computeTemplate(const std::vector<std::vector<double>> A,
                const std::vector<std::vector<double>> C,
                std::vector<std::vector<int>> oldTemplate)
{
  std::vector<bool> dirCovered(A.size(), false);
  for (unsigned i = 0; i < oldTemplate.size(); i++) {
    for (unsigned j = 0; j < oldTemplate[i].size(); j++) {
      dirCovered[oldTemplate[i][j]] = true;
    }
  }

  unsigned uncoveredRows = 0;
  for (unsigned i = 0; i < dirCovered.size(); i++) {
    uncoveredRows += dirCovered[i] ? 0 : 1;
  }

  unsigned n = A[0].size(); // var num, = cols of A
  unsigned m = A.size();    // dirs num = rows of A
  unsigned c = C.size();    // constr num = assumptions = rows of C
  unsigned Pn = oldTemplate.size() + ceil(((double)(c + uncoveredRows)) / n);

  unsigned cols = Pn * (m + c);

  //	std::cout << "\tn = " << n << ", m = " << m << ", c = " << c << ", Pn =
  //" << Pn << ", cols = " << cols << ", rows = " << rows << std::endl;

  // create ILP problem
  glp_prob *lp;
  lp = glp_create_prob();
  glp_set_obj_dir(lp, GLP_MIN);

  // continuous case parameters
  glp_smcp lp_param;
  glp_init_smcp(&lp_param);
  lp_param.msg_lev = GLP_MSG_ERR;

  // integer/boolean case parameters
  /*	glp_iocp ilp_param;
          glp_init_iocp(&ilp_param);
          ilp_param.presolve = GLP_ON;
    ilp_param.msg_lev = GLP_MSG_OFF;*/

  // add columns (unbounded)
  glp_add_cols(lp, cols);
  for (unsigned int i = 0; i < cols; i++) {
    // columns are unbounded (continuous case)
    glp_set_col_bnds(lp, i + 1, GLP_DB, 0, 1);
    // set columns to be boolean (discrete case)
    //		glp_set_col_kind(lp, i + 1, GLP_BV);
  }

  // add objective function (all ones, we want only satisfiability)
  for (unsigned i = 0; i < cols; i++) {
    glp_set_obj_coef(lp, i + 1, 0);
  }

  unsigned globalIndex = 1;
  paralCardConstraints(&lp, A, C, &globalIndex, Pn);
  //	varCoverConstraints(&lp, A, C, &globalIndex, Pn);
  directionUseConstraints(&lp, A, C, &globalIndex, Pn);
  oldTemplateConstraints(&lp, A, C, &globalIndex, oldTemplate, Pn);
  independenceConstraints(&lp, A, C, &globalIndex, Pn);

  /* TODO: check that LP (not integer) is sufficient ->
   * matrix is not always TUM, but solution so far has always been int
   */
  glp_simplex(lp, &lp_param);
  //	glp_intopt(lp, &ilp_param);

  std::vector<std::vector<int>> T(Pn, std::vector<int>{});
  for (unsigned P = 0; P < Pn; P++) {
    for (unsigned d = 0; d < m + c; d++) {
      if (glp_get_col_prim(lp, map_paral(d, P, Pn) + 1) == 1) {
        //			if (glp_mip_col_val(lp, map_paral(d, P, Pn) +
        // 1)
        //== 1) {
        T[P].push_back(d);
      }
    }
  }

  return T;
}

void paralCardConstraints(glp_prob **lp,
                          const std::vector<std::vector<double>> A,
                          const std::vector<std::vector<double>> C,
                          unsigned *startingIndex, unsigned Pn)
{
  unsigned n = A[0].size(); // var num, = cols of A
  unsigned m = A.size();    // dirs num = rows of A
  unsigned c = C.size();    // constr num = assumptions = rows of C

  unsigned cols = Pn * (m + c);

  unsigned index = *startingIndex;

  // for each Parallelotope P, P has n directions
  for (unsigned P = 0; P < Pn; P++) {
    int *indeces = (int *)malloc((cols + 1) * sizeof(int));
    double *constr = (double *)malloc((cols + 1) * sizeof(double));
    int len = 0;
    indeces[0] = 0;
    constr[0] = 0;
    for (unsigned d = 0; d < m + c; d++) {
      len++;
      indeces[len] = map_paral(d, P, Pn) + 1;
      constr[len] = 1;
    }
    glp_add_rows(*lp, 1);
    glp_set_row_bnds(*lp, index, GLP_FX, n, n);
    glp_set_mat_row(*lp, index, len, indeces, constr);
    free(constr);
    free(indeces);
    index++;
  }
  *startingIndex = index;
}

void varCoverConstraints(glp_prob **lp,
                         const std::vector<std::vector<double>> A,
                         const std::vector<std::vector<double>> C,
                         unsigned *startingIndex, unsigned Pn)
{
  unsigned n = A[0].size(); // var num, = cols of A
  unsigned m = A.size();    // dirs num = rows of A
  unsigned c = C.size();    // constr num = assumptions = rows of C

  unsigned cols = Pn * (m + c);

  unsigned index = *startingIndex;

  // each parallelotope covers each variable
  for (unsigned v = 0; v < n; v++) {    // for each variable v
    for (unsigned P = 0; P < Pn; P++) { // for each Parallelotope
      int *indeces = (int *)malloc((cols + 1) * sizeof(int));
      double *constr = (double *)malloc((cols + 1) * sizeof(double));
      int len = 0;
      indeces[0] = 0;
      constr[0] = 0;
      for (unsigned d = 0; d < m; d++) {
        if (A[d][v] != 0) { // for each direction in A covering v
          len++;
          indeces[len] = map_paral(d, P, Pn) + 1;
          constr[len] = 1;
        }
      }

      for (unsigned d = 0; d < c; d++) {
        if (C[d][v] != 0) { // for each direction in C covering v
          len++;
          indeces[len] = map_paral(d + m, P, Pn) + 1;
          constr[len] = 1;
        }
      }

      glp_add_rows(*lp, 1);
      glp_set_row_bnds(*lp, index, GLP_LO, 1, 0);
      glp_set_mat_row(*lp, index, len, indeces, constr);

      free(constr);
      free(indeces);
      index++;
    }
  }
  *startingIndex = index;
}

void directionUseConstraints(glp_prob **lp, std::vector<std::vector<double>> A,
                             std::vector<std::vector<double>> C,
                             unsigned *startingIndex, unsigned Pn)
{
  unsigned m = A.size(); // dirs num = rows of A
  unsigned c = C.size(); // constr num = assumptions = rows of C

  unsigned cols = Pn * (m + c);

  unsigned index = *startingIndex;

  // directions in A
  for (unsigned d = 0; d < m; d++) {
    int *indeces = (int *)malloc((cols + 1) * sizeof(int));
    double *constr = (double *)malloc((cols + 1) * sizeof(double));
    int len = 0;
    indeces[0] = 0;
    constr[0] = 0;
    for (unsigned P = 0; P < Pn; P++) {
      len++;
      indeces[len] = map_paral(d, P, Pn) + 1;
      constr[len] = 1;
    }
    glp_add_rows(*lp, 1);
    glp_set_row_bnds(*lp, index, GLP_LO, 1, 0);
    glp_set_mat_row(*lp, index, len, indeces, constr);
    free(constr);
    free(indeces);
    index++;
  }

  // directions in C
  for (unsigned d = 0; d < c; d++) {
    int *indeces = (int *)malloc((cols + 1) * sizeof(int));
    double *constr = (double *)malloc((cols + 1) * sizeof(double));
    int len = 0;
    indeces[0] = 0;
    constr[0] = 0;
    for (unsigned P = 0; P < Pn; P++) {
      len++;
      indeces[len] = map_paral(d + m, P, Pn) + 1;
      constr[len] = 1;
    }
    glp_add_rows(*lp, 1);
    glp_set_row_bnds(*lp, index, GLP_LO, 1, 0);
    glp_set_mat_row(*lp, index, len, indeces, constr);
    free(constr);
    free(indeces);
    index++;
  }
  *startingIndex = index;
}

void oldTemplateConstraints(glp_prob **lp,
                            const std::vector<std::vector<double>> A,
                            const std::vector<std::vector<double>> C,
                            unsigned *startingIndex,
                            std::vector<std::vector<int>> oldTemplate,
                            unsigned Pn)
{
  unsigned m = A.size(); // dirs num = rows of A
  unsigned c = C.size(); // constr num = assumptions = rows of C

  unsigned cols = Pn * (m + c);

  unsigned index = *startingIndex;

  for (unsigned P = 0; P < oldTemplate.size(); P++) {
    for (unsigned d = 0; d < oldTemplate[P].size(); d++) {
      int *indeces = (int *)malloc((cols + 1) * sizeof(int));
      double *constr = (double *)malloc((cols + 1) * sizeof(double));
      int len = 1;
      indeces[0] = 0;
      constr[0] = 0;
      indeces[1] = map_paral(oldTemplate[P][d], P, Pn) + 1;
      constr[1] = 1;
      glp_add_rows(*lp, 1);
      glp_set_row_bnds(*lp, index, GLP_FX, 1, 1);
      glp_set_mat_row(*lp, index, len, indeces, constr);
      free(constr);
      free(indeces);
      index++;
    }
  }

  *startingIndex = index;
}

void independenceConstraints(glp_prob **lp,
                             const std::vector<std::vector<double>> A,
                             const std::vector<std::vector<double>> C,
                             unsigned *startingIndex, unsigned Pn)
{
  unsigned n = A[0].size(); // var num, = cols of A
  unsigned m = A.size();    // dirs num = rows of A
  unsigned c = C.size();    // constr num = assumptions = rows of C

  unsigned cols = Pn * (m + c);

  unsigned index = *startingIndex;

  // tentative parallelotope, to check if rows are independent
  // dirSet has a boolean variable for each direction, true if we take it,
  // false otherwise
  std::vector<bool> tentParal(m, false);
  for (unsigned i = 0; i < n; i++) {
    tentParal[m - i - 1] = true;
  }

  // vector of zeroes, for offsets
  std::vector<double> zeroes(n, 0);

  bool finished = false;

  while (!finished) {

    std::vector<unsigned> dirSet{};
    for (unsigned i = 0; i < m; i++) {
      if (tentParal[i]) {
        dirSet.push_back(i);
      }
    }

    //		std::cout << "considering directions " << dirSet << std::endl;

    std::vector<std::vector<double>> M{};
    for (unsigned i = 0; i < n; i++) {
      if (dirSet[i] < m) {
        M.push_back(A[dirSet[i]]);
      } else {
        M.push_back(C[dirSet[i] - m]);
      }
    }
    LinearAlgebra::Dense::LUP_Factorization<double> LUP(M);
    try {
      std::vector<double> res = LUP.solve(zeroes);
    } catch (std::domain_error &e) {
      // vectors are dependent (?)
      for (unsigned p = 0; p < Pn; p++) {
        int *indeces = (int *)malloc((cols + 1) * sizeof(int));
        double *constr = (double *)malloc((cols + 1) * sizeof(double));
        int len = 0;
        indeces[0] = 0;
        constr[0] = 0;
        for (unsigned di = 0; di < n; di++) {
          len++;
          indeces[len] = map_paral(dirSet[di], p, Pn) + 1;
          constr[len] = 1;
        }

        glp_add_rows(*lp, 1);
        glp_set_row_bnds(*lp, index, GLP_DB, 0, n - 1);
        glp_set_mat_row(*lp, index, len, indeces, constr);
        free(constr);
        free(indeces);
        index++;
      }
    }

    int index = m - 1;
    unsigned trues = 0;
    while (!tentParal[index]) {
      index--;
    }
    while (index >= 0 && tentParal[index]) {
      index--;
      trues++;
    }

    if (index == -1) {
      // all trues to the left, no more cases to consider
      finished = true;
    } else {
      /* tentParal is <1, 0, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0>
       *                                          |  |--| trues
       *                                        index
       * now, we move left the true in pos index+1, and we move to the end the
       * remaining trues
       */
      tentParal[index] = true;
      tentParal[index + 1] = false;

      unsigned i = m - 1;
      for (; i >= m - trues + 1; i--) {
        tentParal[i] = true;
      }
      while (i > (unsigned)index + 1) {
        tentParal[i] = false;
        i--;
      }
    }
  }

  *startingIndex = index;
}

double findDirectionBound(std::vector<std::vector<double>> A,
                          std::vector<double> LB, std::vector<double> UB,
                          std::vector<double> dir, bool minimize)
{
  unsigned row_n = A.size();
  unsigned col_n = A[0].size();

  glp_prob *lp;
  lp = glp_create_prob();

  if (minimize) {
    glp_set_obj_dir(lp, GLP_MIN);
  } else {
    glp_set_obj_dir(lp, GLP_MAX);
  }

  glp_smcp lp_param;
  glp_init_smcp(&lp_param);
  lp_param.msg_lev = GLP_MSG_OFF;

  glp_add_rows(lp, row_n);

  // add columns (unbounded)
  glp_add_cols(lp, col_n);
  for (unsigned int i = 0; i < col_n; i++) {
    glp_set_col_bnds(lp, i + 1, GLP_FR, 0, 0);
  }

  // add objective function (all ones, we want only satisfiability)
  for (unsigned i = 0; i < dir.size(); i++) {
    glp_set_obj_coef(lp, i + 1, dir[i]);
  }

  // add constraints
  for (unsigned i = 0; i < A.size(); i++) {
    int *indeces = (int *)malloc((col_n + 1) * sizeof(int));
    double *constr = (double *)malloc((col_n + 1) * sizeof(double));
    int len = 0;
    indeces[0] = 0;
    constr[0] = 0;
    for (unsigned j = 0; j < A[i].size(); j++) {
      len++;
      indeces[len] = j + 1;
      constr[len] = A[i][j];
    }
    if (LB[i] == UB[i]) {
      glp_set_row_bnds(lp, i + 1, GLP_FX, LB[i], UB[i]);
    } else {
      glp_set_row_bnds(lp, i + 1, GLP_DB, LB[i], UB[i]);
    }
    glp_set_mat_row(lp, i + 1, len, indeces, constr);
    free(constr);
    free(indeces);
  }

  glp_exact(lp, &lp_param);

  double res;
  if (glp_get_status(lp) == GLP_UNBND) {
    if (minimize) {
      res = -std::numeric_limits<double>::infinity();
    } else {
      res = std::numeric_limits<double>::infinity();
    }
  } else {
    res = glp_get_obj_val(lp);
  }
  glp_delete_prob(lp);
  glp_free_env();

  return res;
}
