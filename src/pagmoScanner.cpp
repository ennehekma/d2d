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

//! Execute pagmo_scanner.
void executePagmoScanner( const rapidjson::Document& config )
{
    const PagmoScannerInput input = checkPagmoScannerInput( config );
    
    SQLite::Database database( input.databasePath.c_str( ),
                               SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE );

    createPagmoScannerTable( database, input.numberOfLegs );
    
    std::cout << "Parsing TLE catalog ... " << std::endl;

    // Parse catalog and store TLE objects.
    std::ifstream catalogFile( input.catalogPath.c_str( ) );
    std::string catalogLine;

    // Check if catalog is 2-line or 3-line version.
    std::getline( catalogFile, catalogLine );
    const int tleLines = getTleCatalogType( catalogLine );

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

    std::vector< int > allObjects;
    for (int i = 0; i < tleObjects.size( ); ++i)
    {
        allObjects.push_back( tleObjects[i].NoradNumber( ) );
    }

    // BEGIN {Make list of crossectional areas}
    std::string satcatLine;
    std::ifstream satcatFile ( input.satcatPath.c_str( ) );
    std::map< int, double > allCrossSections;
    if ( satcatFile.is_open( ) )
    {
        bool skip = true;
        while ( getline ( satcatFile, satcatLine ) )
        {
            if ( skip == true ){ skip = false; continue; }

            std::string TleString = satcatLine.substr( 13, 18 );
            int TleInteger;
            if ( !( std::istringstream( TleString ) >> TleInteger ) ) TleInteger = 0;

            std::string RcsString = satcatLine.substr( 120, 127 );
            double radarCrossSection;
            if ( !( std::istringstream( RcsString ) >> radarCrossSection ) ) radarCrossSection = 0;

            if( std::find( allObjects.begin( ), allObjects.end( ), TleInteger ) 
                != allObjects.end( ) ) 
            {
                allCrossSections.insert ( 
                    std::pair< int, double >( TleInteger, radarCrossSection ) );
            }
            satcatLine.clear( );
        }
        satcatFile.close( );
    }

    std::ostringstream pagmoScannerTableInsert;
    
    pagmoScannerTableInsert
        << "INSERT INTO pagmo_scanner_results_" 
        << input.numberOfLegs
        << " VALUES ("
        << "NULL,"
        << ":algorithm,"
        << ":run_number,"
        << ":number_of_generations,"
        << ":generation,"
        << ":population_size,"
        << ":f_variable,"
        << ":cr_variable,"
        << ":strategy,";
    for (int i = 1; i < input.numberOfLegs + 2; ++i)
        {
            pagmoScannerTableInsert << ":object_" << i << ",";
        }    
    for (int i = 1; i < input.numberOfLegs + 2; ++i)
        {
            pagmoScannerTableInsert << ":object_" << i << "_area,";
        }    
    for (int i = 1; i < input.numberOfLegs + 1; ++i)
        {
            pagmoScannerTableInsert << ":departure_epoch_" << i << ",";
            pagmoScannerTableInsert << ":time_of_flight_" << i << ",";
        }    
    pagmoScannerTableInsert
        << ":removed_area,"
        << ":transfer_delta_v"
        << ");";

    SQLite::Statement query( database, pagmoScannerTableInsert.str( ) );

    std::vector< std::string > objectStrings;
    for (int i = 1; i < input.numberOfLegs + 2; ++i)
    {
        std::ostringstream objectString;
        objectString << ":object_" << i ;
        objectStrings.push_back( objectString.str( ) )    ;
    }
    std::vector< std::string > areaStrings;
    for (int i = 1; i < input.numberOfLegs + 2; ++i)
    {
        std::ostringstream areaString;
        areaString << ":object_" << i << "_area" ;
        areaStrings.push_back( areaString.str( ) )    ;
    }    

    std::vector< std::string > epochStrings;
    std::vector< std::string > timeOfFlightStrings;
    for (int i = 1; i < input.numberOfLegs + 1; ++i)
    {
        std::ostringstream epochString;
        epochString << ":departure_epoch_" << i ;
        epochStrings.push_back( epochString.str( ) );

        std::ostringstream timeOfFlightString;
        timeOfFlightString << ":time_of_flight_" << i ;
        timeOfFlightStrings.push_back( timeOfFlightString.str( ) );
    }

    int dimensionOfProblem = 3 * input.numberOfLegs + 1;
    pagmo::problem::thesis_multi thesis_multi(  dimensionOfProblem,
                                                input.numberOfLegs,
                                                input.stayTime,
                                                input.departureEpochUpperBound,
                                                input.timeOfFlightUpperBound,
                                                input.initialEpoch,
                                                tleObjects);
    int numberOfGeneration_DE = 5000;
    
    static const double arrFValues[] = { 0.2, 0.4, 0.6, 0.8, 1.0 };
    std::vector< double> vectorF_DE ( arrFValues, 
        arrFValues + sizeof( arrFValues ) / sizeof( arrFValues[ 0 ] ) );
    
    static const double arrCRValues[] = { 0.2, 0.4, 0.6, 0.8, 1.0 };
    std::vector<double> vectorCR_DE ( arrCRValues, 
        arrCRValues + sizeof( arrCRValues ) / sizeof( arrCRValues[ 0 ] ) );

    static const int arrPopulationMultiplierDE[] = { 10, 13, 20 };
    std::vector< int > vectorPopulationMultiplier_DE ( arrPopulationMultiplierDE, 
        arrPopulationMultiplierDE + sizeof( arrPopulationMultiplierDE ) 
        / sizeof( arrPopulationMultiplierDE[ 0 ] ) );    

    for (unsigned int i = 0; i < vectorPopulationMultiplier_DE.size( ); ++i)
    {
        int populationSize_DE = vectorPopulationMultiplier_DE[ i ] * dimensionOfProblem;
        std::cout << "Population size: " << populationSize_DE << std::endl;
    
        for (unsigned int j = 0; j < vectorF_DE.size( ); ++j)
        {
            double f_variable = vectorF_DE[ j ];
            std::cout << "    F value: " << f_variable << std::endl;
    
            for (unsigned int k = 0; k < vectorCR_DE.size( ); ++k)
            {
                double cr_variable = vectorCR_DE[ k ];
                std::cout << "        CR value: " << cr_variable << std::endl;

                for (int run_number = 1; run_number < input.numberOfRuns + 1; ++run_number)
                {
                    // Define population and algorithm 
                    pagmo::population populationDE(     thesis_multi, 
                                                        populationSize_DE );
                    
                    pagmo::algorithm::de algorithmDE(   1, 
                                                        f_variable, 
                                                        cr_variable, 
                                                        input.strategy );

                    // Store initial generation of population
                    query.bind( ":algorithm",              algorithmDE.get_name( ) );
                    query.bind( ":run_number",             run_number );
                    query.bind( ":number_of_generations",  numberOfGeneration_DE );
                    query.bind( ":generation",             0 );
                    query.bind( ":population_size",        populationSize_DE );
                    query.bind( ":f_variable",             f_variable );
                    query.bind( ":cr_variable",            cr_variable );
                    query.bind( ":strategy",               input.strategy );
                    for (int i = 0; i < input.numberOfLegs + 1; ++i)
                    {
                        int currentInteger = static_cast< int >(populationDE.champion( ).x[ i ]);
                        int currentTleNumber = static_cast< int >(tleObjects[currentInteger].NoradNumber( ));
                        query.bind( objectStrings[ i ], currentTleNumber );
                    }
                    double totalRemovedCrossSection = 0.0;
                    for (int i = 0; i < input.numberOfLegs + 1; ++i)
                    {
                        int currentInteger = static_cast< int >(populationDE.champion( ).x[ i ]);
                        int currentTleNumber = static_cast< int >(tleObjects[currentInteger].NoradNumber( ));
                        double currentRemovedCrossSection = 
                                    allCrossSections.find( currentTleNumber )->second;
                        totalRemovedCrossSection = 
                                    currentRemovedCrossSection +  totalRemovedCrossSection;
                        query.bind( areaStrings[ i ], currentRemovedCrossSection );
                    }
                    for (int i = 0; i < input.numberOfLegs; ++i)
                    {
                        query.bind( epochStrings[ i ],          
                            populationDE.champion( ).x[ input.numberOfLegs + 1 + i * 2]);
                        query.bind( timeOfFlightStrings[ i ],   
                            populationDE.champion( ).x[ input.numberOfLegs + 2 + i * 2]);
                    }                   
                    query.bind( ":removed_area",           totalRemovedCrossSection );
                    query.bind( ":transfer_delta_v",       populationDE.champion( ).f[ 0 ] );
                    // Execute insert query.
                    query.executeStep( );
                    // Reset SQL insert query.
                    query.reset( );
    
                    double oldChampion = 1.0;
                    double newChampion = 1.0;
                    for( int generation = 1; generation < numberOfGeneration_DE; ++generation )
                    {
                        // Proceed with next round of optimising
                        algorithmDE.evolve( populationDE );
                        newChampion = populationDE.champion( ).f[ 0 ];
                        // Check if new champion is significantly better compared to old (>0.1 m/s)
                        // if so, store the new value
                        if (newChampion < oldChampion - 0.0001 )
                        {
                            query.bind( ":algorithm",              algorithmDE.get_name( ) );
                            query.bind( ":run_number",             run_number );
                            query.bind( ":number_of_generations",  numberOfGeneration_DE );
                            query.bind( ":generation",             generation );
                            query.bind( ":population_size",        populationSize_DE );
                            query.bind( ":f_variable",             f_variable );
                            query.bind( ":cr_variable",            cr_variable );
                            query.bind( ":strategy",               input.strategy );
                            for (int i = 0; i < input.numberOfLegs + 1; ++i)
                            {
                                int currentInteger = static_cast< int >(populationDE.champion( ).x[ i ]);
                                int currentTleNumber = static_cast< int >(tleObjects[currentInteger].NoradNumber( ));
                                query.bind( objectStrings[ i ], currentTleNumber );
                            }
                            double totalRemovedCrossSection = 0.0;
                            for (int i = 0; i < input.numberOfLegs + 1; ++i)
                            {
                                int currentInteger = static_cast< int >(populationDE.champion( ).x[ i ]);
                                int currentTleNumber = static_cast< int >(tleObjects[currentInteger].NoradNumber( ));
                                double currentRemovedCrossSection = 
                                            allCrossSections.find( currentTleNumber )->second;
                                totalRemovedCrossSection = 
                                            currentRemovedCrossSection +  totalRemovedCrossSection;
                                query.bind( areaStrings[ i ], currentRemovedCrossSection );
                            }
                            for (int i = 0; i < input.numberOfLegs; ++i)
                            {
                                query.bind( epochStrings[ i ],          
                                    populationDE.champion( ).x[ input.numberOfLegs + 1 + i * 2 ] );
                                query.bind( timeOfFlightStrings[ i ],   
                                    populationDE.champion( ).x[ input.numberOfLegs + 2 + i * 2 ] );
                            }                   
                            query.bind( ":removed_area",        totalRemovedCrossSection );
                            query.bind( ":transfer_delta_v",    populationDE.champion( ).f[ 0 ] );
                            // Execute insert query.
                            query.executeStep( );
                            // Reset SQL insert query.
                            query.reset( );
                        }
                        oldChampion = newChampion;
                    }
                    // std::cout << "      Done!" << std::endl;   
                }
            }
            // std::cout << "    F = " << f_variable << " done!" << std::endl;
        }
        // std::cout << "Population " <<  populationSize_DE << " done!" << std::endl;
    }

    return;
}


