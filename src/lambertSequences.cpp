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
    
    // Create multimap of departure-arrival combinations.
    std::multimap<int, int> combinations;
    
    // Create multimap with data of each combination.
    allDatapoints lookupDeltaV;

    // Fill combintations and lookupDeltaV
    while ( lambertScannerQuery.executeStep( ) )
    {
        int departureObject     = lambertScannerQuery.getColumn( 0 );
        int arrivalObject       = lambertScannerQuery.getColumn( 1 );
        double departureEpoch   = lambertScannerQuery.getColumn( 2 );
        double timeOfFlight     = lambertScannerQuery.getColumn( 3 );
        double transferDeltaV   = lambertScannerQuery.getColumn( 4 );
        
        departureArrivalCombo combo = std::make_pair(departureObject,arrivalObject);
        combinations.insert( combo );
        
        datapoint point;
        point.push_back(departureEpoch-2457399.5);
        point.push_back(timeOfFlight);
        point.push_back(transferDeltaV);
        lookupDeltaV.insert( std::pair<departureArrivalCombo,datapoint>(combo,point));
    }

    // Make list of all departure objects. 
    std::vector<int> allDepartureObjects;
    for(    std::multimap<int,int>::iterator itAllDepartureObjects = combinations.begin();
            itAllDepartureObjects != combinations.end(); 
            ++itAllDepartureObjects)
    {
        allDepartureObjects.push_back(itAllDepartureObjects->first);
    }

    // Make list of all depature objects unique. 
    std::vector<int> uniqueDepartureObjects (allDepartureObjects.size());
    std::vector<int>::iterator itUniqueDepartureObjects;
    itUniqueDepartureObjects=std::unique_copy(allDepartureObjects.begin(),
                        allDepartureObjects.end(),
                        uniqueDepartureObjects.begin());

    uniqueDepartureObjects.resize( std::distance(   uniqueDepartureObjects.begin(),
                                                    itUniqueDepartureObjects));
    
    // Print unique departure objects.
    std::cout   << "Printing the " 
                << uniqueDepartureObjects.size() 
                << " unique departure objects:" 
                << std::endl;

    for(unsigned int i=0; i<uniqueDepartureObjects.size(); i++)
    {
        std::cout << uniqueDepartureObjects[i] << std::endl;
    }
    std::cout << "" << std::endl;
    
    // Print combinations if necessary. 
    std::cout   << "Printing the " 
                << combinations.size() 
                << " debris to debris combinations: " 
                << std::endl;
    std::multimap<int, int>::iterator currentCombination = combinations.begin();
    while(currentCombination != combinations.end())
    {
        std::cout<<"Departure object = "<<currentCombination->first<<"    Arrival object = "<<currentCombination->second<<std::endl;
        currentCombination++;
    }
    std::cout << "" << std::endl;

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

    // Output the resulting sequences
    std::ofstream shortlistFile( input.sequencesPath.c_str( ) );
    bool skip = false;
    int possibleSequences = 0;
    for (unsigned int i = 1; i < allSequences.size()+1; ++i)
    {
        std::list<int> currentSequence = allSequences[i];
        double sequenceDeltaV = 0.0;
        double sequenceTimeOfFlight = 0.0;
        double overallDepartureEpoch = 0.0;
        double previousDepartureEpoch = 0.0;
        std::list<int>::iterator itCurrentSequeceConstructor = currentSequence.begin();
        int departureObject ;
        int arrivalObject ;
        bool skip = false;
        
        for (unsigned int k = 0; k < currentSequence.size()-1; ++k)
        {               
            departureObject = *itCurrentSequeceConstructor;
            itCurrentSequeceConstructor++;
            arrivalObject = *itCurrentSequeceConstructor;
            departureArrivalCombo combo;
            combo = std::make_pair(departureObject,arrivalObject);
            
            if (k==0)
            {
                overallDepartureEpoch    = lookupDeltaV.find(combo)->second[0] ;
            }

            double currentDepartureEpoch    = lookupDeltaV.find(combo)->second[0] ;
            
            if (currentDepartureEpoch < previousDepartureEpoch + input.stayTime && k != input.sequenceLength-2)
            // if (currentDepartureEpoch < previousDepartureEpoch + input.stayTime )
            {
                skip = true;
                // std::cout << "dsajkl" << k << std::endl;
                break;
            }
            double currentTimeOfFlight      = lookupDeltaV.find(combo)->second[1] ;
            double currentDeltaV            = lookupDeltaV.find(combo)->second[2] ;
            
            sequenceTimeOfFlight    = sequenceTimeOfFlight + currentTimeOfFlight;
            sequenceDeltaV          = sequenceDeltaV + currentDeltaV;
            previousDepartureEpoch  = currentDepartureEpoch + currentTimeOfFlight/86400 ;
        }

        if (skip==false)
        {
            for (std::list<int>::iterator itSequencePrinter = currentSequence.begin(); itSequencePrinter != currentSequence.end(); itSequencePrinter++)
            {
                shortlistFile << *itSequencePrinter << ' ';
            }

            std::list<int> currentSequence = allSequences[i];
            std::list<int>::iterator itCurrentSequencePrinter = currentSequence.begin();
            for (unsigned int k = 0; k < currentSequence.size()-1; ++k)
            {               
                departureObject = *itCurrentSequencePrinter;
                itCurrentSequencePrinter++;
                arrivalObject = *itCurrentSequencePrinter;
                departureArrivalCombo combo;
                combo = std::make_pair(departureObject,arrivalObject);
                if (k==0)
                {
                    overallDepartureEpoch    = lookupDeltaV.find(combo)->second[0] ;
                }
                double currentDepartureEpoch    = lookupDeltaV.find(combo)->second[0] ;
                double currentTimeOfFlight      = lookupDeltaV.find(combo)->second[1] ;
                double currentDeltaV            = lookupDeltaV.find(combo)->second[2] ;
                shortlistFile << currentDepartureEpoch << ' ';
                shortlistFile << currentTimeOfFlight/86400 << ' ';
                shortlistFile << currentDeltaV << ' ';
            }
            shortlistFile << overallDepartureEpoch      << ' ' ;
            shortlistFile << sequenceTimeOfFlight/86400 << ' ' ;
            shortlistFile << sequenceDeltaV             << std::endl;
         
            possibleSequences++;
        }
    }
    shortlistFile.close( );

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

