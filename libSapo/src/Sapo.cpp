/**
 * @file Sapo.cpp
 * @author Tommaso Dreossi (tommasodreossi@berkeley.edu)
 * @author Alberto Casagrande (acasagrande@units.it)
 * @brief Core of Sapo tool
 * 
 * @version 0.3
 * @date 2022-04-12
 * 
 * @copyright Copyright (c) 2015-2022
 * 
 */

#include "Sapo.h"

#include <limits>

#ifdef WITH_THREADS
#include <shared_mutex>

#include "SapoThreads.h"
#endif // WITH_THREADS

#include "PolytopesUnion.h"

/**
 * Constructor that instantiates Sapo
 *
 * @param[in] model model to analyze
 */
Sapo::Sapo(const Model &model):
    decomp(0), max_param_splits(0), num_of_pre_splits(0),
    max_bundle_magnitude(std::numeric_limits<double>::max()),
    _dynamical_system(model.dynamical_system()),
    assumptions(model.assumptions())
{
}

Flowpipe Sapo::reach(Bundle init_set, unsigned int k,
                     ProgressAccounter *accounter) const
{
  init_set.intersect_with(this->assumptions);

  // create current bundles list
  std::list<Bundle> cbundles = init_set.split(max_bundle_magnitude, 1.0);

  // create next bundles list
  std::list<Bundle> nbundles;

  // last polytope union in flowpipe
  SetsUnion<Polytope> last_step(init_set);
  simplify(last_step);

  // create flowpipe
  Flowpipe flowpipe;
  flowpipe.push_back(last_step);

#ifdef WITH_THREADS
  std::mutex mutex;

  auto compute_next_bundles_and_add_to_last
      = [&nbundles, &last_step, &mutex](const Sapo *sapo, const Bundle &bundle)
#else
  auto compute_next_bundles_and_add_to_last
      = [&nbundles, &last_step](const Sapo *sapo, const Bundle &bundle)
#endif

  {
    using namespace LinearAlgebra;

    const DynamicalSystem<double> &ds = sapo->dynamical_system();

    // get the transformed bundle
    Bundle nbundle = ds.transform(bundle, sapo->t_mode); // transform it

    // guarantee the assumptions
    nbundle.intersect_with(sapo->assumptions);

    if (sapo->decomp > 0) { // if requested, decompose it
      nbundle = nbundle.decompose(sapo->decomp_weight, sapo->decomp);
    }

    // TODO: check whether there is any chance for a transformed bundle to
    // be empty
    if (!nbundle.is_empty()) {
      // split if necessary the new reached bundle and add the resulting
      // bundles to the nbundles list
      nbundles.splice(nbundles.end(),
                      nbundle.split(sapo->max_bundle_magnitude));

      Polytope bls = nbundle;
      {
#ifdef WITH_THREADS
        std::unique_lock<std::mutex> lock(mutex);
#endif // WITH_THREADS
        last_step.add(bls);
      }
    }
  };

  unsigned int i = 0;

  // while time horizon has not been reached and last step reached is not empty
  // TODO: check whether there exists any chance for the last_step to be empty
  while (i < k && last_step.size() != 0) {

    // create a new last step reach set
    last_step = SetsUnion<Polytope>();
    i++;

#ifdef WITH_THREADS
    ThreadPool::BatchId batch_id = thread_pool.create_batch();

    // for all the old bundles
    for (auto b_it = std::cbegin(cbundles); b_it != std::cend(cbundles);
         ++b_it) {
      // submit the task to the thread pool
      thread_pool.submit_to_batch(batch_id,
                                  compute_next_bundles_and_add_to_last, this,
                                  std::ref(*b_it));
    }

    // join to the pool threads
    thread_pool.join_threads(batch_id);

    // close the batch
    thread_pool.close_batch(batch_id);
#else  // WITH_THREADS

    // for all the old bundles
    for (auto b_it = std::cbegin(cbundles); b_it != std::cend(cbundles);
         ++b_it) {

      compute_next_bundles_and_add_to_last(this, std::ref(*b_it));
    }
#endif // WITH_THREADS

    // swap current bundles and new bundles
    std::swap(cbundles, nbundles);

    nbundles = std::list<Bundle>();

    // add the last step to the flow pipe
    flowpipe.push_back(last_step); // store result

    if (accounter != NULL) {
      accounter->increase_performed();
    }
  }

  return flowpipe;
}

