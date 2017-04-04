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
    for (pagmo::archipelago::size_type i = 0; i< a.get_size(); ++i) 
    {
        sol << "island " << i << " ";
        sol << a.get_island(i)->get_algorithm()->get_name() ;
        // sol << "f = " << a.get_island(i)->get_f() ;
        // sol << " cr = " << a.get_island(i)->get_algorithm()->get_cr() ;

        sol << " : (";
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
    

    SQLite::Database database( input.databasePath.c_str( ),
                               SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE );


    createPagmoScannerTable(database);
    // checkPagmoScannerInput
    std::ifstream catalogFile( input.catalogPath.c_str( ) );
    
    // std::ifstream catalogFile("/home/enne/work/d2d/data/catalogs/thesis_group_big_objects.txt");
    // std::ifstream catalogFile("/home/enne/d2d/data/catalogs/thesis_group_big_objects.txt");
    // std::ifstream catalogFile("/home/enne/work/d2d/data/catalogs/just2.txt");
    std::string catalogLine;

    const int tleLines = 3;
    // Reset file stream to start of file.
    catalogFile.seekg( 0, std::ios::beg );
    int counter = 1;
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
    std::ostringstream pagmoScannerTableInsert;
    pagmoScannerTableInsert
        << "INSERT INTO pagmo_scanner_results VALUES ("
        << "NULL,"
        << ":algorithm,"
        << ":run_number,"
        << ":number_of_generations,"
        << ":generation,"
        << ":population_size,"
        << ":f_variable,"
        << ":cr_variable,"
        << ":transfer_delta_v"
        << ");";

    SQLite::Statement query( database, pagmoScannerTableInsert.str( ) );


    int numberOfLegs = 4;
    int dimensionOfProblem = 3*numberOfLegs+1;
    double stayTime = 86400.0;
    const double departureEpochUpperBound = 14 * 86400;
    const double timeOfFlightUpperBound = 2 * 86400;
    DateTime initialEpoch = DateTime( 2016,1,12,12,0,0 );
    pagmo::problem::thesis_multi thesis_multi(  dimensionOfProblem,
                                                numberOfLegs,
                                                stayTime,
                                                departureEpochUpperBound,
                                                timeOfFlightUpperBound,
                                                initialEpoch,
                                                tleObjects);


    int dim = 3*numberOfLegs+1;
    std::cout << dim << std::endl;
    // for (int i = 0; i < 10; ++i)
    // {
    //     pagmo::population populationSGA(thesis_multi,13*dim);
    //     pagmo::algorithm::sga mixedIntegerAlgo(5000);

    //     mixedIntegerAlgo.evolve(populationSGA);

    //     std::cout << populationSGA.champion().x << std::endl;
    //     std::cout << populationSGA.champion().f << std::endl;
    // }

    double popSizeSGA = 13*dim;
    int runs = 20;
    double f_variable = 0.8;
    double cr_variable = 0.9;
    int numberOfGenSGA = 5000;
    for (int run_number = 1; run_number < runs; ++run_number)
    {
        pagmo::population populationSGA( thesis_multi, popSizeSGA );
        pagmo::algorithm::sga algorithmSGA( 1 );
                                    

        query.bind( ":algorithm" , algorithmSGA.get_name() );
        query.bind( ":run_number" , run_number );
        query.bind( ":number_of_generations" , numberOfGenSGA );
        query.bind( ":generation" , 0 );
        query.bind( ":population_size" , popSizeSGA );
        query.bind( ":f_variable" , f_variable );
        query.bind( ":cr_variable" , cr_variable );
        query.bind( ":transfer_delta_v" , populationSGA.champion().f[0] );
        // Execute insert query.
        query.executeStep( );
        // Reset SQL insert query.
        query.reset( );                  

        double oldchamp = 1.0;
        double newchamp = 1.0;
        for( int i = 1; i < numberOfGenSGA; ++i )
        {
            // Proceed with next round of optimising
            algorithmSGA.evolve( populationSGA );
            newchamp = populationSGA.champion().f[0];
            if (newchamp  < oldchamp )
            {
                query.bind( ":algorithm" , algorithmSGA.get_name() );
                query.bind( ":run_number" , run_number );
                query.bind( ":number_of_generations" , numberOfGenSGA );
                query.bind( ":generation" , i );
                query.bind( ":population_size" , popSizeSGA );
                query.bind( ":f_variable" , f_variable );
                query.bind( ":cr_variable" , cr_variable );
                query.bind( ":transfer_delta_v" , populationSGA.champion().f[0] );
                // Execute insert query.
                query.executeStep( );
                // Reset SQL insert query.
                query.reset( );
            }
            oldchamp = newchamp;
        }
    }


    // if ( static_cast< int >(populationSGA.champion().f[0]) == 55555555)
    // {
    //     std::cout << "No sequence was found.." << std::endl;
    // }
    // else
    // {
    //     for (int j = 0; j < numberOfLegs; ++j)
    //     {
    //         int departureIterator = static_cast< int >(populationSGA.champion().x[j]);
    //         int arrivalIterator = static_cast< int >(populationSGA.champion().x[j+1]);
    
    //         double legDepartureEpoch = populationSGA.champion().x[numberOfLegs+1+j*2];
    //         double legTimeOfFlight = populationSGA.champion().x[numberOfLegs+2+j*2];
    //         double legArrivalEpoch = legDepartureEpoch + legTimeOfFlight;

    //         Tle departureObject = tleObjects[departureIterator];
    //         Tle arrivalObject = tleObjects[arrivalIterator];
            
    //         SGP4 sgp4Departure( departureObject );
    //         DateTime departureEpoch = initialEpoch.AddSeconds( legDepartureEpoch );
    //         Eci tleDepartureState = sgp4Departure.FindPosition( departureEpoch );

    //         boost::array< double, 3 > departurePosition;
    //         departurePosition[ 0 ] = tleDepartureState.Position( ).x;
    //         departurePosition[ 1 ] = tleDepartureState.Position( ).y;
    //         departurePosition[ 2 ] = tleDepartureState.Position( ).z;
            
    //         boost::array< double, 3 > departureVelocity;
    //         departureVelocity[ 0 ] = tleDepartureState.Velocity( ).x;
    //         departureVelocity[ 1 ] = tleDepartureState.Velocity( ).y;
    //         departureVelocity[ 2 ] = tleDepartureState.Velocity( ).z;

    //         // Define arrival position:
    //         DateTime arrivalEpoch = departureEpoch.AddSeconds( legTimeOfFlight );
    //         SGP4 sgp4Arrival( arrivalObject );
    //         Eci tleArrivalState   = sgp4Arrival.FindPosition( arrivalEpoch );

    //         boost::array< double, 3 > arrivalPosition;
    //         arrivalPosition[ 0 ] = tleArrivalState.Position( ).x;
    //         arrivalPosition[ 1 ] = tleArrivalState.Position( ).y;
    //         arrivalPosition[ 2 ] = tleArrivalState.Position( ).z;

    //         boost::array< double, 3 > arrivalVelocity;
    //         arrivalVelocity[ 0 ] = tleArrivalState.Velocity( ).x;
    //         arrivalVelocity[ 1 ] = tleArrivalState.Velocity( ).y;
    //         arrivalVelocity[ 2 ] = tleArrivalState.Velocity( ).z;

    //         kep_toolbox::lambert_problem targeter(  departurePosition,
    //                                                 arrivalPosition,
    //                                                 legTimeOfFlight,
    //                                                 kMU,
    //                                                 1,
    //                                                 50 );

    //         const int numberOfSolutions = targeter.get_v1( ).size( );

    //         // Compute Delta-Vs for transfer and determine index of lowest.
    //         typedef std::vector< Vector3 > VelocityList;
    //         VelocityList departureDeltaVs( numberOfSolutions );
    //         VelocityList arrivalDeltaVs( numberOfSolutions );

    //         typedef std::vector< double > TransferDeltaVList;
    //         TransferDeltaVList transferDeltaVs( numberOfSolutions );

    //         for ( int i = 0; i < numberOfSolutions; i++ )
    //         {
    //             // Compute Delta-V for transfer.
    //             const Vector3 transferDepartureVelocity = targeter.get_v1( )[ i ];
    //             const Vector3 transferArrivalVelocity = targeter.get_v2( )[ i ];

    //             departureDeltaVs[ i ] = sml::add( transferDepartureVelocity,
    //                                               sml::multiply( departureVelocity, -1.0 ) );
    //             arrivalDeltaVs[ i ]   = sml::add( arrivalVelocity,
    //                                               sml::multiply( transferArrivalVelocity, -1.0 ) );

    //             transferDeltaVs[ i ]
    //                 = sml::norm< double >( departureDeltaVs[ i ] )
    //                     + sml::norm< double >( arrivalDeltaVs[ i ] );
    //         }

    //         const TransferDeltaVList::iterator minimumDeltaVIterator
    //             = std::min_element( transferDeltaVs.begin( ), transferDeltaVs.end( ) );

    //         std::cout   << "Leg "
    //                     << j
    //                     << " "
    //                     << tleObjects[departureIterator].NoradNumber() 
    //                     << " to " 
    //                     << tleObjects[arrivalIterator].NoradNumber() 
    //                     << std::endl
    //                     << legDepartureEpoch/86400 
    //                     << " + "
    //                     << legTimeOfFlight/86400 
    //                     << " = " 
    //                     << legArrivalEpoch/86400 
    //                     << " dV: " 
    //                     << *minimumDeltaVIterator 
    //                     << std::endl;
            


        // }
// }



    // std::cout << counter << std::endl;
    return;
}