//! Recurse leg-by-leg to generate list of TLE sequences.
// void recurse(          const int                    currentSequencePosition,
//                        const TleObjects&            tleObjects,
//                              Sequence&              sequence,
//                              int&                   sequenceId,
//                              ListOfSequences&       listOfSequences )
void recurse(   const   int                                  currentSequencePosition,
                        std::list<int>&                      currentSequence,
                        std::multimap<int,int>&              combinations,
                        std::map<int, std::list<int> >&      allSequences,
                        int&                                 sequenceId,
                const   int&                                 maxSequenceLength )
{   
    if (currentSequencePosition==maxSequenceLength)
    {
        allSequences[sequenceId] = currentSequence;
        sequenceId++;
        currentSequence.pop_back();
        return;   
    }
    // Set last object in sequence as the object to match.
    int match =  currentSequence.back();
    
    // Find possible next objects in sequence.
    std::vector< int > options;
    for (   std::multimap<int, int>::iterator itCombinations = combinations.begin();
            itCombinations != combinations.end(); 
            itCombinations++)
    {
        if (itCombinations->first==match)
        {
            options.push_back(itCombinations->second);
        }
    }
    
    // Remove objects already in sequence from possible options.
    std::vector<int> toBeRemoved;
    int size_options = options.size();
    for (int k = 0; k < size_options; ++k)
    {
        bool found = (std::find(currentSequence.begin(), currentSequence.end(), options[k]) != currentSequence.end());
        if (found==1)
            {
                toBeRemoved.push_back(k);
            }
    }   
    
    std::reverse(toBeRemoved.begin(),toBeRemoved.end());    
    
    for (unsigned int i = 0; i < toBeRemoved.size(); ++i)
    {
        options.erase(options.begin()+toBeRemoved[i]);
    }
    
    if (options.size() == 0) 
    {
        // allSequences[sequenceId] = currentSequence;
        // sequenceId++;
        currentSequence.pop_back();
        return;
    }
    else
    {
        int tempsize = options.size();
        for (int i = 0; i < tempsize; ++i)
        {
            currentSequence.push_back(options[i]);
            recurse(    currentSequencePosition +1,
                        currentSequence,
                        combinations,
                        allSequences,
                        sequenceId,
                        maxSequenceLength);
        }
    }
    currentSequence.pop_back();
}

