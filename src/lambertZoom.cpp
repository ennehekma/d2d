/*
 * Copyright (c) 2014-2016 Kartik Kumar, Dinamica Srl (me@kartikkumar.com)
 * Copyright (c) 2016 Enne Hekma, Delft University of Technology (ennehekma@gmail)
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
#include <boost/tuple/tuple.hpp>

#include <libsgp4/Eci.h>
#include <libsgp4/Globals.h>
#include <libsgp4/SGP4.h>
#include <libsgp4/Tle.h>

#include <sqlite3.h>

#include <omp.h>

#include <SML/sml.hpp>
#include <Astro/astro.hpp>

#include "D2D/lambertZoom.hpp"
#include "D2D/tools.hpp"

namespace d2d

{

bool sortByDV(const boost::tuple<double, double, double> &a,
              const boost::tuple<double, double, double> &b)
{
    return a.get<2>() < b.get<2>();
}

bool sortByTime(const boost::tuple<double, double, double> &a,
                const boost::tuple<double, double, double> &b)
{
    if (a.get<0>() == b.get<0>())
    {
        return a.get<1>() < b.get<1>();
    }
    return a.get<0>() < b.get<0>();
}

bool gridPointComparison(const boost::tuple<double, double, double> &a,
                         const boost::tuple<double, double, double> &b)
{
    if ((a.get<0>() == b.get<0>()) && (a.get<1>() == b.get<1>()))
    {
        return true;
    }
    return false;
}


//! Execute lambert_scanner.
void executeLambertZoom( const rapidjson::Document& config )
{
    // Verify config parameters. Exception is thrown if any of the parameters are missing.
    const LambertZoomInput input = checkLambertZoomInput( config );
    time_t now = time(0);
    tm* localtm = localtime(&now);
    std::cout << "The local date and time is: " << asctime(localtm) << std::endl;
    // Set gravitational parameter used by Lambert targeter.
    const double earthGravitationalParameter = kMU;
    std::cout << "Earth gravitational parameter " << earthGravitationalParameter
              << " kg m^3 s^-2" << std::endl;

    std::cout << std::endl;
    std::cout << "******************************************************************" << std::endl;
    std::cout << "                       Simulation & Output                        " << std::endl;
    std::cout << "******************************************************************" << std::endl;
    std::cout << std::endl;

    double percentage = input.topPoints / (input.departureEpochSteps * input.timeOfFlightSteps) * 100;

    if (percentage < 1)
    {
        std::cout
                << "Percentage of initial grid points that is used for next generation is small. Only "
                << percentage
                << " percent is used."
                << std::endl;
    }

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
    createLambertZoomTable( database );
    std::cout << "SQLite database set up successfully!" << std::endl;

    // Start SQL transaction.
    SQLite::Transaction transaction( database );

    // Setup insert query.
    std::ostringstream lambertScannerTableInsert;
    lambertScannerTableInsert
        << "INSERT INTO lambert_scanner_zoom_results VALUES ("
        << "NULL,"
        << ":departure_object_id,"
        << ":arrival_object_id,"
        << ":departure_epoch,"
        << ":time_of_flight,"
        << ":revolutions,"
        << ":prograde,"
        << ":zoom_loop_counter,"
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

    std::ostringstream lambertScannerSubsetTableInsert;
    lambertScannerSubsetTableInsert
        << "INSERT INTO lambert_scanner_zoom_results_subset VALUES ("
        << "NULL,"
        << ":departure_object_id,"
        << ":arrival_object_id,"
        << ":departure_epoch,"
        << ":arrival_epoch,"
        << ":transfer_delta_v"
        << ");";

    SQLite::Statement querySubset( database, lambertScannerSubsetTableInsert.str( ) );



    std::cout << "Computing Lambert transfers and populating database ... " << std::endl;

    // Create a vector of tuple's that contain times after departure epoch, times of flight and
    // associated dVs.
    // Loop over all departure objects.
    boost::progress_display showProgress( tleObjects.size( ) );
    int bibaboe = 0;
    // #pragma omp parallel for num_threads( 4 )
    for ( unsigned int i = 0; i < tleObjects.size( ); i++ )
    {
        // Compute departure state.
        Tle departureObject = tleObjects[ i ];
        SGP4 sgp4Departure( departureObject );
        const int departureObjectId = static_cast< int >( departureObject.NoradNumber( ) );


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


            std::vector< boost::tuple< double, double, double > > combinations;

            // Fill the vector with the initial grid. Loop over departure epoch grid.
            for ( int m = 0; m < input.departureEpochSteps; ++m )
            {
                const double departureEpoch = input.departureEpochStepSize * m;
                // Loop over time-of-flight grid.
                for ( int k = 0; k < input.timeOfFlightSteps; k++ )
                {
                    const double timeOfFlight
                        = input.timeOfFlightMinimum + k * input.timeOfFlightStepSize;

                    // Set initial dV to 100.0
                    combinations.push_back( boost::make_tuple( departureEpoch, timeOfFlight, 100.0));
                }
            }

            // Set stepsizes based on input from user.
            double departureEpochStepSize = input.departureEpochStepSize;
            double timeOfFlightStepSize = input.timeOfFlightStepSize;

            // Tle departureObject = tleObjects[ 0 ];
            // Tle arrivalObject = tleObjects[ 1 ];

            // SGP4 sgp4Departure( departureObject );
            // SGP4 sgp4Arrival( arrivalObject );

            // const int arrivalObjectId = static_cast< int >( arrivalObject.NoradNumber( ) );
            // const int departureObjectId = static_cast< int >( departureObject.NoradNumber( ) );

            bool firstloop = true;
            int zoomLoopCounter=0;
            // for (int z = 0; z < input.iterations; z++)
            while ( departureEpochStepSize > 1 || timeOfFlightStepSize > 1 )
            {

                if (zoomLoopCounter==input.iterations)
                {
                    // std::cout << "Max iterations reached: " << zoomLoopCounter << std::endl;
                    break;
                }
                int lengte = combinations.size();

                // For every iteration after the first, the best combinations are used to create a finer
                // grid to find a lower dV.
                if (firstloop==false)
                {
                    if (boost::get<2>(combinations[0]) > 2.0)
                    {
                        bibaboe++;
                        break;
                    }
                    // The stepsize is half of the previous stepsize at each iteration.
                    departureEpochStepSize = departureEpochStepSize*input.multiplier;
                    timeOfFlightStepSize = timeOfFlightStepSize*input.multiplier;

                    // The grid is constructed as follows, half a step below the current point 3 new points.
                    // Half a step to the left and the right and 3 points above the current point.
                    for (int l = 0; l < lengte; ++l)
                    {
                        combinations.push_back(boost::make_tuple(
                            get<0>(combinations[l])-departureEpochStepSize,
                            get<1>(combinations[l])-timeOfFlightStepSize,
                            100.0));
                        combinations.push_back(boost::make_tuple(
                            get<0>(combinations[l]),
                            get<1>(combinations[l])-timeOfFlightStepSize,
                            100.0));
                        combinations.push_back(boost::make_tuple(
                            get<0>(combinations[l])+departureEpochStepSize,
                            get<1>(combinations[l])-timeOfFlightStepSize,
                            100.0));

                        combinations.push_back(boost::make_tuple(
                            get<0>(combinations[l])-departureEpochStepSize,
                            get<1>(combinations[l]),
                            100.0));
                        combinations.push_back(boost::make_tuple(
                            get<0>(combinations[l])+departureEpochStepSize,
                            get<1>(combinations[l]),
                            100.0));

                        combinations.push_back(boost::make_tuple(
                            get<0>(combinations[l])-departureEpochStepSize,
                            get<1>(combinations[l])+timeOfFlightStepSize,
                            100.0));
                        combinations.push_back(boost::make_tuple(
                            get<0>(combinations[l]),
                            get<1>(combinations[l])+timeOfFlightStepSize,
                            100.0));
                        combinations.push_back(boost::make_tuple(
                            get<0>(combinations[l])+departureEpochStepSize,
                            get<1>(combinations[l])+timeOfFlightStepSize,
                            100.0));
                    }

                // Sort the vector with all grid points, including those remaining from previous iterations.
                std::sort(combinations.begin(), combinations.end(), sortByTime);

                // Remove duplicates in the grid. Duplicates are those with equal departure epoch and time
                // of flight.
                std::vector<boost::tuple< double, double, double > >::iterator it;
                it = std::unique(combinations.begin(), combinations.end(), gridPointComparison);
                combinations.resize( std::distance(combinations.begin(),it) );
                }

                // Set boolean to not skip the creation of the extra gridpoints after the first iteration.
                firstloop = false;

                // Loop over entire set of combinations and calculate dV based on Lambert.
                for ( unsigned int i = 0; i < combinations.size(); i++ )
                {
                    // New combinations are initaialized with dV at 100.0, only those nee to be calculated.
                    if (boost::get<2>(combinations[i]) != 100.0)
                    {
                        continue;
                    }

                    // Grid points are collected to create useable times.
                    double currentDepartureEpoch = boost::get<0>(combinations[i]);
                    double timeOfFlight = boost::get<1>(combinations[i]);

                    // Remove grid point that have negative departure epoch or time of flight.
                    if (currentDepartureEpoch<0 || timeOfFlight<0 )
                    {
                        boost::get<2>(combinations[i]) = 99.0;
                        continue;
                    }

                    // Compute departure state.
                    DateTime departureEpoch = input.departureEpochInitial;
                    if ( input.departureEpochInitial == DateTime( ) )
                    {
                        departureEpoch = departureObject.Epoch( );
                    }

                    // Set departure vector.
                    departureEpoch = departureEpoch.AddSeconds( currentDepartureEpoch );
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

                    // Set arrival vector
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

                    for ( int j = 0; j < numberOfSolutions; j++ )
                    {
                        // Compute Delta-V for transfer.
                        const Vector3 transferDepartureVelocity = targeter.get_v1( )[ j ];
                        const Vector3 transferArrivalVelocity = targeter.get_v2( )[ j ];

                        departureDeltaVs[ j ] = sml::add( transferDepartureVelocity,
                                                          sml::multiply( departureVelocity, -1.0 ) );
                        arrivalDeltaVs[ j ]   = sml::add( arrivalVelocity,
                                                          sml::multiply( transferArrivalVelocity, -1.0 ) );

                        transferDeltaVs[ j ]
                            = sml::norm< double >( departureDeltaVs[ j ] )
                                + sml::norm< double >( arrivalDeltaVs[ j ] );
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

                    // Update the dV with the lowest one from the Lambert targetter
                    boost::get<2>(combinations[i]) = *minimumDeltaVIterator;

                    // Bind values to SQL insert query.
                    if (zoomLoopCounter>=input.iterations-3 && *minimumDeltaVIterator < 1.0)
                    // if (zoomLoopCounter==input.iterations-1)
                    {
            
                        // #pragma omp critical( database_operations )
                        {
                            query.bind( ":departure_object_id",  departureObjectId );
                            query.bind( ":arrival_object_id",    arrivalObjectId );
                            query.bind( ":departure_epoch",      departureEpoch.ToJulian( ) );
                            query.bind( ":time_of_flight",       timeOfFlight );
                            query.bind( ":revolutions",          revolutions );
                            query.bind( ":prograde",             input.isPrograde );
                            query.bind( ":zoom_loop_counter",    zoomLoopCounter );
                            query.bind( ":departure_position_x", departureState[ astro::xPositionIndex ] );
                            query.bind( ":departure_position_y", departureState[ astro::yPositionIndex ] );
                            query.bind( ":departure_position_z", departureState[ astro::zPositionIndex ] );
                            query.bind( ":departure_velocity_x", departureState[ astro::xVelocityIndex ] );
                            query.bind( ":departure_velocity_y", departureState[ astro::yVelocityIndex ] );
                            query.bind( ":departure_velocity_z", departureState[ astro::zVelocityIndex ] );
                            query.bind( ":departure_semi_major_axis",
                                departureStateKepler[ astro::semiMajorAxisIndex ] );
                            query.bind( ":departure_eccentricity",
                                departureStateKepler[ astro::eccentricityIndex ] );
                            query.bind( ":departure_inclination",
                                departureStateKepler[ astro::inclinationIndex ] );
                            query.bind( ":departure_argument_of_periapsis",
                                departureStateKepler[ astro::argumentOfPeriapsisIndex ] );
                            query.bind( ":departure_longitude_of_ascending_node",
                                departureStateKepler[ astro::longitudeOfAscendingNodeIndex ] );
                            query.bind( ":departure_true_anomaly",
                                departureStateKepler[ astro::trueAnomalyIndex ] );
                            query.bind( ":arrival_position_x",  arrivalState[ astro::xPositionIndex ] );
                            query.bind( ":arrival_position_y",  arrivalState[ astro::yPositionIndex ] );
                            query.bind( ":arrival_position_z",  arrivalState[ astro::zPositionIndex ] );
                            query.bind( ":arrival_velocity_x",  arrivalState[ astro::xVelocityIndex ] );
                            query.bind( ":arrival_velocity_y",  arrivalState[ astro::yVelocityIndex ] );
                            query.bind( ":arrival_velocity_z",  arrivalState[ astro::zVelocityIndex ] );
                            query.bind( ":arrival_semi_major_axis",
                                arrivalStateKepler[ astro::semiMajorAxisIndex ] );
                            query.bind( ":arrival_eccentricity",
                                arrivalStateKepler[ astro::eccentricityIndex ] );
                            query.bind( ":arrival_inclination",
                                arrivalStateKepler[ astro::inclinationIndex ] );
                            query.bind( ":arrival_argument_of_periapsis",
                                arrivalStateKepler[ astro::argumentOfPeriapsisIndex ] );
                            query.bind( ":arrival_longitude_of_ascending_node",
                                arrivalStateKepler[ astro::longitudeOfAscendingNodeIndex ] );
                            query.bind( ":arrival_true_anomaly",
                                arrivalStateKepler[ astro::trueAnomalyIndex ] );
                            query.bind( ":transfer_semi_major_axis",
                                transferStateKepler[ astro::semiMajorAxisIndex ] );
                            query.bind( ":transfer_eccentricity",
                                transferStateKepler[ astro::eccentricityIndex ] );
                            query.bind( ":transfer_inclination",
                                transferStateKepler[ astro::inclinationIndex ] );
                            query.bind( ":transfer_argument_of_periapsis",
                                transferStateKepler[ astro::argumentOfPeriapsisIndex ] );
                            query.bind( ":transfer_longitude_of_ascending_node",
                                transferStateKepler[ astro::longitudeOfAscendingNodeIndex ] );
                            query.bind( ":transfer_true_anomaly",
                                transferStateKepler[ astro::trueAnomalyIndex ] );
                            query.bind( ":departure_delta_v_x", departureDeltaVs[ minimumDeltaVIndex ][ 0 ] );
                            query.bind( ":departure_delta_v_y", departureDeltaVs[ minimumDeltaVIndex ][ 1 ] );
                            query.bind( ":departure_delta_v_z", departureDeltaVs[ minimumDeltaVIndex ][ 2 ] );
                            query.bind( ":arrival_delta_v_x",   arrivalDeltaVs[ minimumDeltaVIndex ][ 0 ] );
                            query.bind( ":arrival_delta_v_y",   arrivalDeltaVs[ minimumDeltaVIndex ][ 1 ] );
                            query.bind( ":arrival_delta_v_z",   arrivalDeltaVs[ minimumDeltaVIndex ][ 2 ] );
                            query.bind( ":transfer_delta_v",    *minimumDeltaVIterator );

                            // Execute insert query.
                            query.executeStep( );

                            // Reset SQL insert query.
                            query.reset( );
                        }
                    }
                }

                // Sort the vector with all grid points, including those remaining from previous iterations.
                std::sort(combinations.begin(), combinations.end(), sortByDV);

                //
                // int topPoints = round(combinations.size() * 0.2 );
                // if (topPoints > input.topPoints)
                // {
                //     topPoints = input.topPoints;
                // }
                // std::cout << topPoints << std::endl;
                combinations.erase(combinations.begin()+input.topPoints,combinations.end());
                zoomLoopCounter++;
            }
        }
        ++showProgress;

    }
    // Commit transaction.
    // transaction.commit( );

    // std::cout << combinations[0].get<2>() << std::endl;
    // std::cout << z << std::endl;
    std::cout << "bibaboe: " << bibaboe << std::endl;
    std::cout << std::endl;
    std::cout << "Database populated successfully!" << std::endl;
    std::cout << std::endl;
    std::ostringstream getNumberOfOccurences;
    getNumberOfOccurences <<    "SELECT departure_object_id, "
                          <<    "arrival_object_id, "
                          <<    "min(transfer_delta_v),"
                          <<    "count(*) "
                          <<    "AS number_of_occurences "
                          <<    "FROM lambert_scanner_zoom_results "
                          <<    "GROUP BY departure_object_id, arrival_object_id "
                          <<    "ORDER BY number_of_occurences "
                          <<    ";";

    SQLite::Statement occurencesQuery( database, getNumberOfOccurences.str( ) );

    // Create multimap of departure-arrival combinations.
    std::multimap< int, int > combinations;
    
    // Create multimap with data of each combination.
    std::map< std::pair<int, int>, double> allDVs;
    while ( occurencesQuery.executeStep( ) )
    {
        int departureObject = occurencesQuery.getColumn( 0 );
        int arrivalObject   = occurencesQuery.getColumn( 1 );
        double transferDeltaV = occurencesQuery.getColumn( 2 );

        departureArrivalCombo combo = std::make_pair( departureObject, arrivalObject );
        combinations.insert( combo );       
        allDVs.insert(std::make_pair(combo, transferDeltaV));
    }

    // Create one vector with all objects and one with all departure obects
    std::vector< int > allObjects;
    std::vector< int > allDepartureObjects;
    for( std::multimap< int, int >::iterator itAllDepartureObjects = combinations.begin( );
         itAllDepartureObjects != combinations.end( ); 
         ++itAllDepartureObjects )
    {
        allObjects.push_back( itAllDepartureObjects->first );
        allObjects.push_back( itAllDepartureObjects->second );
        allDepartureObjects.push_back( itAllDepartureObjects->first );
    }

    // Make list of all depature objects unique. 
    std::sort( allDepartureObjects.begin( ), allDepartureObjects.end( ) );           
    std::vector< int > uniqueDepartureObjects(allDepartureObjects.size( ) );
    std::vector< int >::iterator itUniqueDepartureObjects;
    itUniqueDepartureObjects = std::unique_copy(    allDepartureObjects.begin( ),
                                                    allDepartureObjects.end( ),
                                                    uniqueDepartureObjects.begin( ) );

    uniqueDepartureObjects.resize( std::distance(   uniqueDepartureObjects.begin( ),
                                                    itUniqueDepartureObjects ) );
    
    // Make list of all objects unique. 
    std::sort( allObjects.begin( ), allObjects.end( ) );           
    std::vector< int > uniqueObjects( allObjects.size( ) );
    std::vector< int >::iterator itUniqueObjects;
    itUniqueObjects = std::unique_copy( allObjects.begin( ), 
                                        allObjects.end( ), 
                                        uniqueObjects.begin( ) );

    uniqueObjects.resize( std::distance(    uniqueObjects.begin( ), 
                                            itUniqueObjects ) );

    now = time(0);
    localtm = localtime(&now);
    std::cout << "List made, making allDatapoints." << std::endl;
    std::cout << "The time is: " << asctime(localtm) << std::endl;
    mapOflistsofdatapoints allDatapoints;
    int totalpoints = 0;
    
    for ( std::multimap< int, int >::iterator itCombinations = combinations.begin( );
          itCombinations != combinations.end( ); 
          itCombinations++)
    {
        std::pair< int, int > combo;
        combo = std::make_pair( itCombinations->first, itCombinations->second );
        double bestTransferDeltaV = allDVs.find( combo )->second;
        
        std::ostringstream getCurrentCombination;
        getCurrentCombination <<    "SELECT  departure_object_id, "
                              <<    " arrival_object_id, "
                              <<    " departure_epoch, "
                              <<    " time_of_flight, "
                              <<    " transfer_delta_v "
                              <<    "FROM lambert_scanner_zoom_results "
                              <<    "WHERE departure_object_id = "
                              <<    itCombinations->first
                              <<    " AND arrival_object_id = "
                              <<    itCombinations->second
                              <<    " ORDER BY (departure_epoch + time_of_flight/86400 )  ASC "
                              <<    ";";

        SQLite::Statement currentQuery( database, getCurrentCombination.str( ) );
        vectorOfDatapoints currentVectorOfDatapoints;
        int transferId=1;
        
        while ( currentQuery.executeStep( ) )
        {    
            int departureObject = currentQuery.getColumn( 0 );
            int arrivalObject =   currentQuery.getColumn( 1 );
            
            double departureEpoch = currentQuery.getColumn( 2 );
            departureEpoch = departureEpoch -2457400;

            double timeOfFlight   = currentQuery.getColumn( 3 );
            timeOfFlight = timeOfFlight  / 86400;

            double transferDeltaV = currentQuery.getColumn( 4 );
            double  arrivalEpoch = departureEpoch + timeOfFlight;
            
            currentVectorOfDatapoints.push_back( 
                LambertPorkChopPlotGridPoint( transferId,
                                              departureEpoch,
                                              arrivalEpoch,
                                              timeOfFlight,
                                              transferDeltaV ) );
            transferId++;
        }

        // Remove datapoints after best transfer deltaV
        int bestDeltaVIterator = 0;
        for (unsigned int k = 0; k < currentVectorOfDatapoints.size( ); ++k)
        {
            if ( currentVectorOfDatapoints[ k ].transferDeltaV == bestTransferDeltaV )
            {
                // std::cout << currentVectorOfDatapoints[k].transferDeltaV << std::endl;
                break;
            }
            bestDeltaVIterator++;
        }

        currentVectorOfDatapoints.erase( 
            currentVectorOfDatapoints.begin( ) + bestDeltaVIterator + 1, 
            currentVectorOfDatapoints.end( ) );

        // Create subset of best datapoints within an arrivalEpoch window.
        listOfDatapoints bestDatapoints;
        double earliestArrivalEpoch = currentVectorOfDatapoints.front( ).arrivalEpoch;
        double currentLowestDeltaV = currentVectorOfDatapoints.front( ).transferDeltaV;
        
        int currentBestDatapoint = 0;
        int lastBestDatapoint = 0;

        for (unsigned int k = 0; k < currentVectorOfDatapoints.size( ); ++k)
        {
            if (currentVectorOfDatapoints[ k ].arrivalEpoch < earliestArrivalEpoch + 0.5 )
            {
                if ( currentVectorOfDatapoints[ k ].transferDeltaV < currentLowestDeltaV )
                {
                    currentBestDatapoint = k;
                }
            }
            else
            {
                if (lastBestDatapoint != currentBestDatapoint)
                {
                    bestDatapoints.push_back(
                        currentVectorOfDatapoints[ currentBestDatapoint ]);                   
                }
                lastBestDatapoint = currentBestDatapoint;
                currentLowestDeltaV = 1.0;
                earliestArrivalEpoch = earliestArrivalEpoch + 0.5;
            }

        }
        bestDatapoints.push_back( currentVectorOfDatapoints.back( ) );
        allDatapoints.insert( std::make_pair( combo, bestDatapoints ) );
       
        totalpoints = totalpoints + bestDatapoints.size();
    }

// Print to screen the number of solutions found. 
    for (mapOflistsofdatapoints::iterator i = allDatapoints.begin(); i != allDatapoints.end( ); ++i)
    {
        std::cout << i->first.first << " to " << i->first.second << " for " << i->second.back().transferDeltaV << " has " << i->second.size() << " solutions." << std::endl;

        listOfDatapoints currentList = i->second;
        // currentList.sort(compareByArrivalEpoch); // compareBy defined at end of this cpp file
        for (listOfDatapoints::iterator j = currentList.begin(); j != currentList.end( ); ++j)   
        {
            std::cout << j->departureEpoch << " "  << j->arrivalEpoch << " " << j->transferDeltaV << std::endl;

            querySubset.bind( ":departure_object_id",  i->first.first );
            querySubset.bind( ":arrival_object_id",    i->first.second );
            querySubset.bind( ":departure_epoch",      j->departureEpoch );
            querySubset.bind( ":arrival_epoch",      j->arrivalEpoch );                       
            querySubset.bind( ":transfer_delta_v",    j->transferDeltaV );

            // Execute insert querySubset.
            querySubset.executeStep( );

            // Reset SQL insert querySubset.
            querySubset.reset( );

        }        
    }

    transaction.commit( );


    // for (int i = 0; i < 10; ++i)
    // {
    //     std::cout << combinations[i].get<0>() << ", " << combinations[i].get<1>() << ", " << combinations[i].get<2>() << std::endl;
    // }

    // Check if shortlist file should be created; call function to write output.
    if ( input.shortlistLength > 0 )
    {
        std::cout << "Writing shortlist to file ... " << std::endl;
        writeTransferZoomShortlist( database, input.shortlistLength, input.shortlistPath );
        std::cout << "Shortlist file created successfully!" << std::endl;
    }
}

//! Check lambert_scanner input parameters.
LambertZoomInput checkLambertZoomInput( const rapidjson::Document& config )
{
    const std::string catalogPath = find( config, "catalog" )->value.GetString( );
    std::cout << "Catalog                       " << catalogPath << std::endl;

    const std::string databasePath = find( config, "database" )->value.GetString( );
    std::cout << "Database                      " << databasePath << std::endl;

    const ConfigIterator departureEpochIterator = find( config, "departure_epoch" );
    std::map< std::string, int > departureEpochElements;
    if ( departureEpochIterator->value.Size( ) == 0 )
    {
        DateTime dummyEpoch;
        departureEpochElements[ "year" ]    = dummyEpoch.Year( );
        departureEpochElements[ "month" ]   = dummyEpoch.Month( );
        departureEpochElements[ "day" ]     = dummyEpoch.Day( );
        departureEpochElements[ "hours" ]   = dummyEpoch.Hour( );
        departureEpochElements[ "minutes" ] = dummyEpoch.Minute( );
        departureEpochElements[ "seconds" ] = dummyEpoch.Second( );
    }

    else
    {
        departureEpochElements[ "year" ]    = departureEpochIterator->value[ 0 ].GetInt( );
        departureEpochElements[ "month" ]   = departureEpochIterator->value[ 1 ].GetInt( );
        departureEpochElements[ "day" ]     = departureEpochIterator->value[ 2 ].GetInt( );

        if ( departureEpochIterator->value.Size( ) > 3 )
        {
            departureEpochElements[ "hours" ] = departureEpochIterator->value[ 3 ].GetInt( );

            if ( departureEpochIterator->value.Size( ) > 4 )
            {
                departureEpochElements[ "minutes" ] = departureEpochIterator->value[ 4 ].GetInt( );

                if ( departureEpochIterator->value.Size( ) > 5 )
                {
                    departureEpochElements[ "seconds" ]
                        = departureEpochIterator->value[ 5 ].GetInt( );
                }
            }
        }
    }

    const DateTime departureEpoch( departureEpochElements[ "year" ],
                                   departureEpochElements[ "month" ],
                                   departureEpochElements[ "day" ],
                                   departureEpochElements[ "hours" ],
                                   departureEpochElements[ "minutes" ],
                                   departureEpochElements[ "seconds" ] );

    if ( departureEpochIterator->value.Size( ) == 0 )
    {
        std::cout << "Departure epoch               TLE epoch" << std::endl;
    }

    else
    {
        std::cout << "Departure epoch               " << departureEpoch << std::endl;
    }

    const double departureEpochRange
        = find( config, "departure_epoch_grid" )->value[ 0 ].GetDouble( );
    std::cout << "Departure epoch grid range    " << departureEpochRange << std::endl;
    const double departureGridSteps
        = find( config, "departure_epoch_grid" )->value[ 1 ].GetDouble( );
    std::cout << "Departure epoch grid steps    " << departureGridSteps << std::endl;

    const double timeOfFlightMinimum
        = find( config, "time_of_flight_grid" )->value[ 0 ].GetDouble( );
    std::cout << "Minimum Time-of-Flight        " << timeOfFlightMinimum << std::endl;
    const double timeOfFlightMaximum
        = find( config, "time_of_flight_grid" )->value[ 1 ].GetDouble( );
    std::cout << "Maximum Time-of-Flight        " << timeOfFlightMaximum << std::endl;

    const double multiplier
        = find( config, "multiplier" )->value.GetDouble( );
    std::cout << "Multiplier                    " << multiplier << std::endl;

    if ( timeOfFlightMinimum > timeOfFlightMaximum )
    {
        throw std::runtime_error( "ERROR: Maximum time-of-flight must be larger than minimum!" );
    }

    const double timeOfFlightSteps
        = find( config, "time_of_flight_grid" )->value[ 2 ].GetDouble( );
    std::cout << "# Time-of-Flight steps        " << timeOfFlightSteps << std::endl;

    const bool isPrograde = find( config, "is_prograde" )->value.GetBool( );
    if ( isPrograde )
    {
        std::cout << "Prograde transfer?            true" << std::endl;
    }
    else
    {
        std::cout << "Prograde transfer?            false" << std::endl;
    }

    const int iterations = find( config, "iterations" )->value.GetInt( );
    std::cout << "Number of iterations          " << iterations << std::endl;

    const int topPoints = find( config, "top_points" )->value.GetInt( );
    std::cout << "Number of top points used     " << topPoints << std::endl;

    const int revolutionsMaximum = find( config, "revolutions_maximum" )->value.GetInt( );
    std::cout << "Maximum revolutions           " << revolutionsMaximum << std::endl;

    const int shortlistLength = find( config, "shortlist" )->value[ 0 ].GetInt( );
    std::cout << "# of shortlist transfers      " << shortlistLength << std::endl;

    std::string shortlistPath = "";
    if ( shortlistLength > 0 )
    {
        shortlistPath = find( config, "shortlist" )->value[ 1 ].GetString( );
        std::cout << "Shortlist                     " << shortlistPath << std::endl;
    }

    return LambertZoomInput( catalogPath,
                             databasePath,
                             departureEpoch,
                             departureGridSteps,
                             departureEpochRange/departureGridSteps,
                             timeOfFlightMinimum,
                             timeOfFlightMaximum,
                             timeOfFlightSteps,
                             ( timeOfFlightMaximum - timeOfFlightMinimum ) / timeOfFlightSteps,
                             multiplier,
                             isPrograde,
                             revolutionsMaximum,
                             iterations,
                             topPoints,
                             shortlistLength,
                             shortlistPath );
}

//! Create lambert_scanner table.
void createLambertZoomTable( SQLite::Database& database )
{
    // Drop table from database if it exists.
    database.exec( "DROP TABLE IF EXISTS lambert_scanner_zoom_results;" );

    // Set up SQL command to create table to store lambert_scanner results.
    std::ostringstream lambertScannerTableCreate;
    lambertScannerTableCreate
        << "CREATE TABLE lambert_scanner_zoom_results ("
        << "\"transfer_id\"                             INTEGER PRIMARY KEY AUTOINCREMENT,"
        << "\"departure_object_id\"                     TEXT,"
        << "\"arrival_object_id\"                       TEXT,"
        << "\"departure_epoch\"                         REAL,"
        << "\"time_of_flight\"                          REAL,"
        << "\"revolutions\"                             INTEGER,"
        // N.B.: SQLite doesn't support booleans so 0 = false, 1 = true for 'prograde'
        << "\"prograde\"                                INTEGER,"
        << "\"zoom_loop_counter\"                       INTEGER,"
        << "\"departure_position_x\"                    REAL,"
        << "\"departure_position_y\"                    REAL,"
        << "\"departure_position_z\"                    REAL,"
        << "\"departure_velocity_x\"                    REAL,"
        << "\"departure_velocity_y\"                    REAL,"
        << "\"departure_velocity_z\"                    REAL,"
        << "\"departure_semi_major_axis\"               REAL,"
        << "\"departure_eccentricity\"                  REAL,"
        << "\"departure_inclination\"                   REAL,"
        << "\"departure_argument_of_periapsis\"         REAL,"
        << "\"departure_longitude_of_ascending_node\"   REAL,"
        << "\"departure_true_anomaly\"                  REAL,"
        << "\"arrival_position_x\"                      REAL,"
        << "\"arrival_position_y\"                      REAL,"
        << "\"arrival_position_z\"                      REAL,"
        << "\"arrival_velocity_x\"                      REAL,"
        << "\"arrival_velocity_y\"                      REAL,"
        << "\"arrival_velocity_z\"                      REAL,"
        << "\"arrival_semi_major_axis\"                 REAL,"
        << "\"arrival_eccentricity\"                    REAL,"
        << "\"arrival_inclination\"                     REAL,"
        << "\"arrival_argument_of_periapsis\"           REAL,"
        << "\"arrival_longitude_of_ascending_node\"     REAL,"
        << "\"arrival_true_anomaly\"                    REAL,"
        << "\"transfer_semi_major_axis\"                REAL,"
        << "\"transfer_eccentricity\"                   REAL,"
        << "\"transfer_inclination\"                    REAL,"
        << "\"transfer_argument_of_periapsis\"          REAL,"
        << "\"transfer_longitude_of_ascending_node\"    REAL,"
        << "\"transfer_true_anomaly\"                   REAL,"
        << "\"departure_delta_v_x\"                     REAL,"
        << "\"departure_delta_v_y\"                     REAL,"
        << "\"departure_delta_v_z\"                     REAL,"
        << "\"arrival_delta_v_x\"                       REAL,"
        << "\"arrival_delta_v_y\"                       REAL,"
        << "\"arrival_delta_v_z\"                       REAL,"
        << "\"transfer_delta_v\"                        REAL"
        <<                                              ");";

    // Execute command to create table.
    database.exec( lambertScannerTableCreate.str( ).c_str( ) );

    // Execute command to create index on transfer Delta-V column.
    std::ostringstream transferDeltaVIndexCreate;
    transferDeltaVIndexCreate << "CREATE INDEX IF NOT EXISTS \"transfer_delta_v\" on "
                              << "lambert_scanner_zoom_results (transfer_delta_v ASC);";
    database.exec( transferDeltaVIndexCreate.str( ).c_str( ) );

    if ( !database.tableExists( "lambert_scanner_zoom_results" ) )
    {
        throw std::runtime_error( "ERROR: Creating table 'lambert_scanner_zoom_results' failed!" );
    }


    database.exec( "DROP TABLE IF EXISTS lambert_scanner_zoom_results_subset;" );

    // Set up SQL command to create table to store lambert_scanner results.
    std::ostringstream lambertScannerSubsetTableCreate;
    lambertScannerSubsetTableCreate
        << "CREATE TABLE lambert_scanner_zoom_results_subset ("
        << "\"transfer_id\"                             INTEGER PRIMARY KEY AUTOINCREMENT,"
        << "\"departure_object_id\"                     TEXT,"
        << "\"arrival_object_id\"                       TEXT,"
        << "\"departure_epoch\"                         REAL,"
        << "\"arrival_epoch\"                          REAL,"
        << "\"transfer_delta_v\"                        REAL"
        <<                                              ");";

    // Execute command to create table.
    database.exec( lambertScannerSubsetTableCreate.str( ).c_str( ) );

    if ( !database.tableExists( "lambert_scanner_zoom_results_subset" ) )
    {
        throw std::runtime_error( "ERROR: Creating table 'lambert_scanner_zoom_results_subset' failed!" );
    }

}

//! Write transfer shortlist to file.
void writeTransferZoomShortlist( SQLite::Database& database,
                             const int shortlistNumber,
                             const std::string& shortlistPath )
{
    // Fetch transfers to include in shortlist.
    // Database table is sorted by transfer_delta_v, in ascending order.
    std::ostringstream shortlistSelect;
    shortlistSelect <<  "SELECT * "
                    <<  "FROM lambert_scanner_zoom_results "
                    <<  "GROUP BY departure_object_id, arrival_object_id "
                    <<  "ORDER BY transfer_delta_v ASC "
                    <<  "LIMIT "
                    << shortlistNumber << ";";

    SQLite::Statement query( database, shortlistSelect.str( ) );

    // Write fetch data to file.
    std::ofstream shortlistFile( shortlistPath.c_str( ) );

    // Print file header.
    shortlistFile << "transfer_id,"
                  << "departure_object_id,"
                  << "arrival_object_id,"
                  << "departure_epoch,"
                  << "time_of_flight,"
                  << "revolutions,"
                  << "prograde,"
                  << "zoom_loop_counter,"
                  << "departure_position_x,"
                  << "departure_position_y,"
                  << "departure_position_z,"
                  << "departure_velocity_x,"
                  << "departure_velocity_y,"
                  << "departure_velocity_z,"
                  << "departure_semi_major_axis,"
                  << "departure_eccentricity,"
                  << "departure_inclination,"
                  << "departure_argument_of_periapsis,"
                  << "departure_longitude_of_ascending_node,"
                  << "departure_true_anomaly,"
                  << "arrival_position_x,"
                  << "arrival_position_y,"
                  << "arrival_position_z,"
                  << "arrival_velocity_x,"
                  << "arrival_velocity_y,"
                  << "arrival_velocity_z,"
                  << "arrival_semi_major_axis,"
                  << "arrival_eccentricity,"
                  << "arrival_inclination,"
                  << "arrival_argument_of_periapsis,"
                  << "arrival_longitude_of_ascending_node,"
                  << "arrival_true_anomaly,"
                  << "transfer_semi_major_axis,"
                  << "transfer_eccentricity,"
                  << "transfer_inclination,"
                  << "transfer_argument_of_periapsis,"
                  << "transfer_longitude_of_ascending_node,"
                  << "transfer_true_anomaly,"
                  << "departure_delta_v_x,"
                  << "departure_delta_v_y,"
                  << "departure_delta_v_z,"
                  << "arrival_delta_v_x,"
                  << "arrival_delta_v_y,"
                  << "arrival_delta_v_z,"
                  << "transfer_delta_v"
                  << std::endl;

    // Loop through data retrieved from database and write to file.
    while( query.executeStep( ) )
    {
        const int    transferId                         = query.getColumn( 0 );
        const int    departureObjectId                  = query.getColumn( 1 );
        const int    arrivalObjectId                    = query.getColumn( 2 );
        const double departureEpoch                     = query.getColumn( 3 );
        const double timeOfFlight                       = query.getColumn( 4 );
        const int    revolutions                        = query.getColumn( 5 );
        const int    prograde                           = query.getColumn( 6 );
        const int    zoom_loop_counter                  = query.getColumn( 7 );
        const double departurePositionX                 = query.getColumn( 8 );
        const double departurePositionY                 = query.getColumn( 9 );
        const double departurePositionZ                 = query.getColumn( 10 );
        const double departureVelocityX                 = query.getColumn( 11 );
        const double departureVelocityY                 = query.getColumn( 12 );
        const double departureVelocityZ                 = query.getColumn( 13 );
        const double departureSemiMajorAxis             = query.getColumn( 14 );
        const double departureEccentricity              = query.getColumn( 15 );
        const double departureInclination               = query.getColumn( 16 );
        const double departureArgumentOfPeriapsis       = query.getColumn( 17 );
        const double departureLongitudeOfAscendingNode  = query.getColumn( 18 );
        const double departureTrueAnomaly               = query.getColumn( 19 );
        const double arrivalPositionX                   = query.getColumn( 20 );
        const double arrivalPositionY                   = query.getColumn( 21 );
        const double arrivalPositionZ                   = query.getColumn( 22 );
        const double arrivalVelocityX                   = query.getColumn( 23 );
        const double arrivalVelocityY                   = query.getColumn( 24 );
        const double arrivalVelocityZ                   = query.getColumn( 25 );
        const double arrivalSemiMajorAxis               = query.getColumn( 26 );
        const double arrivalEccentricity                = query.getColumn( 27 );
        const double arrivalInclination                 = query.getColumn( 28 );
        const double arrivalArgumentOfPeriapsis         = query.getColumn( 29 );
        const double arrivalLongitudeOfAscendingNode    = query.getColumn( 30 );
        const double arrivalTrueAnomaly                 = query.getColumn( 31 );
        const double transferSemiMajorAxis              = query.getColumn( 32 );
        const double transferEccentricity               = query.getColumn( 33 );
        const double transferInclination                = query.getColumn( 34 );
        const double transferArgumentOfPeriapsis        = query.getColumn( 35 );
        const double transferLongitudeOfAscendingNode   = query.getColumn( 36 );
        const double transferTrueAnomaly                = query.getColumn( 37 );
        const double departureDeltaVX                   = query.getColumn( 38 );
        const double departureDeltaVY                   = query.getColumn( 39 );
        const double departureDeltaVZ                   = query.getColumn( 40 );
        const double arrivalDeltaVX                     = query.getColumn( 41 );
        const double arrivalDeltaVY                     = query.getColumn( 42 );
        const double arrivalDeltaVZ                     = query.getColumn( 43 );
        const double transferDeltaV                     = query.getColumn( 44 );

        shortlistFile << transferId                         << ","
                      << departureObjectId                  << ","
                      << arrivalObjectId                    << ","
                      << departureEpoch-2457000             << ","
                      << timeOfFlight                       << ","
                      << revolutions                        << ","
                      << prograde                           << ","
                      << zoom_loop_counter                  << ","
                      << departurePositionX                 << ","
                      << departurePositionY                 << ","
                      << departurePositionZ                 << ","
                      << departureVelocityX                 << ","
                      << departureVelocityY                 << ","
                      << departureVelocityZ                 << ","
                      << departureSemiMajorAxis             << ","
                      << departureEccentricity              << ","
                      << departureInclination               << ","
                      << departureArgumentOfPeriapsis       << ","
                      << departureLongitudeOfAscendingNode  << ","
                      << departureTrueAnomaly               << ","
                      << arrivalPositionX                   << ","
                      << arrivalPositionY                   << ","
                      << arrivalPositionZ                   << ","
                      << arrivalVelocityX                   << ","
                      << arrivalVelocityY                   << ","
                      << arrivalVelocityZ                   << ","
                      << arrivalSemiMajorAxis               << ","
                      << arrivalEccentricity                << ","
                      << arrivalInclination                 << ","
                      << arrivalArgumentOfPeriapsis         << ","
                      << arrivalLongitudeOfAscendingNode    << ","
                      << arrivalTrueAnomaly                 << ","
                      << transferSemiMajorAxis              << ","
                      << transferEccentricity               << ","
                      << transferInclination                << ","
                      << transferArgumentOfPeriapsis        << ","
                      << transferLongitudeOfAscendingNode   << ","
                      << transferTrueAnomaly                << ","
                      << departureDeltaVX                   << ","
                      << departureDeltaVY                   << ","
                      << departureDeltaVZ                   << ","
                      << arrivalDeltaVX                     << ","
                      << arrivalDeltaVY                     << ","
                      << arrivalDeltaVZ                     << ","
                      << transferDeltaV
                      << std::endl;
    }

    shortlistFile.close( );
}

} // namespace d2d