void writeShortlist( SQLite::Database&      database, 
                     const int              numberOfLegs, 
                     const DateTime         initialEpoch,
                     std::vector< Tle >     tleObjects,
                     const std::string      shortlistPath )
{
// for (int j = 0; j < numberOfLegs; ++j)
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
//     }
}
//! Check pagmo_scanner input parameters.
PagmoScannerInput checkPagmoScannerInput( const rapidjson::Document& config )
{
    const std::string catalogPath = find( config, "catalog" )->value.GetString( );
    std::cout << "Catalog                       " << catalogPath << std::endl;

    const std::string databasePath = find( config, "database" )->value.GetString( );
    std::cout << "Database                      " << databasePath << std::endl;

    const std::string satcatPath = find( config, "satcat_path" )->value.GetString( );
    std::cout << "Satcat                      " << satcatPath << std::endl;

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

    const int strategy
        = find( config, "strategy" )->value.GetInt( );
    std::cout << "strategy                      " << strategy << std::endl;

    const int numberOfRuns
        = find( config, "number_of_runs" )->value.GetInt( );
    std::cout << "Number of runs                " << numberOfRuns << std::endl;
    
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
                                satcatPath,
                                initialEpoch,
                                numberOfLegs,
                                strategy,
                                numberOfRuns,
                                departureEpochUpperBound,
                                timeOfFlightUpperBound,
                                stayTime);
}

