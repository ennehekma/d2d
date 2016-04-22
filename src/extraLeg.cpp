/*
 * Copyright (c) 2014-2016 Kartik Kumar (me@kartikkumar.com)
 * Copyright (c) 2016 Enne Hekma (ennehekma@gmail.com)
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

#include <Atom/atom.hpp>

#include <Astro/astro.hpp>

#include "D2D/extraLeg.hpp"
#include "D2D/tools.hpp"
#include "D2D/typedefs.hpp"

namespace d2d
{
// typedef double Real;
// typedef std::pair< Vector3, Vector3 > Velocities;

//! Execute atom_scanner.
void executeExtraLeg( const rapidjson::Document& config )
{

    std::cout.precision( 15 );

    // Verify config parameters. Exception is thrown if any of the parameters are missing.
    const ExtraLegInput input = checkExtraLegInput( config );

    // // Set gravitational parameter used by Atom targeter.
    // const double earthGravitationalParameter = kMU;
    // std::cout << "Earth gravitational parameter " << earthGravitationalParameter
    //           << " kg m^3 s^-2" << std::endl;

    // // Set mean radius used [km].
    // const double earthMeanRadius = kXKMPER;
    // std::cout << "Earth mean radius               " << earthMeanRadius << " km" << std::endl;

    std::cout << std::endl;
    std::cout << "******************************************************************" << std::endl;
    std::cout << "                       Simulation & Output                        " << std::endl;
    std::cout << "******************************************************************" << std::endl;
    std::cout << std::endl;


    // Open database in read/write mode.
    // N.B.: Database must already exist and contain two populated table called
    //       "lambert_scanner_results" and "sgp4_scanner_results".
    SQLite::Database database( input.databasePath.c_str( ), SQLITE_OPEN_READWRITE );

    // Create table for leg_x_results in SQLite database.
    // std::cout << "Creating SQLite database table if needed ... " << std::endl;
    // createExtraLegTable( database );
    // std::cout << "SQLite database set up successfully!" << std::endl;

    // Start SQL transaction.

    // Fetch number of rows in spg4_scanner_results table.
    std::ostringstream sgp4ScannerTableSizeSelect;
    sgp4ScannerTableSizeSelect << "SELECT COUNT ( DISTINCT arrival_object_id ) "                  \
                               << "FROM lambert_scanner_results;";

    const int atomScannerTableSize
        = database.execAndGet( sgp4ScannerTableSizeSelect.str( ) );
    std::cout << "Amount of objects in database: " << atomScannerTableSize << std::endl;
    int legToProcess = 1;
    for (int i = 1; i < atomScannerTableSize; ++i)
    {
        std::ostringstream checkTables;
        checkTables << "leg_" << i << "_results" ;
        bool a = database.tableExists( checkTables.str( ) );
        legToProcess = i;
        if (a==0)
        {
            break;
        }
    }
    std::cout << "Leg to process: " << legToProcess << std::endl;
    std::cout << "Leg to process teswt: " << !database.tableExists( "leg_1_results" )  << std::endl;


    createExtraLegTable( database, legToProcess );
    // database.exec( "DROP TABLE IF EXISTS leg_1_results;" );
    SQLite::Transaction transaction( database );


    std::ostringstream atomScannerTableInsert;
    atomScannerTableInsert << "INSERT INTO leg_" << legToProcess << "_results VALUES ("
                           << "NULL,"
                           << ":lambert_transfer_id,"
                           << ":atom_departure_delta_v_x,"
                           << ":atom_departure_delta_v_y,"
                           << ":atom_departure_delta_v_z,"
                           << ":atom_arrival_delta_v_x,"
                           << ":atom_arrival_delta_v_y,"
                           << ":atom_arrival_delta_v_z,"
                           << ":atom_transfer_delta_v"
                           << ");";
    // std::cout << "Database create ated: " << std::endl;
    SQLite::Statement atomQuery( database, atomScannerTableInsert.str( ) );
    int lambertTransferId = 37;
    atomQuery.bind( ":lambert_transfer_id",              lambertTransferId );
    atomQuery.executeStep( );
    atomQuery.reset( );
    transaction.commit( );


    std::ostringstream atomTopList;
    atomTopList << "SELECT *, min(atom_scanner_results.atom_transfer_delta_v ) "
                << "FROM        lambert_scanner_results "
                << "INNER JOIN  atom_scanner_results "
                << "ON          lambert_scanner_results.transfer_id "
                << "          = atom_scanner_results.lambert_transfer_id "
                << "GROUP BY    lambert_scanner_results.departure_object_id, "
                << "            lambert_scanner_results.arrival_object_id "
                << "ORDER BY    atom_scanner_results.atom_transfer_delta_v "
                << "ASC ;";
    SQLite::Statement atomTopQuery( database, atomTopList.str( ) );

    int departureObjectIds [5] = { };
    double departureEpochStarts [5] = { };
    int i = 0;
    while (atomTopQuery.executeStep() && i<10)
    {
        const int departureObjectId     = atomTopQuery.getColumn(1);
        const int arrivalObjectId       = atomTopQuery.getColumn(2);
        const double departureEpoch     = atomTopQuery.getColumn(3);
        const double timeOfFlight       = atomTopQuery.getColumn(4);

        departureObjectIds [i]          = arrivalObjectId;
        departureEpochStarts [i]        = departureEpoch + timeOfFlight/86400;
        i++;
    }
    std::cout << departureEpochStarts[0] << std::endl;


}

// ! Check atom_scanner input parameters.
ExtraLegInput checkExtraLegInput( const rapidjson::Document& config )
{
    const std::string databasePath = find( config, "database" )->value.GetString( );
    std::cout << "Database                      " << databasePath << std::endl;

    const double relativeTolerance = find( config, "relative_tolerance" )->value.GetDouble( );
    std::cout << "Relative tolerance            " << relativeTolerance << std::endl;

    const double absoluteTolerance = find( config, "absolute_tolerance" )->value.GetDouble( );
    std::cout << "Absolute tolerance            " << absoluteTolerance << std::endl;

    const int maxIterations = find( config, "maximum_iterations" )->value.GetInt( );
    // std::cout << "Maximum iterations Atom solver" << maxIterations << std::endl;

    const int shortlistLength = find( config, "shortlist" )->value[ 0 ].GetInt( );
    std::cout << "# of shortlist transfers      " << shortlistLength << std::endl;

    std::string shortlistPath = "";
    if ( shortlistLength > 0 )
    {
        shortlistPath = find( config, "shortlist" )->value[ 1 ].GetString( );
        std::cout << "Shortlist                       " << shortlistPath << std::endl;
    }

    return ExtraLegInput( relativeTolerance,
                             absoluteTolerance,
                             databasePath,
                             maxIterations,
                             shortlistLength,
                             shortlistPath );
}

//! Create atom_scanner table.
void createExtraLegTable( SQLite::Database& database, int legToProcess )
{
    // Drop table from database if it exists.
    std::ostringstream checkExistingTable;
    checkExistingTable << "DROP TABLE IF EXISTS leg_" << legToProcess << "_results;" ;
    database.exec( checkExistingTable.str().c_str() );

    // Set up SQL command to create table to store atom_scanner results.
    std::ostringstream extraLegTableCreate;
    extraLegTableCreate
        << "CREATE TABLE leg_" << legToProcess << "_results ("
        << "\"transfer_id\"                                  INTEGER PRIMARY KEY AUTOINCREMENT,"
        << "\"lambert_transfer_id\"                          INT,"
        << "\"atom_departure_delta_v_x\"                     REAL,"
        << "\"atom_departure_delta_v_y\"                     REAL,"
        << "\"atom_departure_delta_v_z\"                     REAL,"
        << "\"atom_arrival_delta_v_x\"                       REAL,"
        << "\"atom_arrival_delta_v_y\"                       REAL,"
        << "\"atom_arrival_delta_v_z\"                       REAL,"
        << "\"atom_transfer_delta_v\"                        REAL"
        <<                                              ");";
    // Execute command to create table.
    database.exec( extraLegTableCreate.str( ).c_str( ) );

    // Check if new table is correctly created.
    std::ostringstream newTable;
    newTable << "leg_" << legToProcess << "_results" ;
    if ( !database.tableExists( newTable.str().c_str() ) )
    {
        throw std::runtime_error( "ERROR: Creating table 'leg_x_results' failed!" );
    }
}

//! Write transfer shortlist to file.
// void writeAtomTransferShortlist( SQLite::Database& database,
//                              const int shortlistNumber,
//                              const std::string& shortlistPath )
// {
//     // Fetch transfers to include in shortlist.
//     // Database table is sorted by transfer_delta_v, in ascending order.
//     std::ostringstream shortlistSelect;
//     shortlistSelect << "SELECT * "
//                     << "FROM leg_x_results "
//                     << "ORDER BY atom_transfer_delta_v ASC "
//                     << "LIMIT "
//                     << shortlistNumber << ";";
//     SQLite::Statement query( database, shortlistSelect.str( ) );

//     // Write fetch data to file.
//     std::ofstream shortlistFile( shortlistPath.c_str( ) );

//     // Print file header.
//     shortlistFile << "transfer_id,"
//                   << "lambert_transfer_id,"
//                   << "atom_departure_delta_v_x,"
//                   << "atom_departure_delta_v_y,"
//                   << "atom_departure_delta_v_z,"
//                   << "atom_arrival_delta_v_x,"
//                   << "atom_arrival_delta_v_y,"
//                   << "atom_arrival_delta_v_z,"
//                   << "atom_transfer_delta_v"
//                   << std::endl;

//     // Loop through data retrieved from database and write to file.
//     while( query.executeStep( ) )
//     {
//         const int    atomTransferId                   = query.getColumn( 0 );
//         const int    lambertTransferId                = query.getColumn( 1 );
//         const double atomDepartureDeltaVX               = query.getColumn( 2 );
//         const double atomDepartureDeltaVY               = query.getColumn( 3 );
//         const double atomDepartureDeltaVZ               = query.getColumn( 4 );
//         const double atomArrivalDeltaVX                 = query.getColumn( 5 );
//         const double atomArrivalDeltaVY                 = query.getColumn( 6 );
//         const double atomArrivalDeltaVZ                 = query.getColumn( 7 );
//         const double atomTransferDeltaV                 = query.getColumn( 8 );

//         shortlistFile << atomTransferId           << ","
//                       << lambertTransferId        << ","
//                       << atomDepartureDeltaVX       << ","
//                       << atomDepartureDeltaVY       << ","
//                       << atomDepartureDeltaVZ       << ","
//                       << atomArrivalDeltaVX         << ","
//                       << atomArrivalDeltaVY         << ","
//                       << atomArrivalDeltaVZ         << ","
//                       << atomTransferDeltaV
//                       << std::endl;
//     }

//     shortlistFile.close( );
// }

} // namespace d2d
