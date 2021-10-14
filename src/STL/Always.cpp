/**
 * @file Always.cpp
 * Always STL formula
 *
 * @author Tommaso Dreossi <tommasodreossi@berkeley.edu>
 * @version 0.1
 */

#include "Always.h"

/**
 * Constructor that instantiates an Always formula (G_[a,b]f)
 *
 * @param[in] a beginning of temporal interval
 * @param[in] b end of temporal interval
 * @param[in] f subformula
 */
Always::Always(int a, int b, const std::shared_ptr<STL> f): STL(ALWAYS), f(f), a(a), b(b) {}

/**
 * Print the formula
 */
void Always::print() const {
	cout<<"always_["<<this->a<<","<<this->b<<"] (";
	this->f->print();
	cout<<")";
}

Always::~Always() 
{}
