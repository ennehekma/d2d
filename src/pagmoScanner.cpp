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

    const double departureEpochUpperBound = 14*86400;
    const double timeOfFlightUpperBound = 2*86400;
    const DateTime initialEpoch  =  DateTime( 2016,1,12,12,0,0 );
    // std::ofstream myfile;
    // myfile.open ("/home/enne/work/d2d/data/pagmo/example.csv");
    // myfile << "id,depid,arrid,run,gen,deltav" << std::endl;

    createPagmoScannerTable( database );
    std::map< std::pair< int, int >, double >  overallBest;
    std::vector<std::pair< int, int> > enne;
    enne.push_back(std::make_pair<int,int>(733, 815));
    enne.push_back(std::make_pair<int,int>(733, 39203));
    enne.push_back(std::make_pair<int,int>(815, 39203));
    enne.push_back(std::make_pair<int,int>(16615,   21689));
    enne.push_back(std::make_pair<int,int>(16615,   32063));
    enne.push_back(std::make_pair<int,int>(16615,   37390));
    enne.push_back(std::make_pair<int,int>(18961,   22830));
    enne.push_back(std::make_pair<int,int>(18961,   23561));
    enne.push_back(std::make_pair<int,int>(18961,   25732));
    enne.push_back(std::make_pair<int,int>(19468,   27432));
    enne.push_back(std::make_pair<int,int>(20443,   36089));
    enne.push_back(std::make_pair<int,int>(21610,   37215));
    enne.push_back(std::make_pair<int,int>(21689,   16615));
    enne.push_back(std::make_pair<int,int>(21689,   32063));
    enne.push_back(std::make_pair<int,int>(21689,   37215));
    enne.push_back(std::make_pair<int,int>(21689,   37390));
    enne.push_back(std::make_pair<int,int>(22830,   18961));
    enne.push_back(std::make_pair<int,int>(22830,   25732));
    enne.push_back(std::make_pair<int,int>(23324,   23753));
    enne.push_back(std::make_pair<int,int>(23324,   25400));
    enne.push_back(std::make_pair<int,int>(23324,   27387));
    enne.push_back(std::make_pair<int,int>(23324,   27432));
    enne.push_back(std::make_pair<int,int>(23324,   28050));
    enne.push_back(std::make_pair<int,int>(23324,   37932));
    enne.push_back(std::make_pair<int,int>(23561,   19468));
    enne.push_back(std::make_pair<int,int>(23561,   25732));
    enne.push_back(std::make_pair<int,int>(23753,   23324));
    enne.push_back(std::make_pair<int,int>(23753,   25400));
    enne.push_back(std::make_pair<int,int>(23753,   27387));
    enne.push_back(std::make_pair<int,int>(23753,   27432));
    enne.push_back(std::make_pair<int,int>(23753,   28050));
    enne.push_back(std::make_pair<int,int>(23753,   37932));
    enne.push_back(std::make_pair<int,int>(23828,   25942));
    enne.push_back(std::make_pair<int,int>(25261,   23828));
    enne.push_back(std::make_pair<int,int>(25261,   25942));
    enne.push_back(std::make_pair<int,int>(25261,   27387));
    enne.push_back(std::make_pair<int,int>(25400,   23324));
    enne.push_back(std::make_pair<int,int>(25400,   23753));
    enne.push_back(std::make_pair<int,int>(25400,   27387));
    enne.push_back(std::make_pair<int,int>(25400,   27432));
    enne.push_back(std::make_pair<int,int>(25400,   28050));
    enne.push_back(std::make_pair<int,int>(25400,   37932));
    enne.push_back(std::make_pair<int,int>(25732,   18961));
    enne.push_back(std::make_pair<int,int>(25732,   22830));
    enne.push_back(std::make_pair<int,int>(25732,   23561));
    enne.push_back(std::make_pair<int,int>(25942,   23828));
    enne.push_back(std::make_pair<int,int>(25942,   25261));
    enne.push_back(std::make_pair<int,int>(27387,   23753));
    enne.push_back(std::make_pair<int,int>(27387,   23753));
    enne.push_back(std::make_pair<int,int>(27387,   25261));
    enne.push_back(std::make_pair<int,int>(27387,   25400));
    enne.push_back(std::make_pair<int,int>(27387,   28050));
    enne.push_back(std::make_pair<int,int>(27387,   37932));
    enne.push_back(std::make_pair<int,int>(27432,   23324));
    enne.push_back(std::make_pair<int,int>(27432,   23753));
    enne.push_back(std::make_pair<int,int>(27432,   25400));
    enne.push_back(std::make_pair<int,int>(27432,   37932));
    enne.push_back(std::make_pair<int,int>(28050,   23324));
    enne.push_back(std::make_pair<int,int>(28050,   23753));
    enne.push_back(std::make_pair<int,int>(28050,   25261));
    enne.push_back(std::make_pair<int,int>(28050,   25400));
    enne.push_back(std::make_pair<int,int>(28050,   27387));
    enne.push_back(std::make_pair<int,int>(28050,   37932));
    enne.push_back(std::make_pair<int,int>(28059,   32959));
    enne.push_back(std::make_pair<int,int>(28059,   39093));
    enne.push_back(std::make_pair<int,int>(31123,   733));
    enne.push_back(std::make_pair<int,int>(32063,   16615));
    enne.push_back(std::make_pair<int,int>(32063,   21689));
    enne.push_back(std::make_pair<int,int>(32063,   37215));
    enne.push_back(std::make_pair<int,int>(32063,   37390));
    enne.push_back(std::make_pair<int,int>(32959,   28059));
    enne.push_back(std::make_pair<int,int>(32959,   39093));
    enne.push_back(std::make_pair<int,int>(36089,   20443));
    enne.push_back(std::make_pair<int,int>(37215,   21610));
    enne.push_back(std::make_pair<int,int>(37215,   21689));
    enne.push_back(std::make_pair<int,int>(37390,   16615));
    enne.push_back(std::make_pair<int,int>(37390,   21689));
    enne.push_back(std::make_pair<int,int>(37390,   32063));
    enne.push_back(std::make_pair<int,int>(37932,   23324));
    enne.push_back(std::make_pair<int,int>(37932,   23753));
    enne.push_back(std::make_pair<int,int>(37932,   25400));
    enne.push_back(std::make_pair<int,int>(37932,   27387));
    enne.push_back(std::make_pair<int,int>(37932,   27432));
    enne.push_back(std::make_pair<int,int>(37932,   28050));
    enne.push_back(std::make_pair<int,int>(39093,   28059));
    enne.push_back(std::make_pair<int,int>(39093,   32959));
    enne.push_back(std::make_pair<int,int>(39203,   733));
    enne.push_back(std::make_pair<int,int>(39203,   815));
    

    std::ostringstream pagmoScannerTableInsert;
    pagmoScannerTableInsert
        << "INSERT INTO pagmo_scanner_results VALUES ("
        << "NULL,"
        << ":departure_object_id,"
        << ":arrival_object_id,"
        << ":departure_epoch,"
        << ":time_of_flight,"
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

            for (int k = 0; k < enne.size(); ++k)
            {
                if (departureObject.NoradNumber()==enne[k].first and arrivalObject.NoradNumber() == enne[k].second)
                {
                    const int departureObjectId = static_cast< int >( departureObject.NoradNumber( ) );
                    const int arrivalObjectId = static_cast< int >( arrivalObject.NoradNumber( ) );
                    std::pair<int,int> combo = std::make_pair(departureObjectId,arrivalObjectId);
                    std::cout << departureObjectId << " " << arrivalObjectId << std::endl;
                    
                    pagmo::problem::thesis thesis(  2,
                                                    departureObject,
                                                    arrivalObject,
                                                    initialEpoch,
                                                    departureEpochUpperBound,
                                                    timeOfFlightUpperBound);

                    
                    
                    for (int size_multiplier = 1; size_multiplier < 8; ++size_multiplier)
                    {
                    
                        int popSizeDE = 10*size_multiplier;
                        
                        int runs = 10;
                        double f_variable = 0.8;
                        double cr_variable = 0.9;
                        int numberOfGenDE = 500;
                        for (int run_number = 1; run_number < runs; ++run_number)
                        {
                            pagmo::population populationDE( thesis, popSizeDE );
                            pagmo::algorithm::jde algorithmDE( 1 );
                                                        
                            query.bind( ":departure_object_id" , departureObjectId );
                            query.bind( ":arrival_object_id" , arrivalObjectId );
                            query.bind( ":departure_epoch" ,  populationDE.champion().x[0] );
                            query.bind( ":time_of_flight" , populationDE.champion().x[1] );
                            query.bind( ":algorithm" , algorithmDE.get_name() );
                            query.bind( ":run_number" , run_number );
                            query.bind( ":number_of_generations" , numberOfGenDE );
                            query.bind( ":generation" , 0 );
                            query.bind( ":population_size" , popSizeDE );
                            query.bind( ":f_variable" , f_variable );
                            query.bind( ":cr_variable" , cr_variable );
                            query.bind( ":transfer_delta_v" , populationDE.champion().f[0] );
                            // Execute insert query.
                            query.executeStep( );
                            // Reset SQL insert query.
                            query.reset( );                  

                            double oldchamp = 1.0;
                            double newchamp = 1.0;
                            for( int i = 1; i < numberOfGenDE; ++i )
                            {
                                // Proceed with next round of optimising
                                algorithmDE.evolve( populationDE );
                                newchamp = populationDE.champion().f[0];
                                if (newchamp  < oldchamp )
                                {
                                    query.bind( ":departure_object_id" , departureObjectId );
                                    query.bind( ":arrival_object_id" , arrivalObjectId );
                                    query.bind( ":departure_epoch" ,  populationDE.champion().x[0] );
                                    query.bind( ":time_of_flight" , populationDE.champion().x[1] );
                                    query.bind( ":algorithm" , algorithmDE.get_name() );
                                    query.bind( ":run_number" , run_number );
                                    query.bind( ":number_of_generations" , numberOfGenDE );
                                    query.bind( ":generation" , i );
                                    query.bind( ":population_size" , popSizeDE );
                                    query.bind( ":f_variable" , f_variable );
                                    query.bind( ":cr_variable" , cr_variable );
                                    query.bind( ":transfer_delta_v" , populationDE.champion().f[0] );
                                    // Execute insert query.
                                    query.executeStep( );
                                    // Reset SQL insert query.
                                    query.reset( );
                                }
                                oldchamp = newchamp;
                            }
                        }
                    }
                }
            }
            counter++;
        }

    }
    // myfile.close();
    std::cout << counter << std::endl;
    return;
}

//! Check pagmo_scanner input parameters.
PagmoScannerInput checkPagmoScannerInput( const rapidjson::Document& config )
{
    const std::string catalogPath = find( config, "catalog" )->value.GetString( );
    std::cout << "Catalog                       " << catalogPath << std::endl;

    const std::string databasePath = find( config, "database" )->value.GetString( );
    std::cout << "Database                      " << databasePath << std::endl;

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
                                databasePath,
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
        << "\"departure_object_id\"                     TEXT,"
        << "\"arrival_object_id\"                       TEXT,"
        << "\"departure_epoch\"                         REAL,"
        << "\"time_of_flight\"                          REAL,"
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
