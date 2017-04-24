/*
 * Copyright (c) 2014-2016 Kartik Kumar, Dinamica Srl (me@kartikkumar.com)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */

#ifndef D2D_LAMBERT_SEQUENCES_HPP
#define D2D_LAMBERT_SEQUENCES_HPP

#include <string>

#include <keplerian_toolbox.h>

#include <libsgp4/DateTime.h>

#include <rapidjson/document.h>

#include <SQLiteCpp/SQLiteCpp.h>

namespace d2d
{

struct LambertPorkChopPlotGridPoint;
//! Typedef for datapoint including departure epoch, tof and transfer Delta V
typedef std::vector< double > datapoint;


//! Typedef for a pair consisting of a departure and arrival object.
typedef std::pair< int, int > departureArrivalCombo;

//! Typedef for list of datapoints
typedef std::vector< LambertPorkChopPlotGridPoint > vectorOfDatapoints;
typedef std::list< LambertPorkChopPlotGridPoint > listOfDatapoints;

//! Typedef for multimap of containing the datapoints as values and the combo as key.
typedef std::multimap< departureArrivalCombo, datapoint > allDatapointsOld;
typedef std::map< departureArrivalCombo, listOfDatapoints > mapOflistsofdatapoints;

//! 3-Vector.
typedef boost::array< double, 3 > Vector3;

//! 6-Vector.
typedef boost::array< double, 6 > Vector6;

//! State history.
typedef std::map< double, Vector6 > StateHistory;

//! JSON config iterator.
typedef rapidjson::Value::ConstMemberIterator ConfigIterator;


//! Execute lambert_sequences.
/*!
 * Executes lambert_sequences application mode that performs a grid search to compute \f$\Delta V\f$
 * for debris-to-debris transfers. The transfers are modelled as conic sections. The Lambert
 * targeter employed is based on Izzo (2014), implemented in PyKEP (Izzo, 2012).
 *
 * The results obtained from the grid search are stored in a SQLite database, containing the
 * following table:
 *
 *	- "lambert_sequences_results": contains all Lambert transfers computed during grid search
 *
 * @param[in] config User-defined configuration options (extracted from JSON input file)
 */
void executeLambertSequences( const rapidjson::Document& config );

//! Input for lambert_sequences application mode.
/*!
 * Data struct containing all valid lambert_sequences input parameters. This struct is populated by
 * the checkLambertSequencesInput() function and can be used to execute the lambert_sequences
 * application mode.
 *
 * @sa checkLambertSequencesInput, executeLambertSequences
 */
struct LambertSequencesInput
{
public:

    //! Construct data struct.
    /*!
     * Constructs data struct based on verified input parameters.
     *
     * @sa checkLambertSequencesInput, executeLambertSequences
     * @param[in] aCatalogPath             Path to TLE catalog
     * @param[in] aDatabasePath            Path to SQLite database
     * @param[in] aDepartureEpochInitial   Departure epoch grid initial epoch
     * @param[in] someDepartureEpochSteps  Number of steps to take in departure epoch grid
     * @param[in] aDepartureEpochStepSize  Departure epoch grid step size (derived parameter) [s]
     * @param[in] aTimeOfFlightMinimum     Minimum time-of-flight [s]
     * @param[in] aTimeOfFlightMaximum     Maximum time-of-flight [s]
     * @param[in] someTimeOfFlightSteps    Number of steps to take in time-of-flight grid
     * @param[in] aTimeOfFlightStepSize    Time-of-flight step size (derived parameter) [s]
     * @param[in] progradeFlag             Flag indicating if prograde transfer should be computed
     *                                     (false = retrograde)
     * @param[in] aRevolutionsMaximum      Maximum number of revolutions
     * @param[in] aShortlistLength         Number of transfers to include in shortlist
     * @param[in] aShortlistPath           Path to shortlist file
     */
    LambertSequencesInput(  const std::string&  aDatabasePath,
                            const int&          aSequenceLength,
                            const int&          aStayTime,
                            const std::string&  aSatcatPath,
                            const std::string&  aSequencesPath )
        : databasePath( aDatabasePath ),
          sequenceLength( aSequenceLength),
          stayTime( aStayTime ),
          satcatPath(  aSatcatPath ),
          sequencesPath( aSequencesPath )
    { }

    //! Path to SQLite database to store output.
    const std::string databasePath;

    //! Number of sequences.
    const int sequenceLength;

    //! Stay time
    const int stayTime;

    //! Path to satcat file
    const std::string  satcatPath;

