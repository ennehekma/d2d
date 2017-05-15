/*
 * Copyright (c) 2014-2016 Kartik Kumar, Dinamica Srl (me@kartikkumar.com)
 * Copyright (c) 2014-2016 Abhishek Agrawal, Delft University of Technology (abhishek.agrawal@protonmail.com)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */

#ifndef D2D_TOOLS_HPP
#define D2D_TOOLS_HPP

#include <string>

#include <keplerian_toolbox.h>

#include <libsgp4/DateTime.h>

#include <rapidjson/document.h>

#include <SQLiteCpp/SQLiteCpp.h>

#include <algorithm>
#include <iostream>
#include <limits>
#include <map>
#include <string>

#include <boost/array.hpp>

#include <catch.hpp>

#include <libsgp4/Eci.h>
#include <libsgp4/Tle.h>

#include <rapidjson/document.h>

#include <Astro/astro.hpp>

#include "D2D/typedefs.hpp"

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

//! Create custom CATCH Approx object with tolerance for comparing doubles.
/*!
 * Creates a custom CATCH Approx object that can be used for comparing doubles in unit tests. The
 * tolerance is set based on machine precision (epsilon) for doubles.
 *
 * Using this comparison object ensures that large numbers are compared in a relative sense and
 * small numbers in an absolute sense.
 *
 * @see <a href="https://github.com/philsquared/Catch/include/internal/catch_approx.hpp">CATCH</a>
 * @see <a href="http://www.thusspakeak.com/ak/2013/06/01-YoureGoingToHaveToThink01.html">
 *      Harris, R. (2013)</a>
 */
static Approx approx
  = Approx::custom( ).epsilon( std::numeric_limits< double >::epsilon( ) * 1000.0 );

//! Get root-path for D2D.
/*!
 * Returns path to root-directory of the D2D application (trailing slash included).
 *
 * @todo    Find a way to test the root-path function.
 *
 * @return  D2D root-path.
 */
static inline std::string getRootPath( )
{
    std::string filePath( __FILE__ );
    return filePath.substr(
      0, filePath.length( ) - std::string( "include/D2D/tools.hpp" ).length( ) );
}

//! Sample Kepler orbit.
/*!
 * Samples a Kepler orbit and generates a state-history stored in a STL map (key=epoch). The
 * Kepler orbit is sampled by using propagate_lagrangian() provided with PyKep (Izzo, 2012).
 *
 * @param[in]  initialState           Initial Cartesian state [km; km/s]
 * @param[in]  propagationTime        Total propagation time [s]
 * @param[in]  numberOfSamples        Number of samples, distributed evenly over propagation time
 * @param[in]  gravitationalParameter Gravitational parameter of central body [km^3 s^-2]
 * @param[in]  initialEpoch           Epoch corresponding to initial Cartesian state
 *                                    (default = 0.0) [Julian date]
 * @return                            State-history of sampled Kepler orbit
 */
StateHistory sampleKeplerOrbit( const Vector6& initialState,
                                const double propagationTime,
                                const int numberOfSamples,
                                const double gravitationalParameter,
                                const double initialEpoch = 0.0 );

//! Sample SGP4 orbit.
/*!
 * Samples a SGP4 orbit and generates a state-history stored in a STL map (key=epoch). The
 * SGP4 orbit is sampled by using FindPosition() provided with libsgp4.
 *
 * @param[in]  tle                    Two-line element data of the object to be propagated 
 * @param[in]  initialEpochJulian     Starting epoch for the SGP4 propagator
 *                                    (default = 0.0) [Julian date]
 * @param[in]  propagationTime        Total propagation time [s]
 * @param[in]  numberOfSamples        Number of samples, distributed evenly over propagation time
 * @return                            State-history of sampled SGP4 orbit
 */
StateHistory sampleSGP4Orbit( const Tle& tle,
                              const double propagationTime,
                              const int numberOfSamples,
                              const double initialEpochJulian = 0.0 );

//! Execute convergence test for a virtual TLE.
/*!
 * A test of convergence for a virtual TLE generated from the
 * convertCartesianStateToTwoLineElements function in the Atom library.
 *
 * @sa atom::convertCartesianStateToTwoLineElements
 * @param[in]   propagatedCartesianState    The state obtained after propagating virtual TLE 
 *                                          using SGP4 with time-of-flight = 0.0
 * @param[in]   trueCartesianState          The true cartesian state corresponding to zero 
 *                                          time-of-flight
 * @param[in]   relativeTolerance           Relative difference between the propagated and true 
 *                                          Cartesian state is checked against the relative 
 *                                          tolerance
 * @param[in]   absoluteTolerance           Absolute difference between the propagated and true 
 *                                          Cartesian state is checked against the absolute 
 *                                          tolerance
 * @return                                  Returns boolean 'true' if the test passed, 
 *                                          'false' otherwise
 */