Flowpipe Sapo::reach(Bundle init_set, const SetsUnion<Polytope> &pSet,
                     unsigned int k, ProgressAccounter *accounter) const
{
  using namespace std;
  const unsigned int num_p_poly = pSet.size();

  // Each polytope in pSet corresponds to the list of bundles
  // reachable by using the parameters in that polytope.
  // This list is stored in an element of the vector cbundles

  init_set.intersect_with(this->assumptions);

  // create current bundle list vector
  std::list<Bundle> cbundle_list = init_set.split(max_bundle_magnitude, 1.0);
  std::vector<std::list<Bundle>> cbundles(num_p_poly, cbundle_list);

  // create next bundles list
  std::vector<std::list<Bundle>> nbundles;

  // last polytope union in flowpipe
  SetsUnion<Polytope> last_step(init_set);
  simplify(last_step);

  // create flowpipe
  Flowpipe flowpipe;
  flowpipe.push_back(last_step);

#ifdef WITH_THREADS_TEMP_DISABLED
  std::mutex add_mtx;

  auto compute_next_bundles_and_add_to_last
      = [&nbundles, &cbundles, &last_step, &add_mtx](
            const Sapo *sapo, const Polytope &pSet, const unsigned int pos)
#else
  auto compute_next_bundles_and_add_to_last
      = [&nbundles, &cbundles, &last_step](
            const Sapo *sapo, const Polytope &pSet, const unsigned int pos)
#endif

  {
    const DynamicalSystem<double> &ds = sapo->dynamical_system();

    // for all the parameter sets
    for (auto b_it = std::cbegin(cbundles[pos]);
         b_it != std::cend(cbundles[pos]); ++b_it) {
      // get the transformed bundle
      Bundle nbundle = ds.transform(*b_it, pSet, sapo->t_mode); // transform it

      // guarantee the assumptions
      nbundle.intersect_with(sapo->assumptions);

      if (sapo->decomp > 0) { // if requested, decompose it
        nbundle = nbundle.decompose(sapo->decomp_weight, sapo->decomp);
      }

      Polytope bls = nbundle;

      // TODO: check whether there is any chance for a transformed bundle to
      // be empty
      if (!bls.is_empty()) {
        // split if necessary the new reached bundle and add the resulting
        // bundles to the nbundles list
        nbundles[pos].splice(nbundles[pos].end(),
                             nbundle.split(sapo->max_bundle_magnitude));

        {
#ifdef WITH_THREADS_TEMP_DISABLED
          std::unique_lock<std::mutex> lock(add_mtx);
#endif
          last_step.add(bls);
        }
      }
    }
  };

  unsigned int i = 0;

  // while time horizon has not been reached and last step reached is not empty
  // TODO: check whether there exists any chance for the last_step to be empty
  while (i < k && last_step.size() != 0) {

    nbundles = std::vector<std::list<Bundle>>(num_p_poly);

    // create a new last step reach set
    last_step = SetsUnion<Polytope>();
    i++;

    unsigned int pSet_idx = 0;
#ifdef WITH_THREADS_TEMP_DISABLED
    ThreadPool::BatchId batch_id = thread_pool.create_batch();

    // for all the old bundles
    for (auto p_it = std::cbegin(pSet); p_it != std::cend(pSet); ++p_it) {
      // submit the task to the thread pool
      thread_pool.submit_to_batch(batch_id,
                                  compute_next_bundles_and_add_to_last, this,
                                  std::ref(*p_it), pSet_idx++);
    }

    // join to the pool threads
    thread_pool.join_threads(batch_id);

    // close the batch
    thread_pool.close_batch(batch_id);
#else  // WITH_THREADS

    // for all the old bundles
    for (auto p_it = std::cbegin(pSet); p_it != std::cend(pSet); ++p_it) {

      compute_next_bundles_and_add_to_last(this, std::ref(*p_it), pSet_idx++);
    }
#endif // WITH_THREADS

    // move the new bundles content in the current bundles
    cbundles = std::move(nbundles);

    // add the last step to the flow pipe
    flowpipe.push_back(last_step); // store result

    if (accounter != NULL) {
      accounter->increase_performed();
    }
  }

  if (accounter != NULL) {
    accounter->increase_performed_to(k);
  }

  return flowpipe;
}

/**
 * @brief Get every a finer covering of a set
 *
 * This method takes a list of polytope unions and splits
 * each of its objects in `num_of_polytope_splits` polytope
 * unions that cover the original sets.
 *
 * @param orig is a list of poytope unions
 * @param num_of_polytope_splits is the number of splits to
 *        be performed
 * @return a list of polytopes unions that cover the
 *         region covered `orig`
 */