    //! Path to sequences file.
    const std::string sequencesPath;

protected:

private:
};

//! Check lambert_sequences input parameters.
/*!
 * Checks that all inputs for the lambert_sequences application mode are valid. If not, an error is
 * thrown with a short description of the problem. If all inputs are valid, a data struct
 * containing all the inputs is returned, which is subsequently used to execute lambert_sequences
 * and related functions.
 *
 * @sa executeLambertSequences, LambertSequencesInput
 * @param[in] config User-defined configuration options (extracted from JSON input file)
 * @return           Struct containing all valid input to execute lambert_sequences
 */
LambertSequencesInput checkLambertSequencesInput( const rapidjson::Document& config );

//! Create lambert_sequences table.
/*!
 * Creates lambert_sequences table in SQLite database used to store results obtaned from running
 * the lambert_sequences application mode.
 *
 * @sa executeLambertSequences
 * @param[in] database SQLite database handle
 */
void createLambertSequencesTable( SQLite::Database& database, int sequenceLength );

//! Write transfer shortlist to file.
/*!
 * Writes shortlist of debris-to-debris Lambert transfers to file. The shortlist is based on the
 * requested number of transfers with the lowest transfer \f$\Delta V\f$, retrieved by sorting the
 * transfers in the SQLite database.
 *
 * @sa executeLambertSequences, createLambertSequencesTable
 * @param[in] database        SQLite database handle
 * @param[in] shortlistNumber Number of entries to include in shortlist (if it exceeds number of
 *                            entries in database table, the whole table is written to file)
 * @param[in] shortlistPath   Path to shortlist file
 */
void writeTransferShortlist( SQLite::Database& database,
                             const int shortlistNumber,
                             const std::string& shortlistPath );

//! Recursively constrcut sequences.
/*!
 * Recursively constrcut sequences.
 * @sa
 * @param[in]
 * @param[in]
 * @param[in]
 */
void recurse(   const int currentSequencePosition, 
                std::list<int>& currentSequence, 
                std::multimap<int,int>& combinations, 
                std::map<int, std::list<int> >& allSequences,
                int& sequenceId,
                const int&);


void recurseAll(    std::list< int >     currentSequence,
                    int sequenceLength,
                    std::list< int >::iterator& itCurrentSequencePositionConstructor,
                    int level,
                    std::vector<LambertPorkChopPlotGridPoint>&   sequenceNow,
                    std::vector< std::vector< LambertPorkChopPlotGridPoint > >& vectorOfSequencesNow,
                    mapOflistsofdatapoints       allDatapointsRecurse);


struct LambertPorkChopPlotGridPoint
{
public:

    //! Construct data struct.
    /*!
     * Constructs data struct based on departure epoch, time-of-flight and transfer data for grid
     * point in pork-chop plot.
     *
     * @param[in] aTransferId           A unique transfer ID
     * @param[in] aDepartureEpoch       A departure epoch corresponding to a grid point
     * @param[in] anArrivalEpoch        An arrival epoch corresponding to a grid point
     * @param[in] aTimeOfFlight         A time-of-flight (arrival-departure epoch) for a grid point
     * @param[in] aTransferDeltaV       Total computed transfer \f$\Delta V\f$
     */
    LambertPorkChopPlotGridPoint( const int       aTransferId,
                                  const double    aDepartureEpoch,
                                  const double    anArrivalEpoch,
                                  const double    aTimeOfFlight,
                                  const double    aTransferDeltaV )
        : transferId( aTransferId ),
          departureEpoch( aDepartureEpoch ),
          arrivalEpoch( anArrivalEpoch ),
          timeOfFlight( aTimeOfFlight ),
          transferDeltaV( aTransferDeltaV )
    { }

    //! Overload operator-=.
    /*!
     * Overloads operator-= to assign current object to object provided as input.
     *
     * WARNING: this is a dummy overload to get by the problem of adding a
     *          LambertPorkChopPlotGridPoint object to a STL container! It does not correctly assign
     *          the current object to the dummy grid point provided!
     *
     * @sa executeLambertScanner
     * @param[in] dummyGridPoint Dummy grid point that is ignored
     * @return                   The current object
     */
    LambertPorkChopPlotGridPoint& operator=( const LambertPorkChopPlotGridPoint& dummyGridPoint )
    {
        return *this;
    }

    //! Unique transfer ID.
    const int transferId;

    //! Departure epoch.
    const double departureEpoch;

    //! Arrival epoch.
    const double arrivalEpoch;

    //! Time of flight [s].
    const double timeOfFlight;

    //! Total transfer \f$\Delta V\f$ [km/s].
    const double transferDeltaV;

    // bool operator<(const LambertPorkChopPlotGridPoint& other)
    // {
    //     return (transferDeltaV > other.transferDeltaV);
    // }
protected:
private:
};



} // namespace d2d

/*!
 * Izzo, D. (2014) Revisiting Lambert's problem, http://arxiv.org/abs/1403.2705.
 * Izzo, D. (2012) PyGMO and PyKEP: open source tools for massively parallel optimization in
 * 	astrodynamics (the case of interplanetary trajectory optimization). Proceed. Fifth
 *  International Conf. Astrodynam. Tools and Techniques, ESA/ESTEC, The Netherlands.
 */

#endif // D2D_LAMBERT_SEQUENCES_HPP
