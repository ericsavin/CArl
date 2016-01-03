/*
 * common_header.h
 *
 *  Created on: Jul 29, 2015
 *      Author: Thiago Milanetto Schlittler
 *
 *      File containing all the external libraries includes (except the CGAL
 *	ones, included in "CGAL_typedefs")
 *
 */

#ifndef COMMON_HEADER_H_
#define COMMON_HEADER_H_

#include <unistd.h>

// --- Timing
#include <chrono>

// --- Boost/random
#include <boost/random/uniform_real_distribution.hpp>
#include <boost/random/lagged_fibonacci.hpp>

// --- Boost/variant and functional
#include <boost/variant/apply_visitor.hpp>
#include <functional>

// --- Boost/filesystem
#include <boost/filesystem.hpp>

// --- IO
#include <fstream>
#include <iostream>

// --- Containers
#include <unordered_set>
#include <unordered_map>
#include <deque>
#include <vector>
#include <iterator>

// --- C++ strings
#include <string>
#include <sstream>

extern boost::random::lagged_fibonacci607 m_rng;

// A small homemade assert
#ifndef NDEBUG
	#define homemade_assert_msg(asserted, msg) do \
	{ \
		if (!(asserted)) \
		{ \
			std::cerr << "Assertion `" #asserted "` failed in " << __FILE__ \
                    << " line " << __LINE__ << ": " <<  msg << std::endl; \
			std::exit(EXIT_FAILURE); \
		} \
	} while(false)
#else
	#define homemade_assert_msg(asserted, msg) do { } while (false)
#endif

#endif /* COMMON_HEADER_H_ */