std::list<SetsUnion<Polytope>>
get_a_finer_covering(const std::list<SetsUnion<Polytope>> &orig,
                     const unsigned int num_of_polytope_splits
                     = std::numeric_limits<unsigned>::max())
{
  std::list<SetsUnion<Polytope>> result;
  for (auto ps_it = std::cbegin(orig); ps_it != std::cend(orig); ++ps_it) {

    // if the polytopes union contains more than one polytope
    switch (ps_it->size()) {
    case 0: // the polytopes union does not contain any polytope

      // nothing to add to the resulting list
      break;
    case 1: // the polytopes union contains exactly one polytope
    {       // then, split it by using Polytope::get_a_finer_covering();

      std::list<Polytope> f_cov
          = (ps_it->begin())->split(num_of_polytope_splits);
      for (auto ls_it = std::begin(f_cov); ls_it != std::end(f_cov); ++ls_it) {
        result.push_back(*ls_it);
      }
      break;
    }
    case 2:  // the polytopes union contains more than one polytope
    default: // then, unpack them
    {
      for (auto ls_it = ps_it->begin(); ls_it != ps_it->end(); ++ls_it) {
        result.push_back(*ls_it);
      }
      break;
    }
    }
  }

  return result;
}

#ifdef WITH_THREADS
template<typename T>
class ThreadSafeList
{
  std::list<T> list;
  mutable std::shared_timed_mutex mutex;

public:
  ThreadSafeList(): list() {}

  ThreadSafeList(const std::list<T> &list): list(list) {}

  ThreadSafeList<T> &push_back(T &&obj)
  {
    std::unique_lock<std::shared_timed_mutex> writelock(mutex);

    list.push_back(obj);

    return *this;
  }

  ThreadSafeList<T> &push_back(const T &obj)
  {
    std::unique_lock<std::shared_timed_mutex> writelock(mutex);

    list.push_back(obj);

    return *this;
  }

  const std::list<T> &get_list() const
  {
    std::shared_lock<std::shared_timed_mutex> readlock(mutex);
    return list;
  }
};
#endif // WITH_THREADS

/**
 * Parameter synthesis
 *
 * @param[in] sapo the Sapo object that performs the computation
 * @param[in] init_set is the initial set
 * @param[in] pSetList a list of parameter sets
 * @param[in] formula is an STL formula providing the specification
 * @param[in,out] accounter accounts for the computation progress
 * @returns the list of refined parameter sets
 */
std::list<SetsUnion<Polytope>>
synthesize_list(const Sapo &sapo, const Bundle &init_set,
                const std::list<SetsUnion<Polytope>> &pSetList,
                const std::shared_ptr<STL::STL> &formula,
                ProgressAccounter *accounter)
{

#ifdef WITH_THREADS
  std::vector<SetsUnion<Polytope>> vect_res(pSetList.size());

  auto synthesize_funct
      = [&vect_res, &sapo, &init_set, &formula,
         &accounter](const SetsUnion<Polytope> pSet, const unsigned int idx) {
          vect_res[idx] = sapo.synthesize(init_set, pSet, formula);

          if (accounter != NULL) {
            accounter->increase_performed(formula->time_bounds().end());
          }
        };

  ThreadPool::BatchId batch_id = thread_pool.create_batch();

  unsigned int res_idx = 0;
  for (auto ps_it = std::begin(pSetList); ps_it != std::end(pSetList);
       ++ps_it) {
    // submit the task to the thread pool
    thread_pool.submit_to_batch(batch_id, synthesize_funct, *ps_it, res_idx++);
  }

  // join to the pool threads
  thread_pool.join_threads(batch_id);

  // close the batch
  thread_pool.close_batch(batch_id);

  return std::list<SetsUnion<Polytope>>(std::make_move_iterator(vect_res.begin()),
                                   std::make_move_iterator(vect_res.end()));

#else  // WITH_THREADS
  std::list<SetsUnion<Polytope>> results;

  for (auto ps_it = std::begin(pSetList); ps_it != std::end(pSetList);
       ++ps_it) {
    results.push_back(sapo.synthesize(reachSet, *ps_it, formula));
    if (accounter != NULL) {
      accounter->increase_performed(formula->time_bounds().end());
    }
  }

  return results;
#endif // WITH_THREADS
}

/**
 * Parameter synthesis with splits
 *
 * @param[in] init_set is the initial set
 * @param[in] pSet is the current parameter sets
 * @param[in] formula is an STL formula providing the specification
 * @param[in] max_splits maximum number of splits of the original
 *                       parameter set to identify a non-null solution
 * @param[in] num_of_pre_splits is number of splits to be performed before
 *                             the computation
 * @param[in,out] accounter accounts for the computation progress
 * @returns the list of refined parameter sets
 */