//! Check lambert_scanner input parameters.
LambertSequencesInput checkLambertSequencesInput( const rapidjson::Document& config )
{
    const std::string databasePath = find( config, "database_path" )->value.GetString( );
    std::cout << "Database path                 " << databasePath << std::endl;

    const int sequenceLength       = find( config, "sequence_length" )->value.GetInt( );
    std::cout << "Sequence length               " << sequenceLength << std::endl;

    const int stayTime             = find( config, "stay_time" )->value.GetInt( );
    std::cout << "Stay time                     " << stayTime << std::endl;

    const std::string sequencesPath = find( config, "sequences_path" )->value.GetString( );
    std::cout << "Sequences path                 " << sequencesPath << std::endl;

    return LambertSequencesInput(   databasePath,
                                    sequenceLength,
                                    stayTime,
                                    sequencesPath );
}
                                // stayTime,
//                                 // sequenceLength,

// //! Create lambert_sequences tables.
// void createLambertSequencesTables( SQLite::Database& database, const int sequenceLength )
// {
//     // Drop table from database if it exists.
//     database.exec( "DROP TABLE IF EXISTS sequences;" );
//     database.exec( "DROP TABLE IF EXISTS lambert_scanner_transfers;" );
//     database.exec( "DROP TABLE IF EXISTS lambert_scanner_multi_leg_transfers;" );

//     // Set up SQL command to create table to store lambert_sequences.
//     std::ostringstream lambertScannerSequencesTableCreate;
//     lambertScannerSequencesTableCreate
//         << "CREATE TABLE sequences ("
//         << "\"sequence_id\"                               INTEGER PRIMARY KEY              ,";
//     for ( int i = 0; i < sequenceLength; ++i )
//     {
//         lambertScannerSequencesTableCreate
//             << "\"target_" << i << "\"                    INTEGER                          ,";
//     }
//     for ( int i = 0; i < sequenceLength - 1; ++i )
//     {
//         lambertScannerSequencesTableCreate
//             << "\"transfer_id_" << i + 1 << "\"           INTEGER                          ,";
//     }
//     lambertScannerSequencesTableCreate
//         << "\"launch_epoch\"                              REAL                             ,"
//         << "\"lambert_transfer_delta_v\"                  REAL                             ,"
//         << "\"mission_duration\"                          REAL                             ,"
//         << "\"atom_transfer_delta_v\"                     REAL                             );";

//     // // Execute command to create table.
//     database.exec( lambertScannerSequencesTableCreate.str( ).c_str( ) );

//     if ( !database.tableExists( "sequences" ) )
//     {
//         throw std::runtime_error( "ERROR: Creating table 'sequences' failed!" );
//     }

