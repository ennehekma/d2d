/*
 * Copyright (c) 2016-2017 Enne Hekma, Delft University of Technology (ennehekma@gmail.com)
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
#include <string>
#include <list>

#include <sqlite3.h>

#include "D2D/lambertSequences.hpp"
#include "D2D/tools.hpp"

namespace d2d
{
//! Execute lambert_sequences.
void executeLambertSequences( const rapidjson::Document& config )
{
    // Verify config parameters. Exception is thrown if any of the parameters are missing.
    const LambertSequencesInput input = checkLambertSequencesInput( config );

    std::cout << std::endl;
    std::cout << "******************************************************************" << std::endl;
    std::cout << "                       Simulation & Output                        " << std::endl;
    std::cout << "******************************************************************" << std::endl;
    std::cout << std::endl;

    // Open database, setup strings to select data, adn open database connection. 
    SQLite::Database database( input.databasePath.c_str( ), SQLITE_OPEN_READWRITE );

    // Create table in database for this sequence length.    
    createLambertSequencesTable( database, input.sequenceLength );
    
    // Create multimap of departure-arrival combinations.
    std::multimap< int, int > combinations;
    
    // Create multimap with data of each combination.
    allDatapoints lookupDeltaV;

    std::ostringstream lambertScannerTableSelect;
    lambertScannerTableSelect   << "SELECT  departure_object_id, "
                                <<        " arrival_object_id, "
                                <<        " departure_epoch, "
                                <<        " time_of_flight, "
                                <<        " min(transfer_delta_v)   "
                                << "FROM lambert_scanner_zoom_results  "
                                << "GROUP BY departure_object_id, arrival_object_id  "
                                << "ORDER BY departure_epoch ASC "
                                << ";";
    SQLite::Statement lambertScannerQuery( database, lambertScannerTableSelect.str( ) );   

    // Fill combintations and lookupDeltaV
    while ( lambertScannerQuery.executeStep( ) )
    {
        int departureObject     = lambertScannerQuery.getColumn( 0 );
        int arrivalObject       = lambertScannerQuery.getColumn( 1 );
        double departureEpoch   = lambertScannerQuery.getColumn( 2 );
        double timeOfFlight     = lambertScannerQuery.getColumn( 3 );
        double transferDeltaV   = lambertScannerQuery.getColumn( 4 );
        
        departureArrivalCombo combo = std::make_pair( departureObject, arrivalObject );
        combinations.insert( combo );
        
        datapoint point;
        point.push_back( departureEpoch - 2457400 );
        point.push_back( timeOfFlight );
        point.push_back( transferDeltaV );
        lookupDeltaV.insert( std::pair< departureArrivalCombo, datapoint >( combo, point ) );
    }
    
    // Make list of all departure objects and one of all objects. 
    std::vector< int > allDepartureObjects;
    std::vector< int > allObjects;

    for( std::multimap< int, int >::iterator itAllDepartureObjects = combinations.begin( );
         itAllDepartureObjects != combinations.end( ); 
         ++itAllDepartureObjects )
    {
        allDepartureObjects.push_back( itAllDepartureObjects->first );
        allObjects.push_back( itAllDepartureObjects->first );
        allObjects.push_back( itAllDepartureObjects->second );
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

    uniqueObjects.resize( std::distance( uniqueObjects.begin( ), itUniqueObjects ) );

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

    // Setup inputs for recursive function.
    int currentSequencePosition = 1;
    std::map<int, std::list<int> > allSequences;
    int sequenceId = 1;
    for (unsigned int z = 0; z < uniqueDepartureObjects.size(); ++z)
    {
        int currentFirstObject[] = {uniqueDepartureObjects[z]};
        std::list<int> currentSequence (currentFirstObject, 
                                currentFirstObject + sizeof(currentFirstObject) / sizeof(int) );
        recurse(    currentSequencePosition,
                    currentSequence,
                    combinations,
                    allSequences,
                    sequenceId,
                    input.sequenceLength );
    }

    bool skip = false;
    int possibleSequences = 0;
    for ( unsigned int sequenceiterator = 1; 
          sequenceiterator < allSequences.size( ) + 1; 
          ++sequenceiterator )
    {
        std::list< int > currentSequence = allSequences[ sequenceiterator ];
        double sequenceDeltaV = 0.0;
        double sequenceTimeOfFlight = 0.0;
        double overallDepartureEpoch = 0.0;
        double previousDepartureEpoch = 0.0;
        std::list< int >::iterator itCurrentSequecePositionConstructor = currentSequence.begin( );
        int departureObject;
        int arrivalObject;
        bool skip = false;
        
        for ( unsigned int currentPosition = 0; 
              currentPosition < currentSequence.size() - 1; 
              ++currentPosition )
        {               
            departureObject = *itCurrentSequecePositionConstructor;
            itCurrentSequecePositionConstructor++;
            arrivalObject = *itCurrentSequecePositionConstructor;
            departureArrivalCombo combo;
            combo = std::make_pair( departureObject, arrivalObject );
                
                if ( currentPosition == 0 )
                {
                    overallDepartureEpoch = lookupDeltaV.find( combo )->second[ 0 ];
                }
                double currentDepartureEpoch = lookupDeltaV.find( combo )->second[ 0 ];
                
                // if (currentDepartureEpoch < previousDepartureEpoch + input.stayTime && k != input.sequenceLength-2)
                if ( currentDepartureEpoch < previousDepartureEpoch + input.stayTime )
                {
                    skip = true;
                    break;
                }
                double currentTimeOfFlight = lookupDeltaV.find( combo )->second[ 1 ];
                double currentDeltaV = lookupDeltaV.find( combo )->second[ 2 ];
                
                sequenceTimeOfFlight    = sequenceTimeOfFlight + currentTimeOfFlight;
                sequenceDeltaV          = sequenceDeltaV + currentDeltaV;
                previousDepartureEpoch  = currentDepartureEpoch + currentTimeOfFlight/86400;
        }
        if (skip==false)
        {   
            std::ostringstream lambertSequencesTableInsert;
            lambertSequencesTableInsert << "INSERT INTO lambert_zoom_sequences_"
                                        << input.sequenceLength
                                        << " VALUES ("
                                        << "NULL,";
            
            for ( std::list< int >::iterator itSequencePrinter = currentSequence.begin( ); 
                  itSequencePrinter != currentSequence.end( ); 
                  itSequencePrinter++ )
            {   
                lambertSequencesTableInsert << "\"" << *itSequencePrinter << "\",";
            }
            
            double totalRemovedCrossSection = 0.0;
            for ( std::list< int >::iterator itSequenceAreaPrinter = currentSequence.begin( );
                  itSequenceAreaPrinter != currentSequence.end( );
                  itSequenceAreaPrinter++ )
            {
                double currentRemovedCrossSection = 
                    allCrossSections.find( *itSequenceAreaPrinter )->second;
                totalRemovedCrossSection = currentRemovedCrossSection +  totalRemovedCrossSection;
                lambertSequencesTableInsert << "\"" << currentRemovedCrossSection << "\",";
            }

            std::list< int > currentSequence = allSequences[ sequenceiterator ];
            std::list< int >::iterator itCurrentSequencePositionPrinter = currentSequence.begin( );
            for ( unsigned int k = 0; k < currentSequence.size( ) - 1; ++k)
            {               
                departureObject = *itCurrentSequencePositionPrinter;
                itCurrentSequencePositionPrinter++;
                arrivalObject = *itCurrentSequencePositionPrinter;
                departureArrivalCombo combo;
                combo = std::make_pair( departureObject, arrivalObject );
                if ( k == 0 )
                {
                    overallDepartureEpoch = lookupDeltaV.find( combo )->second[ 0 ];
                }
                double currentDepartureEpoch    = lookupDeltaV.find( combo )->second[ 0 ];
                double currentTimeOfFlight      = lookupDeltaV.find( combo )->second[ 1 ];
                double currentDeltaV            = lookupDeltaV.find( combo )->second[ 2 ];
                lambertSequencesTableInsert << "\"" << currentDepartureEpoch  << "\",";
                lambertSequencesTableInsert << "\"" << currentTimeOfFlight/86400  << "\",";
                lambertSequencesTableInsert << "\"" << currentDeltaV  << "\",";
            }
            lambertSequencesTableInsert << "\"" << overallDepartureEpoch  << "\",";
            lambertSequencesTableInsert << "\"" << sequenceTimeOfFlight/86400  << "\",";
            lambertSequencesTableInsert << "\"" << sequenceDeltaV  << "\",";
            lambertSequencesTableInsert << "\"" << totalRemovedCrossSection    << "\");";    
            
            database.exec( lambertSequencesTableInsert.str( ).c_str( ) );
            
            possibleSequences++;
 
        }
    }

    // Output the resulting sequences
    std::cout   << "Out of "
                << allSequences.size() 
                << " sequences of "
                << input.sequenceLength       
                << " objects found, "
                << possibleSequences 
                << " are feasible with a staytime of "
                << input.stayTime
                << " seconds."
                << std::endl;
    std::cout << "" << std::endl;
}


void recurse(   const   int                                  currentSequencePosition,
                        std::list< int >&                    currentSequence,
                        std::multimap< int, int >&           combinations,
                        std::map< int, std::list< int > >&   allSequences,
                        int&                                 sequenceId,
                const   int&                                 maxSequenceLength )
{   
    if ( currentSequencePosition == maxSequenceLength )
    {
        allSequences[ sequenceId ] = currentSequence;
        sequenceId++;
        currentSequence.pop_back( );
        return;   
    }

    // Set last object in sequence as the object to match.
    int match =  currentSequence.back( );
    
    // Find possible next objects in sequence.
    std::vector< int > options;
    for ( std::multimap< int, int >::iterator itCombinations = combinations.begin( );
          itCombinations != combinations.end( ); 
          itCombinations++)
    {
        if ( itCombinations->first == match )
        {
            options.push_back( itCombinations->second );
        }
    }
    
    // Remove objects already in sequence from possible options.
    std::vector< int > toBeRemoved;
    int size_options = options.size( );
    for (int k = 0; k < size_options; ++k)
    {
        bool found = ( std::find( currentSequence.begin( ), currentSequence.end( ), options[ k ] ) 
                        != currentSequence.end( ) );
        if (found==1)
            {
                toBeRemoved.push_back( k );
            }
    }   

    std::reverse( toBeRemoved.begin( ), toBeRemoved.end( ) );    
    
    for ( unsigned int i = 0; i < toBeRemoved.size( ); ++i)
    {
        options.erase( options.begin( ) + toBeRemoved[ i ]);
    }
    
    if ( options.size( ) == 0 ) 
    {
        currentSequence.pop_back( );
        return;
    }
    else
    {
        int tempsize = options.size( );
        for ( int i = 0; i < tempsize; ++i )
        {
            currentSequence.push_back( options[ i ] );
            recurse( currentSequencePosition + 1,
                     currentSequence,
                     combinations,
                     allSequences,
                     sequenceId,
                     maxSequenceLength);
        }
    }
    currentSequence.pop_back( );
}

//! Check lambert_scanner input parameters.
LambertSequencesInput checkLambertSequencesInput( const rapidjson::Document& config )
{
    const std::string databasePath  = find( config, "database_path" )->value.GetString( );
    std::cout << "Database path                 " << databasePath   << std::endl;

    const int sequenceLength        = find( config, "sequence_length" )->value.GetInt( );
    std::cout << "Sequence length               " << sequenceLength << std::endl;

    const int stayTime              = find( config, "stay_time" )->value.GetInt( );
    std::cout << "Stay time                     " << stayTime       << std::endl;

    const std::string satcatPath    = find( config, "satcat_path" )->value.GetString( );
    std::cout << "Satcat path                   " << satcatPath     << std::endl;

    const std::string sequencesPath = find( config, "sequences_path" )->value.GetString( );
    std::cout << "Sequences path                " << sequencesPath  << std::endl;

    return LambertSequencesInput( databasePath,
                                  sequenceLength,
                                  stayTime,
                                  satcatPath,
                                  sequencesPath );
}
     
//! Create lambert_sequences tables.
void createLambertSequencesTable( SQLite::Database& database, const int sequenceLength )
{
    // Drop table from database if it exists.
    std::ostringstream lambertZoomSequencesTableCheck;
    lambertZoomSequencesTableCheck << "DROP TABLE IF EXISTS lambert_zoom_sequences_"
                                   << sequenceLength
                                   << ";";
    database.exec( lambertZoomSequencesTableCheck.str( ).c_str( ) );

    // Set up SQL command to create table to store lambert_sequences.
    std::ostringstream lambertZoomSequencesTableCreate;
    lambertZoomSequencesTableCreate 
        << "CREATE TABLE lambert_zoom_sequences_"
        << sequenceLength
        << " ("
        << "\"sequence_id\"                                INTEGER PRIMARY KEY              ,";
    for ( int i = 1; i < sequenceLength + 1; ++i )
    {
        lambertZoomSequencesTableCreate
            << "\"object_" << i << "\"                     INTEGER                          ,";
    }
    for ( int i = 1; i < sequenceLength + 1; ++i )
    {
        lambertZoomSequencesTableCreate
            << "\"object_" << i << "_area\"                REAL                             ,";
    }
    for (int i = 1; i < sequenceLength; ++i)
    {
        lambertZoomSequencesTableCreate
            << "\"departure_epoch_" << i    << "\"         INTEGER                          ,"
            << "\"time_of_flight_"  << i    << "\"         REAL                             ,"
            << "\"delta_v_"         << i    << "\"         REAL                             ,";
    }
    lambertZoomSequencesTableCreate
        << "\"overall_departure_epoch\"                    REAL                             ,"
        << "\"cumulative_time_of_flight\"                  REAL                             ,"
        << "\"total_delta_v\"                              REAL                             ,"
        << "\"removed_area\"                               REAL                             );";

    // // Execute command to create table.
    database.exec( lambertZoomSequencesTableCreate.str( ).c_str( ) );
    std::ostringstream lambertZoomSequencesTableExists;
    lambertZoomSequencesTableExists << "lambert_zoom_sequences_" << sequenceLength;

    if ( !database.tableExists( lambertZoomSequencesTableExists.str( ).c_str( ) ) )
    {
        throw std::runtime_error( "ERROR: Creating table 'lambert_zoom_sequences_*' failed!" );
    }
}

//! Write best multi-leg Lambert transfers for each sequence to file.
void writeSequencesToFile( SQLite::Database&    database,
                           const std::string&   sequencesPath,
                           const int            sequenceLength  )
{


        // if (skip==false)
        // {
        //     for (std::list<int>::iterator itSequencePrinter = currentSequence.begin(); itSequencePrinter != currentSequence.end(); itSequencePrinter++)
        //     {
        //         shortlistFile << *itSequencePrinter << ' ';
        //     }
        //     double totalRemovedCrossSection = 0.0;
        //     for (std::list<int>::iterator itSequenceAreaPrinter = currentSequence.begin(); itSequenceAreaPrinter != currentSequence.end(); itSequenceAreaPrinter++)
        //     {
        //         double currentRemovedCrossSection = allCrossSections.find(*itSequenceAreaPrinter)->second;
        //         totalRemovedCrossSection = currentRemovedCrossSection +  totalRemovedCrossSection;
        //         shortlistFile << currentRemovedCrossSection << ' ';
        //     }

        //     std::list<int> currentSequence = allSequences[i];
        //     std::list<int>::iterator itCurrentSequencePositionPrinter = currentSequence.begin();
        //     for (unsigned int k = 0; k < currentSequence.size()-1; ++k)
        //     {               
        //         departureObject = *itCurrentSequencePositionPrinter;
        //         itCurrentSequencePositionPrinter++;
        //         arrivalObject = *itCurrentSequencePositionPrinter;
        //         departureArrivalCombo combo;
        //         combo = std::make_pair(departureObject,arrivalObject);
        //         if (k==0)
        //         {
        //             overallDepartureEpoch    = lookupDeltaV.find(combo)->second[0] ;
        //         }
        //         double currentDepartureEpoch    = lookupDeltaV.find(combo)->second[0] ;
        //         double currentTimeOfFlight      = lookupDeltaV.find(combo)->second[1] ;
        //         double currentDeltaV            = lookupDeltaV.find(combo)->second[2] ;
        //         shortlistFile << currentDepartureEpoch << ' ';
        //         shortlistFile << currentTimeOfFlight/86400 << ' ';
        //         shortlistFile << currentDeltaV << ' ';
        //     }
        //     shortlistFile << overallDepartureEpoch      << ' ' ;
        //     shortlistFile << sequenceTimeOfFlight/86400 << ' ' ;
        //     shortlistFile << sequenceDeltaV             << ' ' ;
        //     shortlistFile << totalRemovedCrossSection   << std::endl;
         
        //     possibleSequences++;
        // }



    // lambertSequencesTableInsert 
    //     << "INSERT INTO lambert_zoom_sequences_"
    //     << input.sequenceLength
    //     << " VALUES ("
    //     << "NULL,";
    // // Print header shortlist file
    // for (int i = 1; i < input.sequenceLength+1; ++i)
    // {
    //     lambertSequencesTableInsert << ":object_" << i << ", ";
    // }
    // for (int i = 1; i < input.sequenceLength+1; ++i)
    // {
    //     lambertSequencesTableInsert << ":object_" << i << "area, " ;
    // }
    // for (int i = 1; i < input.sequenceLength; ++i)
    // {
    //     lambertSequencesTableInsert << ":departure_epoch_" << i << ", " 
    //                                 << ":time_of_flight_" << i << ", " 
    //                                 << ":delta_v_" << i << ", ";
    // }
    //     lambertSequencesTableInsert << ":overall_departure_epoch" << ", " 
    //                                 << ":cumulative_time_of_flight" << ", " 
    //                                 << ":total_delta_v" << ", " 
    //                                 << ":removed_area" 
    //                                 << ");";




    // std::ofstream shortlistFile( input.sequencesPath.c_str( ) );

    // // Print header shortlist file
    // for (int i = 1; i < input.sequenceLength+1; ++i)
    // {
    //     shortlistFile << "object_" << i << " ";
    // }
    // for (int i = 1; i < input.sequenceLength+1; ++i)
    // {
    //     shortlistFile << "object_" << i << "area " ;
    // }
    // for (int i = 1; i < input.sequenceLength; ++i)
    // {
    //     shortlistFile << "departure_epoch_" << i << " " 
    //                   << "time_of_flight_" << i << " " 
    //                   << "delta_v_" << i << " ";
    // }
    //     shortlistFile << "overall_departure_epoch" << " " 
    //                   << "cumulative_time_of_flight" << " " 
    //                   << "total_delta_v" << " " 
    //                   << "removed_area" << std::endl;






    // Fetch sequences tables from database and sort from lowest to highest Delta-V.
    std::ostringstream sequencesSelect;
    sequencesSelect << "SELECT * FROM sequences ORDER BY lambert_transfer_delta_v ASC;";
    SQLite::Statement query( database, sequencesSelect.str( ) );

    // Write sequences to file.
    std::ofstream sequencesFile( sequencesPath.c_str( ) );

    // Print file header.
    sequencesFile << "sequence_id,";
    for ( int i = 0; i < sequenceLength; ++i )
    {
        sequencesFile << "target_" << i << ",";
    }
    for ( int i = 0; i < sequenceLength - 1; ++i )
    {
        sequencesFile << "transfer_id_" << i + 1 << ",";
    }
    sequencesFile << "launch_epoch,"
                  << "lambert_transfer_delta_v,"
                  << "mission_duration"
                  << std::endl;

    // Loop through data retrieved from database and write to file.
    while( query.executeStep( ) )
    {
        const int       sequenceId                     = query.getColumn( 0 );
        std::vector< int > targets( sequenceLength );
        for ( unsigned int i = 0; i < targets.size( ); ++i )
        {
            targets[ i ]                               = query.getColumn( i + 1 );
        }
        std::vector< int > transferIds( sequenceLength - 1 );
        for ( unsigned int i = 0; i < transferIds.size( ); ++i )
        {
            transferIds[ i ]                           = query.getColumn( i + sequenceLength + 1 );
        }
        const double    launchEpoch                    = query.getColumn( 2 * sequenceLength );
        const double    lambertTransferDeltaV          = query.getColumn( 2 * sequenceLength + 1 );
        const double    missionDuration                = query.getColumn( 2 * sequenceLength + 2 );

        sequencesFile << sequenceId                    << ",";
        for ( unsigned int i = 0; i < targets.size( ); ++i )
        {
            sequencesFile << targets[ i ]              << ",";
        }
        for ( unsigned int i = 0; i < transferIds.size( ); ++i )
        {
            sequencesFile << transferIds[ i ]          << ",";
        }
        sequencesFile << launchEpoch                   << ","
                      << lambertTransferDeltaV         << ","
                      << missionDuration
                      << std::endl;
    }

    sequencesFile.close( );
}

} // namespace d2d
