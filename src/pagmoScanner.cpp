/*
 * Copyright (c) 2014-2016 Kartik Kumar, Dinamica Srl (me@kartikkumar.com)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
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

#include "D2D/pagmoScanner.hpp"
#include "D2D/tools.hpp"

#include <pagmo/src/pagmo.h>

// #include <pagmo/src/population.h>
// #include <pagmo/src/algorithm/de.h>

// #include <pagmo/src/problems.h>
// #include <pagmo/src/problem/himmelblau.h>
// #include <pagmo/src/problem/thesis.h>


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

//! Execute pagmo_scanner.
void executePagmoScanner( const rapidjson::Document& config )
{
    const PagmoScannerInput input = checkPagmoScannerInput( config );

    // checkPagmoScannerInput
    std::ifstream catalogFile("/home/enne/work/d2d/data/catalogs/thesis_group_big_objects.txt");
    // std::ifstream catalogFile("/home/enne/d2d/data/catalogs/thesis_group_big_objects.txt");

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
    std::cout << "enne" << std::endl;
    const double departureEpochUpperBound = 14*86400;
    const double timeOfFlightUpperBound = 2*86400;
    std::cout << "enne2" << std::endl;
    const DateTime initialEpoch  =  DateTime( 2016,1,12,12,0,0 );

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
            
            pagmo::problem::thesis thesis(  2,
                                            // departureObject,
                                            // arrivalObject,
                                            // initialEpoch,
                                            departureEpochUpperBound,
                                            timeOfFlightUpperBound);

            pagmo::problem::himmelblau prob;
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

            }
            a.push_back(pagmo::island(al3,thesis,10));
            a.evolve();
            // pagmo::population pop(thesis,100);
            
        }

    }
            // std::cout << get_solutions(a) << std::endl;
            std::cout << "djaklfajlkdjasklfdjsl" << std::endl;
return;
}

//! Check pagmo_scanner input parameters.
PagmoScannerInput checkPagmoScannerInput( const rapidjson::Document& config )
{
    const std::string catalogPath = find( config, "catalog" )->value.GetString( );
    std::cout << "Catalog                       " << catalogPath << std::endl;

    // const std::string databasePath = find( config, "database" )->value.GetString( );
    // std::cout << "Database                      " << databasePath << std::endl;

    // const ConfigIterator departureEpochIterator = find( config, "departure_epoch" );
    // std::map< std::string, int > departureEpochElements;
    // if ( departureEpochIterator->value.Size( ) == 0 )
    // {
    //     DateTime dummyEpoch;
    //     departureEpochElements[ "year" ]    = dummyEpoch.Year( );
    //     departureEpochElements[ "month" ]   = dummyEpoch.Month( );
    //     departureEpochElements[ "day" ]     = dummyEpoch.Day( );
    //     departureEpochElements[ "hours" ]   = dummyEpoch.Hour( );
    //     departureEpochElements[ "minutes" ] = dummyEpoch.Minute( );
    //     departureEpochElements[ "seconds" ] = dummyEpoch.Second( );
    // }

    // else
    // {
    //     departureEpochElements[ "year" ]    = departureEpochIterator->value[ 0 ].GetInt( );
    //     departureEpochElements[ "month" ]   = departureEpochIterator->value[ 1 ].GetInt( );
    //     departureEpochElements[ "day" ]     = departureEpochIterator->value[ 2 ].GetInt( );

    //     if ( departureEpochIterator->value.Size( ) > 3 )
    //     {
    //         departureEpochElements[ "hours" ] = departureEpochIterator->value[ 3 ].GetInt( );

    //         if ( departureEpochIterator->value.Size( ) > 4 )
    //         {
    //             departureEpochElements[ "minutes" ] = departureEpochIterator->value[ 4 ].GetInt( );

    //             if ( departureEpochIterator->value.Size( ) > 5 )
    //             {
    //                 departureEpochElements[ "seconds" ]
    //                     = departureEpochIterator->value[ 5 ].GetInt( );
    //             }
    //         }
    //     }
    // }

    // const DateTime departureEpoch( departureEpochElements[ "year" ],
    //                                departureEpochElements[ "month" ],
    //                                departureEpochElements[ "day" ],
    //                                departureEpochElements[ "hours" ],
    //                                departureEpochElements[ "minutes" ],
    //                                departureEpochElements[ "seconds" ] );

    // if ( departureEpochIterator->value.Size( ) == 0 )
    // {
    //     std::cout << "Departure epoch               TLE epoch" << std::endl;
    // }

    // else
    // {
    //     std::cout << "Departure epoch               " << departureEpoch << std::endl;
    // }

    // const double departureEpochRange
    //     = find( config, "departure_epoch_grid" )->value[ 0 ].GetDouble( );
    // std::cout << "Departure epoch grid range    " << departureEpochRange << std::endl;
    // const double departureGridSteps
    //     = find( config, "departure_epoch_grid" )->value[ 1 ].GetDouble( );
    // std::cout << "Departure epoch grid steps    " << departureGridSteps << std::endl;

    // const double timeOfFlightMinimum
    //     = find( config, "time_of_flight_grid" )->value[ 0 ].GetDouble( );
    // std::cout << "Minimum Time-of-Flight        " << timeOfFlightMinimum << std::endl;
    // const double timeOfFlightMaximum
    //     = find( config, "time_of_flight_grid" )->value[ 1 ].GetDouble( );
    // std::cout << "Maximum Time-of-Flight        " << timeOfFlightMaximum << std::endl;

    // if ( timeOfFlightMinimum > timeOfFlightMaximum )
    // {
    //     throw std::runtime_error( "ERROR: Maximum time-of-flight must be larger than minimum!" );
    // }

    // const double timeOfFlightSteps
    //     = find( config, "time_of_flight_grid" )->value[ 2 ].GetDouble( );
    // std::cout << "# Time-of-Flight steps        " << timeOfFlightSteps << std::endl;

    // const bool isPrograde = find( config, "is_prograde" )->value.GetBool( );
    // if ( isPrograde )
    // {
    //     std::cout << "Prograde transfer?            true" << std::endl;
    // }
    // else
    // {
    //     std::cout << "Prograde transfer?            false" << std::endl;
    // }

    // const int revolutionsMaximum = find( config, "revolutions_maximum" )->value.GetInt( );
    // std::cout << "Maximum revolutions           " << revolutionsMaximum << std::endl;

    const int shortlistLength = find( config, "shortlist" )->value[ 0 ].GetInt( );
    std::cout << "# of shortlist transfers      " << shortlistLength << std::endl;

    std::string shortlistPath = "";
    if ( shortlistLength > 0 )
    {
        shortlistPath = find( config, "shortlist" )->value[ 1 ].GetString( );
        std::cout << "Shortlist                     " << shortlistPath << std::endl;
    }

    return PagmoScannerInput( catalogPath,
                                // databasePath,
                                // departureEpoch,
                                // departureGridSteps,
                                // departureEpochRange/departureGridSteps,
                                // timeOfFlightMinimum,
                                // timeOfFlightMaximum,
                                // timeOfFlightSteps,
                                // ( timeOfFlightMaximum - timeOfFlightMinimum ) / timeOfFlightSteps,
                                // isPrograde,
                                // revolutionsMaximum,
                                // shortlistLength,
                                shortlistPath );
}

} // namespace d2d