//     // Set up SQL command to create table to store lambert_scanner transfers.
//     std::ostringstream lambertScannerTransfersTableCreate;
//     lambertScannerTransfersTableCreate
//         << "CREATE TABLE lambert_scanner_transfers ("
//         << "\"transfer_id\"                             INTEGER PRIMARY KEY,"
//         << "\"departure_object_id\"                     INTEGER,"
//         << "\"arrival_object_id\"                       INTEGER,"
//         << "\"departure_epoch\"                         REAL,"
//         << "\"time_of_flight\"                          REAL,"
//         << "\"revolutions\"                             INTEGER,"
//         // N.B.: SQLite doesn't support booleans so 0 = false, 1 = true for 'prograde'
//         << "\"prograde\"                                INTEGER,"
//         << "\"departure_position_x\"                    REAL,"
//         << "\"departure_position_y\"                    REAL,"
//         << "\"departure_position_z\"                    REAL,"
//         << "\"departure_velocity_x\"                    REAL,"
//         << "\"departure_velocity_y\"                    REAL,"
//         << "\"departure_velocity_z\"                    REAL,"
//         << "\"departure_semi_major_axis\"               REAL,"
//         << "\"departure_eccentricity\"                  REAL,"
//         << "\"departure_inclination\"                   REAL,"
//         << "\"departure_argument_of_periapsis\"         REAL,"
//         << "\"departure_longitude_of_ascending_node\"   REAL,"
//         << "\"departure_true_anomaly\"                  REAL,"
//         << "\"arrival_position_x\"                      REAL,"
//         << "\"arrival_position_y\"                      REAL,"
//         << "\"arrival_position_z\"                      REAL,"
//         << "\"arrival_velocity_x\"                      REAL,"
//         << "\"arrival_velocity_y\"                      REAL,"
//         << "\"arrival_velocity_z\"                      REAL,"
//         << "\"arrival_semi_major_axis\"                 REAL,"
//         << "\"arrival_eccentricity\"                    REAL,"
//         << "\"arrival_inclination\"                     REAL,"
//         << "\"arrival_argument_of_periapsis\"           REAL,"
//         << "\"arrival_longitude_of_ascending_node\"     REAL,"
//         << "\"arrival_true_anomaly\"                    REAL,"
//         << "\"transfer_semi_major_axis\"                REAL,"
//         << "\"transfer_eccentricity\"                   REAL,"
//         << "\"transfer_inclination\"                    REAL,"
//         << "\"transfer_argument_of_periapsis\"          REAL,"
//         << "\"transfer_longitude_of_ascending_node\"    REAL,"
//         << "\"transfer_true_anomaly\"                   REAL,"
//         << "\"departure_delta_v_x\"                     REAL,"
//         << "\"departure_delta_v_y\"                     REAL,"
//         << "\"departure_delta_v_z\"                     REAL,"
//         << "\"arrival_delta_v_x\"                       REAL,"
//         << "\"arrival_delta_v_y\"                       REAL,"
//         << "\"arrival_delta_v_z\"                       REAL,"
//         << "\"transfer_delta_v\"                        REAL,"
//         << "\"leg_id\"                                  INTEGER"
//         <<                                              ");";

//     // Execute command to create table.
//     database.exec( lambertScannerTransfersTableCreate.str( ).c_str( ) );

//     // Execute command to create index on transfer Delta-V column.
//     std::ostringstream transferDeltaVIndexCreate;
//     transferDeltaVIndexCreate << "CREATE INDEX IF NOT EXISTS \"transfer_delta_v\" on "
//                               << "lambert_scanner_transfers (transfer_delta_v ASC);";
//     database.exec( transferDeltaVIndexCreate.str( ).c_str( ) );

//     if ( !database.tableExists( "lambert_scanner_transfers" ) )
//     {
//         throw std::runtime_error( "ERROR: Creating table 'lambert_scanner_transfers' failed!" );
//     }

//     // Set up SQL command to create table to store lambert_scanner multi-leg transfers.
//     std::ostringstream lambertScannerMultiLegTransfersTableCreate;
//     lambertScannerMultiLegTransfersTableCreate
//         << "CREATE TABLE lambert_scanner_multi_leg_transfers ("
//         << "\"multi_leg_transfer_id\"                   INTEGER PRIMARY KEY AUTOINCREMENT,"
//         << "\"sequence_id\"                             INTEGER,";
//     for ( int i = 0; i < sequenceLength - 1; ++i )
//     {
//         lambertScannerMultiLegTransfersTableCreate
//             << "\"transfer_id_" << i + 1 << "\"         INTEGER,";
//     }
//     lambertScannerMultiLegTransfersTableCreate
//         << "\"launch_epoch\"                            REAL,"
//         << "\"mission_duration\"                        REAL,"
//         << "\"total_transfer_delta_v\"                  REAL"
//         <<                                              ");";