bool executeVirtualTleConvergenceTest( const Vector6& propagatedCartesianState,
                                       const Vector6& trueCartesianState,
                                       const double relativeTolerance,
                                       const double absoluteTolerance );

//! Convert SGP4 ECI object to state vector.
/*!
 * Converts a Cartesian state stored in an object of the Eci class in the SGP4 library () into a
 * boost::array of 6 elements.
 *
 * @sa Eci, boost::array
 * @param  state State stored in Eci object
 * @return       State stored in boost::array
 */
Vector6 getStateVector( const Eci state );

//! Print value to stream.
/*!
 * Prints a specified value to stream provided, given a specified width and a filler character.
 *
 * @tparam     DataType  Type for specified value
 * @param[out] stream    Output stream
 * @param[in]  value     Specified value to print
 * @param[in]  width     Width of value printed to stream, in terms of number of characters
 *                       (default = 25)
 * @param[in]  filler    Character used to fill fixed-width (default = ' ')
 */
template< typename DataType >
inline void print( std::ostream& stream,
                   const DataType value,
                   const int width = 25,
                   const char filler = ' ' )
{
    stream << std::left << std::setw( width ) << std::setfill( filler ) << value;
}

//! Print metadata parameter to stream.
/*!
 * Prints metadata parameter to stream provided, given a specified name, value, units, and
 * delimiter.
 *
 * @tparam     DataType      Type for specified value
 * @param[out] stream        Output stream
 * @param[in]  parameterName Name for metadata parameter
 * @param[in]  value         Specified value to print
 * @param[in]  units         Units for value
 * @param[in]  delimiter     Character used to delimiter entries in stream (default = ' ')
 * @param[in]  width         Width of value printed to stream, in terms of number of characters
 *                           (default = 25)
 * @param[in]  filler        Character used to fill fixed-width (default = ' ')
 */
template< typename DataType >
inline void print( std::ostream& stream,
                   const std::string& parameterName,
                   const DataType value,
                   const std::string& units,
                   const char delimiter = ',',
                   const int width = 25,
                   const char filler = ' ' )
{
    print( stream, parameterName, width, filler );
    stream << delimiter;
    print( stream, value, width, filler );
    stream << delimiter;
    print( stream, units, width, filler );
}

//! Print state history to stream.
/*!
 * Prints state history to stream provided, given a specified tate history object, a stream header,
 * and a number of digits of precision.
 *
 * @param[out] stream        Output stream
 * @param[in]  stateHistory  State history containing epochs and associated state vectors
 * @param[in]  streamHeader  A header for the output stream (file header if a file stream is
 *                           provided) (default = "")
 * @param[in]  precision     Digits of precision for state history entries printed to stream
 *                           (default = number of digits of precision for a double)
 */
void print( std::ostream& stream,
            const StateHistory stateHistory,
            const std::string& streamHeader = "",
            const int precision = std::numeric_limits< double >::digits10 );

//! Find parameter.
/*!
 * Finds parameter in config stored in JSON document. An error is thrown if the parameter cannot
 * be found. If the parameter is found, an iterator to the member in the JSON document is returned.
 *
 * @param[in] config        JSON document containing config parameters
 * @param[in] parameterName Name of parameter to find
 * @return                  Iterator to parameter retreived from JSON document
 */
ConfigIterator find( const rapidjson::Document& config, const std::string& parameterName );

//! Remove newline characters from string.
/*!
 * Removes newline characters from a string by making use of the STL erase() and remove()
 * functions. Removes '\\n' and '\\r' characters.
 *
 * @param[in,out] string String from which newline characters should be removed
 */
void removeNewline( std::string& string );

//! Get TLE catalog type.
/*!
 * Determines the TLE catalog type (2-line or 3-line) based on the TLE format.
 *
 * @param[in] catalogFirstLine First line in TLE catalog
 * @return                     Number of lines per TLE in catalog
 */
int getTleCatalogType( const std::string& catalogFirstLine );

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

#endif // D2D_TOOLS_HPP
