/**
 * @file STL.h
 * @author Tommaso Dreossi <tommasodreossi@berkeley.edu>
 * @author Alberto Casagrande <acasagrande@units.it>
 * @brief STL formula
 * @version 0.2
 * @date 2022-05-04
 * 
 * @copyright Copyright (c) 2015-2022
 */

#ifndef STL_H_
#define STL_H_

#include <iostream>
#include <string>
#include <memory>

#include "../SymbolicAlgebra.h"

#include "TimeInterval.h"

namespace STL {

/**
 * @brief STL formula type
 * 
 * This enumeration lists the admitted types 
 * for an STL formula
 */
enum formula_type {
  ATOM,
  CONJUNCTION,
  DISJUNCTION,
  UNTIL,
  ALWAYS,
  EVENTUALLY,
  NEGATION
};

/**
 * @brief STL formulas
 * 
 * This class represents generic STL formulas.
 */
class STL
{

protected:
  formula_type _type; //!< Type of the STL formula

  /**
   * @brief A STL formula constructor
   * 
   * @param type is the type of the new STL formula
   */
  STL(formula_type type);

  /**
   * @brief Print the STL formula in a stream
   * 
   * This method is publicly wrapped by the function
   * `operator<<(std::ostream&, const STL::STL &)`.
   * 
   * @param os is the output stream
   * @return a reference to the output stream
   */
  virtual std::ostream &print(std::ostream &os) const = 0;
public:

  /**
   * @brief Get the STL formula type
   * 
   * @return the type of the current STL formula
   */
  inline const formula_type &get_type() const
  {
    return _type;
  }

  /**
   * @brief Get an equivalent formula in Positive Normal Form (PNF)
   * 
   * A STL formula is in Positive Normal Form (PNF) if it does 
   * not use the negation operator. 
   * For any STL formula \f$\phi\f$ there exists a STL 
   * formula \f$\varphi\f$ in PNF such that 
   * \f[\models \phi \Longleftrightarrow \models \varphi.\f]
   * This method computes the STL formula in PNF of the 
   * current formula.
   * 
   * @return The equivalent STL formula in PNF
   */
  virtual const std::shared_ptr<STL> get_PNF() const = 0;

  /**
   * @brief Get the formula variables
   * 
   * @return the set of formula variables
   */
  virtual std::set<SymbolicAlgebra::Symbol<>> get_variables() const = 0;

  /**
   * @brief Get the formula time bounds 
   * 
   * @return the time interval affecting the formula sematics
   */
  virtual TimeInterval time_bounds() const;

  /**
   * @brief Destroy the STL formula
   */
  virtual ~STL();

  /**
   * @brief Print a STL formula in a stream
   * 
   * @param os is the output stream
   * @param formula is the formula to be printed
   * @return a reference to the output stream
   */
  friend std::ostream &operator<<(std::ostream &os, const STL &formula);
};

}
#endif /* STL_H_ */