//     // Execute command to create table.
//     database.exec( lambertScannerMultiLegTransfersTableCreate.str( ).c_str( ) );

//     // Execute command to create index on total transfer Delta-V column.
//     std::ostringstream totalTransferDeltaVIndexCreate;
//     totalTransferDeltaVIndexCreate << "CREATE INDEX IF NOT EXISTS \"total_transfer_delta_v\" on "
//                                    << "lambert_scanner_multi_leg_transfers "
//                                    << "(total_transfer_delta_v ASC);";
//     database.exec( totalTransferDeltaVIndexCreate.str( ).c_str( ) );

//     if ( !database.tableExists( "lambert_scanner_multi_leg_transfers" ) )
//     {
//         throw std::runtime_error(
//             "ERROR: Creating table 'lambert_scanner_multi_leg_transfers' failed!" );
//     }
// }

// //! Write best multi-leg Lambert transfers for each sequence to file.
// void writeSequencesToFile( SQLite::Database&    database,
//                            const std::string&   sequencesPath,
//                            const int            sequenceLength  )
// {
//     // Fetch sequences tables from database and sort from lowest to highest Delta-V.
//     std::ostringstream sequencesSelect;
//     sequencesSelect << "SELECT * FROM sequences ORDER BY lambert_transfer_delta_v ASC;";
//     SQLite::Statement query( database, sequencesSelect.str( ) );

//     // Write sequences to file.
//     std::ofstream sequencesFile( sequencesPath.c_str( ) );

//     // Print file header.
//     sequencesFile << "sequence_id,";
//     for ( int i = 0; i < sequenceLength; ++i )
//     {
//         sequencesFile << "target_" << i << ",";
//     }
//     for ( int i = 0; i < sequenceLength - 1; ++i )
//     {
//         sequencesFile << "transfer_id_" << i + 1 << ",";
//     }
//     sequencesFile << "launch_epoch,"
//                   << "lambert_transfer_delta_v,"
//                   << "mission_duration"
//                   << std::endl;

//     // Loop through data retrieved from database and write to file.
//     while( query.executeStep( ) )
//     {
//         const int       sequenceId                     = query.getColumn( 0 );
//         std::vector< int > targets( sequenceLength );
//         for ( unsigned int i = 0; i < targets.size( ); ++i )
//         {
//             targets[ i ]                               = query.getColumn( i + 1 );
//         }
//         std::vector< int > transferIds( sequenceLength - 1 );
//         for ( unsigned int i = 0; i < transferIds.size( ); ++i )
//         {
//             transferIds[ i ]                           = query.getColumn( i + sequenceLength + 1 );
//         }
//         const double    launchEpoch                    = query.getColumn( 2 * sequenceLength );
//         const double    lambertTransferDeltaV          = query.getColumn( 2 * sequenceLength + 1 );
//         const double    missionDuration                = query.getColumn( 2 * sequenceLength + 2 );

//         sequencesFile << sequenceId                    << ",";
//         for ( unsigned int i = 0; i < targets.size( ); ++i )
//         {
//             sequencesFile << targets[ i ]              << ",";
//         }
//         for ( unsigned int i = 0; i < transferIds.size( ); ++i )
//         {
//             sequencesFile << transferIds[ i ]          << ",";
//         }
//         sequencesFile << launchEpoch                   << ","
//                       << lambertTransferDeltaV         << ","
//                       << missionDuration
//                       << std::endl;
//     }

//     sequencesFile.close( );
// }

} // namespace d2d
