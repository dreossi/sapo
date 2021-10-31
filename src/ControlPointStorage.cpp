/**
 * @file ControlPointStorage.cpp
 * @brief Implements the ControlPointStorage class methods.
 *  
 * This file contains the implementation of the ControlPointStorage
 * class methods. This class is meant to represent control 
 * points for Bundle transformations.
 *
 * @author Alberto Casagrande <alberto.casagrande@units.it>
 * @version 0.1
 */

#include "ControlPointStorage.h"

ControlPointStorage::ControlPointStorage(const ControlPointStorage& orig)
{
    std::shared_lock<std::shared_timed_mutex> read_lock(orig._mutex, std::defer_lock);
    std::unique_lock<std::shared_timed_mutex> write_lock(_mutex, std::defer_lock);

    for (auto it = std::cbegin(orig._genF_ctrlP); it!= std::end(orig._genF_ctrlP); ++it) {
        _genF_ctrlP[it->first] = std::pair<GiNaC::lst, GiNaC::lst>(GiNaC::lst(it->second.first), GiNaC::lst(it->second.second));
    }
}

std::pair<GiNaC::lst, GiNaC::lst> ControlPointStorage::get(const std::vector<int> index) const
{
    std::shared_lock<std::shared_timed_mutex> read_lock(_mutex, std::defer_lock);

    return _genF_ctrlP.at(index);
}

bool ControlPointStorage::gen_fun_is_equal_to(const std::vector<int> index, const GiNaC::lst& genFun) const
{
    std::shared_lock<std::shared_timed_mutex> read_lock(_mutex, std::defer_lock);

    return _genF_ctrlP.at(index).first.is_equal(genFun);
}

GiNaC::lst ControlPointStorage::get_gen_fun(const std::vector<int> index) const
{
    std::shared_lock<std::shared_timed_mutex> read_lock(_mutex, std::defer_lock);

    return _genF_ctrlP.at(index).first;
}

GiNaC::lst ControlPointStorage::get_ctrl_pts(const std::vector<int> index) const
{
    std::shared_lock<std::shared_timed_mutex> read_lock(_mutex, std::defer_lock);

    return _genF_ctrlP.at(index).second;
}

bool ControlPointStorage::contains(const std::vector<int> index) const
{
    std::shared_lock<std::shared_timed_mutex> read_lock(_mutex, std::defer_lock);

    return (_genF_ctrlP.count(index)>0);
}

ControlPointStorage& ControlPointStorage::set_first(const std::vector<int> index, const GiNaC::lst& genFun)
{
    std::unique_lock<std::shared_timed_mutex> write_lock(_mutex, std::defer_lock);

    _genF_ctrlP[index].first=genFun;

    return *this;
}

ControlPointStorage& ControlPointStorage::set_second(const std::vector<int> index, const GiNaC::lst& ctrlPts)
{
    std::unique_lock<std::shared_timed_mutex> write_lock(_mutex, std::defer_lock);

    _genF_ctrlP[index].second=ctrlPts;

    return *this;
}

