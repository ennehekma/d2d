/*
 * Copyright (c) 2014-2016 Kartik Kumar, Dinamica Srl (me@kartikkumar.com)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <sstream>
#include <stdexcept>
#include <vector>

#include <boost/progress.hpp>

#include <libsgp4/Eci.h>
#include <libsgp4/Globals.h>
#include <libsgp4/SGP4.h>
#include <libsgp4/Tle.h>

#include <SML/sml.hpp>
#include <Astro/astro.hpp>

#include "D2D/lambertScanner.hpp"
#include "D2D/tools.hpp"

#include <iomanip>
#include <pagmo/src/population.h>
// #include <pagmo/src/algorithm/jde.h>

#include <pagmo/src/problem/prob_A.h>

namespace d2d
{

//! Execute lambert_scanner.
void executeLambertScanner( const rapidjson::Document& config )
{
    // pagmo::problem::prob_A prob(5);
    // pagmo::population pop(prob,50);
    // pagmo::algorithm::jde algo(500);



    // algo.set_screen_output(true);
    // algo.evolve(pop);
    std::cout << "Done .. the champion is ... " <<std::endl;
    // std::cout << pop.champion().x << std::endl;
    return;
}
} // namespace d2d