//! Create pagmo_scanner table.
void createPagmoScannerTable( SQLite::Database& database, int numberOfLegs )
{
    std::cout << "Creating Pagmo database...." << std::endl;
    // Drop table from database if it exists.
    // database.exec( "DROP TABLE IF EXISTS pagmo_scanner_results;" );
    
    // // Set up SQL command to create table to store pagmo_scanner results.
    // std::ostringstream pagmoScannerTableCreate;
    // pagmoScannerTableCreate
    //     << "CREATE TABLE pagmo_scanner_results ("
    //     << "\"transfer_id\"                             INTEGER PRIMARY KEY AUTOINCREMENT,"
    //         // << "\"departure_object_id\"                     TEXT,"
    //         // << "\"arrival_object_id\"                       TEXT,"
    //         // << "\"departure_epoch\"                         REAL,"
    //         // << "\"time_of_flight\"                          REAL,"
    //     << "\"algorithm\"                               TEXT,"
    //     << "\"run_number\"                              INTEGER,"
    //     << "\"number_of_generations\"                   INTEGER,"
    //     << "\"generation\"                              INTEGER,"
    //     << "\"population_size\"                         INTEGER,"
    //     << "\"f_variable\"                              REAL,"
    //     << "\"cr_variable\"                             REAL,"
    //     << "\"strategy\"                                INTEGER,"
    //     << "\"transfer_delta_v\"                        REAL"
    //     <<                                              ");";


    // // Execute command to create table.
    // database.exec( pagmoScannerTableCreate.str( ).c_str( ) );

    // // Execute command to create index on transfer Delta-V column.
    // std::ostringstream transferDeltaVIndexCreate;
    // transferDeltaVIndexCreate << "CREATE INDEX IF NOT EXISTS \"transfer_delta_v\" on "
    //                           << "pagmo_scanner_results (transfer_delta_v ASC);";
    // database.exec( transferDeltaVIndexCreate.str( ).c_str( ) );

    //     // throw std::runtime_error( "ERROR: Creating table 'pagmo_scanner_results' failed!" );
    
    
    // if ( !database.tableExists( "pagmo_scanner_results" ) )
    // {
    //     std::cout << "Table exists, results will be appended." << std::endl;
    // }
    


    std::ostringstream pagmoVectorTableCheck;
    pagmoVectorTableCheck << "DROP TABLE IF EXISTS pagmo_scanner_results_" << numberOfLegs <<";";

    database.exec( pagmoVectorTableCheck.str( ) );
    
    // Set up SQL command to create table to store pagmo_scanner results.
    std::ostringstream pagmoVectorTableCreate;
    pagmoVectorTableCreate
        << "CREATE TABLE pagmo_scanner_results_"
        << numberOfLegs
        << "("
        << "\"sequence_id\"                             INTEGER PRIMARY KEY AUTOINCREMENT,"
        << "\"algorithm\"                               TEXT,"
        << "\"run_number\"                              INTEGER,"
        << "\"number_of_generations\"                   INTEGER,"
        << "\"generation\"                              INTEGER,"
        << "\"population_size\"                         INTEGER,"
        << "\"f_variable\"                              REAL,"
        << "\"cr_variable\"                             REAL,"
        << "\"strategy\"                                INTEGER,";

    for (int i = 1; i < numberOfLegs + 2; ++i)
        {
            pagmoVectorTableCreate << "\"object_" << i << "\"        INTEGER,";
        }   
    for (int i = 1; i < numberOfLegs + 2; ++i)
        {
            pagmoVectorTableCreate << "\"object_" << i << "_area\"        REAL,";
        }    
    for (int i = 1; i < numberOfLegs + 1; ++i)
        {
            pagmoVectorTableCreate << "\"departure_epoch_" << i << "\"        REAL,"
                                   << "\"time_of_flight_"  << i << "\"        REAL,";
        }    
    pagmoVectorTableCreate
        << "\"removed_area\"                         REAL,"
        << "\"total_delta_v\"                        REAL"
        <<                                              ");";


    // Execute command to create table.
    database.exec( pagmoVectorTableCreate.str( ).c_str( ) );

    
    std::cout << "Pagmo database created!" << std::endl;

}


} // namespace d2d