//! Check pagmo_scanner input parameters.
PagmoScannerInput checkPagmoScannerInput( const rapidjson::Document& config )
{
    const std::string catalogPath = find( config, "catalog" )->value.GetString( );
    std::cout << "Catalog                       " << catalogPath << std::endl;

    const std::string databasePath = find( config, "database" )->value.GetString( );
    std::cout << "Database                      " << databasePath << std::endl;

    const ConfigIterator initialEpochIterator = find( config, "initial_epoch" );
    std::map< std::string, int > initialEpochElements;
    if ( initialEpochIterator->value.Size( ) == 0 )
    {
        DateTime dummyEpoch;
        initialEpochElements[ "year" ]    = dummyEpoch.Year( );
        initialEpochElements[ "month" ]   = dummyEpoch.Month( );
        initialEpochElements[ "day" ]     = dummyEpoch.Day( );
        initialEpochElements[ "hours" ]   = dummyEpoch.Hour( );
        initialEpochElements[ "minutes" ] = dummyEpoch.Minute( );
        initialEpochElements[ "seconds" ] = dummyEpoch.Second( );
    }

    else
    {
        initialEpochElements[ "year" ]    = initialEpochIterator->value[ 0 ].GetInt( );
        initialEpochElements[ "month" ]   = initialEpochIterator->value[ 1 ].GetInt( );
        initialEpochElements[ "day" ]     = initialEpochIterator->value[ 2 ].GetInt( );

        if ( initialEpochIterator->value.Size( ) > 3 )
        {
            initialEpochElements[ "hours" ] = initialEpochIterator->value[ 3 ].GetInt( );

            if ( initialEpochIterator->value.Size( ) > 4 )
            {
                initialEpochElements[ "minutes" ] = initialEpochIterator->value[ 4 ].GetInt( );

                if ( initialEpochIterator->value.Size( ) > 5 )
                {
                    initialEpochElements[ "seconds" ]
                        = initialEpochIterator->value[ 5 ].GetInt( );
                }
            }
        }
    }

    const DateTime initialEpoch(   initialEpochElements[ "year" ],
                                   initialEpochElements[ "month" ],
                                   initialEpochElements[ "day" ],
                                   initialEpochElements[ "hours" ],
                                   initialEpochElements[ "minutes" ],
                                   initialEpochElements[ "seconds" ] );

    if ( initialEpochIterator->value.Size( ) == 0 )
    {
        std::cout << "Initial epoch             TLE epoch" << std::endl;
    }

    else
    {
        std::cout << "Initial epoch             " << initialEpoch << std::endl;
    }
    
    const int numberOfLegs
        = find( config, "number_of_legs" )->value.GetInt( );
    std::cout << "Number of legs                " << numberOfLegs << std::endl;

    const double departureEpochUpperBound
        = find( config, "departure_epoch_upper_bound" )->value.GetDouble( );
    std::cout << "Departure epoch upper bound   " << departureEpochUpperBound << std::endl;

    const double timeOfFlightUpperBound
        = find( config, "time_of_flight_upper_bound" )->value.GetDouble( );
    std::cout << "Time of flight upper bound    " << timeOfFlightUpperBound << std::endl;

    const double stayTime
        = find( config, "stay_time" )->value.GetDouble( );
    std::cout << "Stay time                     " << stayTime << std::endl;

    return PagmoScannerInput(   catalogPath,
                                databasePath,
                                initialEpoch,
                                numberOfLegs,
                                departureEpochUpperBound,
                                timeOfFlightUpperBound,
                                stayTime);
}