std::list<SetsUnion<Polytope>> Sapo::synthesize(
    Bundle init_set, const SetsUnion<Polytope> &pSet,
    const std::shared_ptr<STL::STL> formula, const unsigned int max_splits,
    const unsigned int num_of_pre_splits, ProgressAccounter *accounter) const
{
  if (this->assumptions.size() > 0) {
    throw std::runtime_error("Assumptions not supported in synthesis yet.");
  }

  std::list<SetsUnion<Polytope>> pSetList{pSet};

  if (num_of_pre_splits > 1) {
    pSetList = get_a_finer_covering(pSetList, num_of_pre_splits);
  }

  const unsigned int max_time = formula->time_bounds().end();
  unsigned int already_performed_steps = 0;

  unsigned int num_of_splits = 0;
  std::list<SetsUnion<Polytope>> res
      = synthesize_list(*this, init_set, pSetList, formula, accounter);

  if (accounter) {
    already_performed_steps = max_time * pSetList.size();
    accounter->increase_performed_to(already_performed_steps);
  }

  while (every_set_is_empty(res) && num_of_splits++ < max_splits) {
    pSetList = get_a_finer_covering(pSetList);

    res = synthesize_list(*this, init_set, pSetList, formula, accounter);

    if (accounter != NULL) {
      already_performed_steps += max_time * pSetList.size();
      accounter->increase_performed_to(already_performed_steps);
    }
  }

  for (auto lss_it = std::begin(res); lss_it != std::end(res); ++lss_it) {
    simplify(*lss_it);
  }

  return res;
}

/**
 * Parameter synthesis for conjunctions
 *
 * @param[in] init_set is the current reached set
 * @param[in] pSet is the current parameter set
 * @param[in] conj is an STL conjunction providing the specification
 * @returns refined parameter set
 */
SetsUnion<Polytope>
Sapo::synthesize(const Bundle &init_set, const SetsUnion<Polytope> &pSet,
                 const std::shared_ptr<STL::Conjunction> conj) const
{
  SetsUnion<Polytope> Pu1
      = this->synthesize(init_set, pSet, conj->get_left_subformula());
  SetsUnion<Polytope> Pu2
      = this->synthesize(init_set, pSet, conj->get_right_subformula());
  return intersect(Pu1, Pu2);
}

/**
 * Parameter synthesis for disjunctions
 *
 * @param[in] init_set is the initial set
 * @param[in] pSet the current parameter set
 * @param[in] conj is an STL disjunction providing the specification
 * @returns refined parameter set
 */
SetsUnion<Polytope>
Sapo::synthesize(const Bundle &init_set, const SetsUnion<Polytope> &pSet,
                 const std::shared_ptr<STL::Disjunction> disj) const
{
  SetsUnion<Polytope> Pu
      = this->synthesize(init_set, pSet, disj->get_left_subformula());
  Pu.add(this->synthesize(init_set, pSet, disj->get_right_subformula()));

  return Pu;
}

/**
 * Parameter synthesis for the eventually formulas
 *
 * @param[in] init_set is the initial set
 * @param[in] pSet the current parameter set
 * @param[in] ev is an STL eventually formula providing the specification
 * @returns refined parameter set
 */
SetsUnion<Polytope>
Sapo::synthesize(const Bundle &init_set, const SetsUnion<Polytope> &pSet,
                 const std::shared_ptr<STL::Eventually> ev) const
{
  std::shared_ptr<STL::Atom> true_atom = std::make_shared<STL::Atom>(-1);

  std::shared_ptr<STL::Until> u = std::make_shared<STL::Until>(
      true_atom, ev->time_bounds().begin(), ev->time_bounds().end(),
      ev->get_subformula());

  return this->synthesize(init_set, pSet, u, 0);
}

/**
 * Parameter synthesis method
 *
 * @param[in] init_set is the initial set
 * @param[in] pSet is the set of parameters
 * @param[in] formula is an STL specification for the model
 * @param[in,out] accounter accounts for the computation progress
 * @returns a parameter set refined according with `formula`
 */
