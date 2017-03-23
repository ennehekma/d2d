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

#include <sqlite3.h>

#include <SML/sml.hpp>
#include <Astro/astro.hpp>
// #include "pagmo.h"
#include "D2D/lambertScanner.hpp"
#include "D2D/tools.hpp"

#include <iomanip>
#include <pagmo/src/population.h>
#include <pagmo/src/pagmo.h>

// #include <pagmo/src/algorithm/jde.h>

#include <pagmo/src/problem/himmelblau.h>
// #include <pagmo/src/problem/gtoc_2.h>
#include <pagmo/src/algorithm/de.h>


#include <pagmo/src/problem/thesis.h>
#include <pagmo/src/problems.h>


namespace d2d
{

std::string get_solutions(pagmo::archipelago a) {
	std::ostringstream sol;
	int sol_size = a.get_island(0)->get_population().champion().x.size();
	int fit_size = a.get_island(0)->get_population().champion().f.size();
	for (pagmo::archipelago::size_type i = 0; i< a.get_size(); ++i) {
		sol << "island " << i ;
		sol << a.get_island(i)->get_algorithm()->get_name() ;
		sol << ": (";
		for(int j = 0; j < sol_size; ++j) {
			sol << a.get_island(i)->get_population().champion().x[j] << ",";
		}
		sol << " with fitness: ";
		for(int j = 0; j < fit_size; ++j) {
			sol << a.get_island(i)->get_population().champion().f[j] << ",";
		}
		sol << ")" << std::endl;
	}
	return sol.str();
}

//! Execute lambert_scanner.
void executeLambertScanner( const rapidjson::Document& config )
{
	std::ifstream catalogFile("/home/enne/work/d2d/data/catalogs/thesis_group_big_objects.txt");
	// std::ifstream catalogFile("/home/enne/work/d2d/data/catalogs/just2.txt");
    std::string catalogLine;

	const int tleLines = 3;
	// Reset file stream to start of file.
    catalogFile.seekg( 0, std::ios::beg );

    typedef std::vector< std::string > TleStrings;
    typedef std::vector< Tle > TleObjects;
    TleObjects tleObjects;

    while ( std::getline( catalogFile, catalogLine ) )
    {
        TleStrings tleStrings;
        removeNewline( catalogLine );
        tleStrings.push_back( catalogLine );
        std::getline( catalogFile, catalogLine );
        removeNewline( catalogLine );
        tleStrings.push_back( catalogLine );

        if ( tleLines == 3 )
        {
            std::getline( catalogFile, catalogLine );
            removeNewline( catalogLine );
            tleStrings.push_back( catalogLine );
            tleObjects.push_back( Tle( tleStrings[ 0 ], tleStrings[ 1 ], tleStrings[ 2 ] ) );
        }

        else if ( tleLines == 2 )
        {
            tleObjects.push_back( Tle( tleStrings[ 0 ], tleStrings[ 1 ] ) );
        }
    }

    catalogFile.close( );
    std::cout << tleObjects.size( ) << " TLE objects parsed from catalog!" << std::endl;
	const DateTime initialEpoch  =  DateTime( 2016,1,12,12,0,0 );
	const double departureEpochUpperBound = 14*86400;
	const double timeOfFlightUpperBound = 2*86400;

	std::map< std::pair< int, int >, double >  overallBest;

    // Loop over all departure objects.
    for ( unsigned int i = 0; i < tleObjects.size( ); i++ )
    {
        // Compute departure state.
        Tle departureObject = tleObjects[ i ];


        // Loop over arrival objects.
        for ( unsigned int j = 0; j < tleObjects.size( ); j++ )
        {
            // Skip the case of the departure and arrival objects being the same.
            if ( i == j )
            {
                continue;
            }

            Tle arrivalObject = tleObjects[ j ];

            const int arrivalObjectId = static_cast< int >( arrivalObject.NoradNumber( ) );
            const int departureObjectId = static_cast< int >( departureObject.NoradNumber( ) );
            std::pair<int,int> combo = std::make_pair(departureObjectId,arrivalObjectId);
            
			pagmo::problem::thesis thesis(	2,
											departureObject,
											arrivalObject,
											initialEpoch,
											departureEpochUpperBound,
											timeOfFlightUpperBound);
			// pagmo::algorithm::de  al1(5000);
			// pagmo::algorithm::sga al2(5000);
			pagmo::algorithm::jde al3(50);


            
			pagmo::archipelago a;
		    a.set_topology(pagmo::topology::unconnected());
		    // for (int i = 0; i < 20; ++i) 
		    for (int i = 0; i < 2; ++i) 
		    {
		    	// a.push_back(pagmo::island(al1,thesis,1000));
		    	// a.push_back(pagmo::island(al2,thesis,1000));
		    	a.push_back(pagmo::island(al3,thesis,10));

			}
			a.evolve();
			std::vector<double> best;
			
		    for (unsigned int j = 0; j < a.get_size(); ++j)
		    {
				best.push_back(a.get_island(j)->get_population().champion().f[0]);
				// std::cout <<  a.get_island(j)->get_population().champion().x <<  a.get_island(j)->get_population().champion().f << std::endl;
		    }
		    for (unsigned int i = 0; i < best.size(); ++i)
		    {
		    	// std::cout << best[i] << std::endl;
		    }
		    std::sort(best.begin(),best.end());
	
			if (best[0]<1)
			{
				std::cout << "" << std::endl;
            	std::cout << departureObjectId<< " " << arrivalObjectId << std::endl;
				std::cout << best[0]  << std::endl;
			    overallBest.insert( std::pair< std::pair<int,int>,double>(combo,best[0]));
			}	    
		}

	}
	std::cout << "" << std::endl;
	for (std::map< std::pair< int, int >, double >::iterator it = overallBest.begin();
		it != overallBest.end();
		it++)
	{
		std::cout << it->first.first << " " << it->first.second << " " << it->second << std::endl;
	}
	// for (int i = 0; i < overallBest.size(); ++i)
	// {
	// 	std::cout << overallBest[][i] << std::endl;
	// }


            
            
            
    // }
    
	// std::vector<double> output(20);
 //    // Evolve the archipelago 10 times.
 //    for (int i = 0; i < 1; ++i)
 //    {	
 //    	std::cout << "Generation " << i << std::endl;
 //    	a.evolve(1);
	//     a.join();
 //    	// output[i] = a.get_island(1)->get_population().champion().f[0];
 //    	// std::cout << output[i] << std::endl;
	//     std::cout << "" << std::endl;
	//     std::cout << "" << std::endl;
 //    }

	// pagmo::algorithm::jde al1(100);
	// pagmo::algorithm::sga al3(1000);
 //    std::cout << get_solutions(a) << std::endl;
 //    std::cout << "Done .. the champion is ... " <<std::endl;
 //    // std::cout << pop.champion().x << std::endl;
 //    std::cout << "With fitness" << std::endl;
 //    // std::cout << pop.champion().f << std::endl;
	// std::cout << a.human_readable() << std::endl;

    // const DateTime initialEpoch(2016,1,12,0,0,0);
    // std::cout << initialEpoch.ToJulian()-2457000 << std::endl;
    // std::vector<boost::shared_ptr< pagmo::base_island > > test = a.get_islands();

    // for (int i = 0; i < 10; ++i)
    // {
	// pagmo::population pop = test[0]->get_population();
    // }
        // Create an archipelago of 10 MPI islands.
	// std::cout << thesis.get_lb() << std::endl;
	// std::cout << thesis.get_ub() << std::endl;

	// pagmo::population pop(thesis,100);

	
	// // std::cout << thesis.get_departure_object().Line1() << std::endl;
	// algo.set_screen_output(true);
	// for (int i = 0; i < 20; ++i)
	// {
	// 	algo.evolve(pop);
	// }

		/* code */


	// std::vector<kep_toolbox::planet::planet_ptr> retval;
	// retval.push_back(kep_toolbox::planet::jpl_lp("earth").clone());
	// retval.push_back(kep_toolbox::planet::jpl_lp("venus").clone());
            // a.push_back(pagmo::island(al2,thesis,1000));
            // a.push_back(pagmo::island(al3,thesis,1000));``



            // a.push_back(pagmo::island(algo1,thesis,100));
            // a.push_back(pagmo::island(algo2,thesis,100));
            // a.push_back(pagmo::island(algo3,thesis,100));
            // a.push_back(pagmo::island(algo4,thesis,100));
            // a.push_back(pagmo::island(algo5,thesis,100));
            // a.push_back(pagmo::island(algo6,thesis,100));
            // a.push_back(pagmo::island(algo7,thesis,100));
            // a.push_back(pagmo::island(algo8,thesis,100));
            // a.push_back(pagmo::island(algo9,thesis,100));
            // a.push_back(pagmo::island(algo10,thesis,100));
            // a.push_back(pagmo::island(algo11,thesis,100));
            // a.push_back(pagmo::island(algo12,thesis,100));
            // a.push_back(pagmo::island(algo13,thesis,100));
            // a.push_back(pagmo::island(algo14,thesis,100));
            // a.push_back(pagmo::island(algo15,thesis,100));
            // a.push_back(pagmo::island(algo16,thesis,100));
            // a.push_back(pagmo::island(algo17,thesis,100));
            // a.push_back(pagmo::island(algo18,thesis,100));
	// // retval.push_back(kep_toolbox::planet::jpl_lp("earth").clone());
	// // retval.push_back(kep_toolbox::planet::jpl_lp("venus").clone());
	// // retval.push_back(kep_toolbox::planet::jpl_lp("earth").clone());
	// retval.push_back(kep_toolbox::planet::jpl_lp("venus").clone());
	// retval.push_back(kep_toolbox::planet::jpl_lp("earth").clone());
	// pagmo::problem::mga_1dsm_tof enne(retval);
	// pagmo::population test(enne,200);
	// pagmo::problem::thesis thesis(5,0.63,Tle("1 35683U 09041C   12289.23158813  .00000484  00000-0  89219-4 0  5863","2 35683  98.0221 185.3682 0001499 100.5295 259.6088 14.69819587172294"));
	// std::cout << thesis.get_member() << std::endl;
	// std::cout << thesis.get_tle1() << std::endl;

	

	// pagmo::problem::thesis thesis;

 	// std::vector<double>::size_type dim(2);
  //   std::vector<double> lb(dim);
  //   std::vector<double> ub(dim);
  //   lb[0] = 0.;
  //   lb[1] = 0.;
  //   ub[0] = 100.;
  //   ub[1] = 100.;
  //   std::cout << (lb.size() != ub.size()) << std::endl;
    // pagmo::mpi_environment env;
 //    Tle departureObject    = Tle(
 //        "1 23324U 94068B   16010.51334980  .00000012  00000-0  28469-4 0  9998",
 //        "2 23324  98.9963 332.6453 0056551 296.3224 170.0557 14.17025685 98063");
 //    Tle arrivalObject      = Tle(
 //        "1 37932U 11068C   16010.87903564  .00000062  00000-0  46596-4 0  9992",
 //        "2 37932  98.4650 332.1884 0038025 123.5258 261.2969 14.21842293214931");
	// const DateTime initialEpoch  =  DateTime( 2016,1,12,12,0,0 );

	// pagmo::algorithm::de algo1(10,0.8,0.9,1);
	// pagmo::algorithm::de algo2(10,0.8,0.9,2);
	// pagmo::algorithm::de algo3(10,0.8,0.9,3);
	// pagmo::algorithm::de algo4(10,0.8,0.9,4);
	// pagmo::algorithm::de algo5(10,0.8,0.9,5);
	// pagmo::algorithm::de algo6(10,0.8,0.9,6);
	// pagmo::algorithm::de algo7(10,0.8,0.9,7);
	// pagmo::algorithm::de algo8(10,0.8,0.9,8);
	// pagmo::algorithm::de algo9(10,0.8,0.9,9);
	// pagmo::algorithm::de algo10(10,0.8,0.9,10);
	
	// pagmo::algorithm::jde algo1(100,1);
	// pagmo::algorithm::jde algo2(100,2);
	// pagmo::algorithm::jde algo3(100,3);
	// pagmo::algorithm::jde algo4(100,4);
	// pagmo::algorithm::jde algo5(100,5);
	// pagmo::algorithm::jde algo6(100,6);
	// pagmo::algorithm::jde algo7(100,7);
	// pagmo::algorithm::jde algo8(100,8);
	// pagmo::algorithm::jde algo9(100,9);
	// pagmo::algorithm::jde algo10(100,10);
	// pagmo::algorithm::jde algo11(100,11);
	// pagmo::algorithm::jde algo12(100,12);
	// pagmo::algorithm::jde algo13(100,13);
	// pagmo::algorithm::jde algo14(100,14);
	// pagmo::algorithm::jde algo15(100,15);
	// pagmo::algorithm::jde algo16(100,16);
	// pagmo::algorithm::jde algo17(100,17);
	// pagmo::algorithm::jde algo18(100,18);


    // tialise the MPI environment.
        // Create a problem and an algorithm.
        // problem::dejong prob(10);
        // algorithm::monte_carlo algo(100);

	// // thesis.human_readable_extra();

	// pagmo::problem::himmelblau prob2;
	// algo.evolve(pop);
	// algo.evolve(pop);


	// pagmo::problem::prob_A enne;

	//Instantiate the problem with default 10 segments
	// pagmo::problem::gtoc_2 prob;//(815,300,110,47);

	// //Create a population containing a single random individual
	// population pop(prob,1);

	// //Set the Turin solution epochs
 // 	decision_vector tmp = pop.get_individual(0).cur_x;
	// tmp[0] = 59870; tmp[1] = 60283 - 59870; tmp[2] = 60373 - 60283;
 // 	tmp[3] = 61979 - 60373; tmp[4] = 62069 - 61979; tmp[5] = 62647 - 62069;
	// tmp[6] = 62737 - 62647; tmp [7] = 63196 - 62737;
	// tmp[8] = 1400; tmp[9] = 1200; tmp[10]= 1100; tmp[11] = 1000;
 // 	pop.set_x(0, tmp);

	// //Instantiate the algorithm
	// algorithm::snopt algo(1000,1E-9,1E-9);
	// algo.set_screen_output(true);
	
	// //Create the island  
	// island isl(algo,pop);

	// //Solve the problem
	// isl.evolve();

	// std::cout << isl << '\n';

	// return 0;










	// std::cout << pop << std::endl;



 //    // pagmo::problem::prob_A prob(5);
 //    // algo.evolve(pop);
    return;
}
} // namespace d2d
