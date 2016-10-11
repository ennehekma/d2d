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
<<<<<<< HEAD
    // pagmo::problem::prob_A prob(5);
    // pagmo::population pop(prob,50);
    // pagmo::algorithm::jde algo(500);
=======
    // Verify config parameters. Exception is thrown if any of the parameters are missing.
    const LambertScannerInput input = checkLambertScannerInput( config );

    // Set gravitational parameter used by Lambert targeter.
    const double earthGravitationalParameter = kMU;
    std::cout << "Earth gravitational parameter " << earthGravitationalParameter
              << " kg m^3 s^-2" << std::endl;

    std::cout << std::endl;
    std::cout << "******************************************************************" << std::endl;
    std::cout << "                       Simulation & Output                        " << std::endl;
    std::cout << "******************************************************************" << std::endl;
    std::cout << std::endl;

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

    // Open database in read/write mode.
    SQLite::Database database( input.databasePath.c_str( ),
                               SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE );

    // Create table for Lambert scanner results in SQLite database.
    std::cout << "Creating SQLite database table if needed ... " << std::endl;
    createLambertScannerTable( database );
    std::cout << "SQLite database set up successfully!" << std::endl;

    // Start SQL transaction.
    SQLite::Transaction transaction( database );

    // Setup insert query.
    std::ostringstream lambertScannerTableInsert;
    lambertScannerTableInsert
        << "INSERT INTO lambert_scanner_results VALUES ("
        << "NULL,"
        << ":departure_object_id,"
        << ":arrival_object_id,"
        << ":departure_epoch,"
        << ":time_of_flight,"
        << ":revolutions,"
        << ":prograde,"
        << ":departure_position_x,"
        << ":departure_position_y,"
        << ":departure_position_z,"
        << ":departure_velocity_x,"
        << ":departure_velocity_y,"
        << ":departure_velocity_z,"
        << ":departure_semi_major_axis,"
        << ":departure_eccentricity,"
        << ":departure_inclination,"
        << ":departure_argument_of_periapsis,"
        << ":departure_longitude_of_ascending_node,"
        << ":departure_true_anomaly,"
        << ":arrival_position_x,"
        << ":arrival_position_y,"
        << ":arrival_position_z,"
        << ":arrival_velocity_x,"
        << ":arrival_velocity_y,"
        << ":arrival_velocity_z,"
        << ":arrival_semi_major_axis,"
        << ":arrival_eccentricity,"
        << ":arrival_inclination,"
        << ":arrival_argument_of_periapsis,"
        << ":arrival_longitude_of_ascending_node,"
        << ":arrival_true_anomaly,"
        << ":transfer_semi_major_axis,"
        << ":transfer_eccentricity,"
        << ":transfer_inclination,"
        << ":transfer_argument_of_periapsis,"
        << ":transfer_longitude_of_ascending_node,"
        << ":transfer_true_anomaly,"
        << ":departure_delta_v_x,"
        << ":departure_delta_v_y,"
        << ":departure_delta_v_z,"
        << ":arrival_delta_v_x,"
        << ":arrival_delta_v_y,"
        << ":arrival_delta_v_z,"
        << ":transfer_delta_v"
        << ");";

    SQLite::Statement query( database, lambertScannerTableInsert.str( ) );

    std::cout << "Computing Lambert transfers and populating database ... " << std::endl;

    // Loop over TLE objects and compute transfers based on Lambert targeter across time-of-flight
    // grid.
    boost::progress_display showProgress( tleObjects.size( ) );

    // Loop over all departure objects.
    for ( unsigned int i = 0; i < tleObjects.size( ); i++ )
    {
        // Compute departure state.
        Tle departureObject = tleObjects[ i ];
        SGP4 sgp4Departure( departureObject );

        DateTime departureEpoch = input.departureEpochInitial;
        if ( input.departureEpochInitial == DateTime( ) )
        {
            departureEpoch = departureObject.Epoch( );
        }

        // Loop over arrival objects.
        for ( unsigned int j = 0; j < tleObjects.size( ); j++ )
        {
            // Skip the case of the departure and arrival objects being the same.
            if ( i == j )
            {
                continue;
            }

            Tle arrivalObject = tleObjects[ j ];
            SGP4 sgp4Arrival( arrivalObject );
            const int arrivalObjectId = static_cast< int >( arrivalObject.NoradNumber( ) );

            // Loop over departure epoch grid.
            for ( int m = 0; m < input.departureEpochSteps; ++m )
            {
                DateTime departureEpoch = input.departureEpochInitial;
                departureEpoch = departureEpoch.AddSeconds( input.departureEpochStepSize * m );

                const Eci tleDepartureState = sgp4Departure.FindPosition( departureEpoch );
                const Vector6 departureState = getStateVector( tleDepartureState );

                Vector3 departurePosition;
                std::copy( departureState.begin( ),
                           departureState.begin( ) + 3,
                           departurePosition.begin( ) );

                Vector3 departureVelocity;
                std::copy( departureState.begin( ) + 3,
                           departureState.end( ),
                           departureVelocity.begin( ) );

                const Vector6 departureStateKepler
                    = astro::convertCartesianToKeplerianElements( departureState,
                                                                  earthGravitationalParameter );
                const int departureObjectId = static_cast< int >( departureObject.NoradNumber( ) );

                // Loop over time-of-flight grid.
                for ( int k = 0; k < input.timeOfFlightSteps; k++ )
                {
                    const double timeOfFlight
                        = input.timeOfFlightMinimum + k * input.timeOfFlightStepSize;

                    const DateTime arrivalEpoch = departureEpoch.AddSeconds( timeOfFlight );
                    const Eci tleArrivalState   = sgp4Arrival.FindPosition( arrivalEpoch );
                    const Vector6 arrivalState  = getStateVector( tleArrivalState );

                    Vector3 arrivalPosition;
                    std::copy( arrivalState.begin( ),
                               arrivalState.begin( ) + 3,
                               arrivalPosition.begin( ) );

                    Vector3 arrivalVelocity;
                    std::copy( arrivalState.begin( ) + 3,
                               arrivalState.end( ),
                               arrivalVelocity.begin( ) );
                    const Vector6 arrivalStateKepler
                        = astro::convertCartesianToKeplerianElements( arrivalState,
                                                                      earthGravitationalParameter );

                    kep_toolbox::lambert_problem targeter( departurePosition,
                                                           arrivalPosition,
                                                           timeOfFlight,
                                                           earthGravitationalParameter,
                                                           !input.isPrograde,
                                                           input.revolutionsMaximum );

                    const int numberOfSolutions = targeter.get_v1( ).size( );

                    // Compute Delta-Vs for transfer and determine index of lowest.
                    typedef std::vector< Vector3 > VelocityList;
                    VelocityList departureDeltaVs( numberOfSolutions );
                    VelocityList arrivalDeltaVs( numberOfSolutions );

                    typedef std::vector< double > TransferDeltaVList;
                    TransferDeltaVList transferDeltaVs( numberOfSolutions );

                    for ( int i = 0; i < numberOfSolutions; i++ )
                    {
                        // Compute Delta-V for transfer.
                        const Vector3 transferDepartureVelocity = targeter.get_v1( )[ i ];
                        const Vector3 transferArrivalVelocity = targeter.get_v2( )[ i ];

                        departureDeltaVs[ i ] = sml::add( transferDepartureVelocity,
                                                          sml::multiply( departureVelocity, -1.0 ) );
                        arrivalDeltaVs[ i ]   = sml::add( arrivalVelocity,
                                                          sml::multiply( transferArrivalVelocity, -1.0 ) );

                        transferDeltaVs[ i ]
                            = sml::norm< double >( departureDeltaVs[ i ] )
                                + sml::norm< double >( arrivalDeltaVs[ i ] );
                    }

                    const TransferDeltaVList::iterator minimumDeltaVIterator
                        = std::min_element( transferDeltaVs.begin( ), transferDeltaVs.end( ) );
                    const int minimumDeltaVIndex
                        = std::distance( transferDeltaVs.begin( ), minimumDeltaVIterator );

                    const int revolutions = std::floor( ( minimumDeltaVIndex + 1 ) / 2 );

                    Vector6 transferState;
                    std::copy( departurePosition.begin( ),
                               departurePosition.begin( ) + 3,
                               transferState.begin( ) );
                    std::copy( targeter.get_v1( )[ minimumDeltaVIndex ].begin( ),
                               targeter.get_v1( )[ minimumDeltaVIndex ].begin( ) + 3,
                               transferState.begin( ) + 3 );

                    const Vector6 transferStateKepler
                        = astro::convertCartesianToKeplerianElements( transferState,
                                                                      earthGravitationalParameter );
>>>>>>> 20d424b... Add missing include statements for SQLite library to access macros (due to change in SQLiteCpp).



<<<<<<< HEAD
    // algo.set_screen_output(true);
    // algo.evolve(pop);
    std::cout << "Done .. the champion is ... " <<std::endl;
    // std::cout << pop.champion().x << std::endl;
    return;
=======
                    // Reset SQL insert query.
                    query.reset( );
                }
            }
        }

        ++showProgress;
    }

    // Commit transaction.
    transaction.commit( );

    std::cout << std::endl;
    std::cout << "Database populated successfully!" << std::endl;
    std::cout << std::endl;

    // Check if shortlist file should be created; call function to write output.
    if ( input.shortlistLength > 0 )
    {
        std::cout << "Writing shortlist to file ... " << std::endl;
        writeTransferShortlist( database, input.shortlistLength, input.shortlistPath );
        std::cout << "Shortlist file created successfully!" << std::endl;
    }
>>>>>>> 20d424b... Add missing include statements for SQLite library to access macros (due to change in SQLiteCpp).
}
} // namespace d2d