SetsUnion<Polytope> Sapo::synthesize(Bundle init_set, const SetsUnion<Polytope> &pSet,
                                const std::shared_ptr<STL::STL> formula,
                                ProgressAccounter *accounter) const
{
  (void)accounter;
  if (this->assumptions.size() > 0) {
    throw std::runtime_error("Assumptions not supported in synthesis yet.");
  }

  switch (formula->get_type()) {

  // Atomic predicate
  case STL::ATOM:
    return synthesize(init_set, pSet,
                      std::dynamic_pointer_cast<STL::Atom>(formula));

  // Conjunction
  case STL::CONJUNCTION:
    return synthesize(init_set, pSet,
                      std::dynamic_pointer_cast<STL::Conjunction>(formula));

  // Disjunction
  case STL::DISJUNCTION:
    return synthesize(init_set, pSet,
                      std::dynamic_pointer_cast<STL::Disjunction>(formula));

  // Until
  case STL::UNTIL:
    return synthesize(init_set, pSet,
                      std::dynamic_pointer_cast<STL::Until>(formula), 0);

  // Always
  case STL::ALWAYS:
    return synthesize(init_set, pSet,
                      std::dynamic_pointer_cast<STL::Always>(formula), 0);

  // Eventually
  case STL::EVENTUALLY:
    return synthesize(init_set, pSet,
                      std::dynamic_pointer_cast<STL::Eventually>(formula));

  default:
    throw std::logic_error("Unsupported formula");
  }
}

/**
 * Parameter synthesis for atomic formulas
 *
 * @param[in] init_set is the initial set
 * @param[in] pSet the current set of parameters
 * @param[in] sigma STL atomic formula
 * @returns refined parameter set
 */
SetsUnion<Polytope> Sapo::synthesize(const Bundle &init_set,
                                const SetsUnion<Polytope> &pSet,
                                const std::shared_ptr<STL::Atom> atom) const
{
  return ::synthesize(this->dynamical_system(), init_set, pSet, atom);
}

/**
 * Parameter synthesis for until formulas
 *
 * @param[in] init_set is the initial set
 * @param[in] pSet the current set of parameters
 * @param[in] formula STL until formula
 * @param[in] time is the time of the current evaluation
 * @returns refined parameter set
 */
SetsUnion<Polytope> Sapo::synthesize(const Bundle &init_set,
                                const SetsUnion<Polytope> &pSet,
                                const std::shared_ptr<STL::Until> formula,
                                const int time) const
{
  const TimeInterval &t_interval = formula->time_bounds();

  // Base case
  if (t_interval.is_empty())
    return SetsUnion<Polytope>();

  // Until interval far
  if (t_interval > time) {
    // Synthesize wrt phi1
    SetsUnion<Polytope> P1
        = this->synthesize(init_set, pSet, formula->get_left_subformula());

    if (P1.is_empty()) {
      return P1; // false until
    } else {
      return transition_and_synthesis(init_set, P1, formula, time);
    }
  }

  // Inside until interval
  if (t_interval.end() > time) {
    // Refine wrt phi1 and phi2
    SetsUnion<Polytope> P1
        = this->synthesize(init_set, pSet, formula->get_left_subformula());

    if (P1.is_empty()) {
      return this->synthesize(init_set, pSet, formula->get_right_subformula());
    }

    SetsUnion<Polytope> result
        = transition_and_synthesis(init_set, P1, formula, time);
    result.add(
        this->synthesize(init_set, pSet, formula->get_right_subformula()));

    return result;
  }

  // If none of the above condition holds, then it must holds that :
  // 			t_interval.begin()<=time and t_interval.end()==time
  return this->synthesize(init_set, pSet, formula->get_right_subformula());
}

/**
 * Parameter synthesis for always formulas
 *
 * @param[in] init_set is the initial set
 * @param[in] pSet the current set of parameters
 * @param[in] formula STL always formula
 * @param[in] time is the time of the current evaluation
 * @returns refined parameter set
 */
SetsUnion<Polytope> Sapo::synthesize(const Bundle &init_set,
                                const SetsUnion<Polytope> &pSet,
                                const std::shared_ptr<STL::Always> formula,
                                const int time) const
{
  const TimeInterval &t_interval = formula->time_bounds();

  // Base case
  if (t_interval.is_empty()) {
    return SetsUnion<Polytope>();
  }

  // Always interval far
  if (t_interval > time) {
    // transition and synthesis
    return transition_and_synthesis(init_set, pSet, formula, time);
  }

  // Inside Always interval
  if (t_interval.end() > time) {

    // Refine wrt phi
    SetsUnion<Polytope> P
        = this->synthesize(init_set, pSet, formula->get_subformula());

    if (P.is_empty()) {
      return P;
    }

    return transition_and_synthesis(init_set, P, formula, time);
  }

  // If none of the above condition holds, then it must holds that :
  // 			t_interval.begin()<=time and t_interval.end()==time
  return this->synthesize(init_set, pSet, formula->get_subformula());
}