//! Create pagmo_scanner table.
void createPagmoScannerTable( SQLite::Database& database )
{
    std::cout << "Creating Pagmo database...." << std::endl;
    // Drop table from database if it exists.
    database.exec( "DROP TABLE IF EXISTS pagmo_scanner_results;" );
    
    // Set up SQL command to create table to store pagmo_scanner results.
    std::ostringstream pagmoScannerTableCreate;
    pagmoScannerTableCreate
        << "CREATE TABLE pagmo_scanner_results ("
        << "\"transfer_id\"                             INTEGER PRIMARY KEY AUTOINCREMENT,"
            // << "\"departure_object_id\"                     TEXT,"
            // << "\"arrival_object_id\"                       TEXT,"
            // << "\"departure_epoch\"                         REAL,"
            // << "\"time_of_flight\"                          REAL,"
        << "\"algorithm\"                               TEXT,"
        << "\"run_number\"                              INTEGER,"
        << "\"number_of_generations\"                   INTEGER,"
        << "\"generation\"                              INTEGER,"
        << "\"population_size\"                         INTEGER,"
        << "\"f_variable\"                              REAL,"
        << "\"cr_variable\"                             REAL,"
        << "\"transfer_delta_v\"                        REAL"
        <<                                              ");";


    // Execute command to create table.
    database.exec( pagmoScannerTableCreate.str( ).c_str( ) );

    // Execute command to create index on transfer Delta-V column.
    std::ostringstream transferDeltaVIndexCreate;
    transferDeltaVIndexCreate << "CREATE INDEX IF NOT EXISTS \"transfer_delta_v\" on "
                              << "pagmo_scanner_results (transfer_delta_v ASC);";
    database.exec( transferDeltaVIndexCreate.str( ).c_str( ) );

        // throw std::runtime_error( "ERROR: Creating table 'pagmo_scanner_results' failed!" );
    
    
    if ( !database.tableExists( "pagmo_scanner_results" ) )
    {
        std::cout << "Table exists, results will be appended." << std::endl;
    }
    std::cout << "Pagmo database created!" << std::endl;

}


} // namespace d2d
