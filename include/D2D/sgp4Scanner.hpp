/*
 * Copyright (c) 2014-2016 Kartik Kumar (me@kartikkumar.com)
 * Copyright (c) 2014-2016 Abhishek Agrawal (abhishek.agrawal@protonmail.com)
 * Distributed under the MIT License.
 * See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT
 */

#ifndef D2D_SGP4_SCANNER_HPP
#define D2D_SGP4_SCANNER_HPP

#include <string>

#include <rapidjson/document.h>

#include <SQLiteCpp/SQLiteCpp.h>

namespace d2d
{

//! Execute sgp4_scanner.
/*!
 * Executes sgp4_scanner application mode that propagates a transfer, computed with the Lambert
 * targeter (Izzo, 2014), using the SGP4/SDP4 models, to compute the mismatch in arrival position
 * and velocity due to perturbations. The Lambert targeter employed is based on Izzo (2014),
 * implemented in PyKEP (Izzo, 2012).
 *
 * This requires that the "lambert_scanner" application mode has been executed, which generates a
 * SQLite database containing all transfers computed (stored in "lambert_scanner_results").
 *
 * This function is called when the user specifies the application mode to be "sgp4_scanner".
 *
 * @sa executeLambertTransfer, executeSGP4Scanner
 * @param[in] config User-defined configuration options (extracted from JSON input file)
 */
void executeSGP4Scanner( const rapidjson::Document& config );

//! Input for sgp4_scanner application mode.
/*!
 * Data struct containing all valid input parameters for the sgp4_scanner application mode.
 * This struct is populated by the checkSGP4ScannerInput() function and can be used to execute
 * the SGP4 Scanner.
 *
 * @sa checkSGP4ScannerInput, executeSGP4Scanner
 */
struct sgp4ScannerInput
{
public:

    //! Construct data struct.
    /*!
     * Constructs data struct based on verified input parameters.
     *
     * @sa checkSGP4ScannerInput, executeSGP4Scanner
     * @param[in] aTransferDeltaVCutoff   Transfer \f$\Delta V\f$ cut-off used by sgp4 scanner
     * @param[in] aRelativeTolerance      Relative tolerance for Cartesian-to-TLE conversion
     * @param[in] aAbsoluteTolerance      Absolute tolerance for the Cartesian-to-TLE conversion
     * @param[in] aDatabasePath           Path to SQLite database
     * @param[in] aShortlistLength        Number of transfers to include in shortlist
     * @param[in] aShortlistPath          Path to shortlist file
     */
    sgp4ScannerInput( const double       aTransferDeltaVCutoff,
                      const double       aRelativeTolerance,
                      const double       aAbsoluteTolerance,
                      const std::string& aDatabasePath,
                      const int          aShortlistLength,
                      const std::string& aShortlistPath )
        : transferDeltaVCutoff( aTransferDeltaVCutoff ),
          relativeTolerance( aRelativeTolerance ),
          absoluteTolerance( aAbsoluteTolerance ),
          databasePath( aDatabasePath ),
          shortlistLength( aShortlistLength ),
          shortlistPath( aShortlistPath )
    { }

    //! Transfer \f$\Delta V\f$ cut-off used by sgp4_scanner.
    const double transferDeltaVCutoff;

    //! Relative tolerance for Cartesian-to-TLE conversion function.
    const double relativeTolerance;

    //! Absolute tolerance for Cartesian-to-TLE conversion function.
    const double absoluteTolerance;

    //! Path to SQLite database to store output.
    const std::string databasePath;

    //! Number of entries (lowest Lambert transfer \f$\Delta V\f$) to include in shortlist.
    const int shortlistLength;

    //! Path to shortlist file.
    const std::string shortlistPath;

protected:

private:
};

//! Check sgp4_scanner input parameters.
/*!
 * Checks that all sgp4_scanner inputs are valid. If not, an error is thrown with a short
 * description of the problem. If all inputs are valid, a data struct containing all the inputs
 * is returned, which is subsequently used to execute the sgp4_scanner application mode and
 * related functions.
 *
 * @sa executeSGP4Scanner, sgp4ScannerInput
 * @param[in] config User-defined configuration options (extracted from JSON input file)
 * @return           Struct containing all valid input to execute SGP4 Scanner
 */
sgp4ScannerInput checkSGP4ScannerInput( const rapidjson::Document& config );

//! Create sgp4_scanner_results table.
/*!
 * Creates sgp4_scanner_results table in SQLite database used to store results obtained from running
 * the "sgp4_scanner" application mode.
 *
 * @sa executeSGP4Scanner
 * @param[in] database SQLite database handle
 */
void createSGP4ScannerTable( SQLite::Database& database );

//! Bind zeroes into sgp4_scanner_results table.
/*!
 * Bind zeroes into sgp4_scanner_results table in SQLite database when the total Lambert transfer
 * \f$\Delta V\f$ is above a user specified cut-off, the convergence test for the
 * virtual TLE fails, or the SGP4 propagation of the virtual TLE to the arrival epoch fails.
 *
 * @sa executeSGP4Scanner
 * @param[in] lambertTransferId transfer_id in the lambert_scanner_results table
 */
std::string bindZeroesSGP4ScannerTable( const int lambertTransferId );

//! Write transfer shortlist to file.
/*!
 * Writes shortlist of debris-to-debris transfers from the SGP4 scanner to file. The shortlist is
 * based on the requested number of transfers with the lowest Lambert transfer \f$\Delta V\f$,
 * retrieved by sorting the transfers in the SQLite database.
 *
 * @sa executeSGP4Scanner, createSGP4ScannerTable
 * @param[in] database        SQLite database handle
 * @param[in] shortlistNumber Number of entries to include in shortlist (if it exceeds number of
 *                            entries in database table, the whole table is written to file)
 * @param[in] shortlistPath   Path to shortlist file
 */
void writeSGP4TransferShortlist( SQLite::Database& database,
                                 const int shortlistNumber,
                                 const std::string& shortlistPath );

} // namespace d2d

#endif // D2D_SGP4_SCANNER_HPP
