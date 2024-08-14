//
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_LTEMCS_H_
#define _LTE_LTEMCS_H_

#include "common/LteCommon.h"

namespace simu5g {

// This file contains MCS types and constants; MCS and ITBS tables;
// and functions related to MCS and Tx-Modes.

struct CQIelem
{
    /// Modulation
    LteMod mod_;

    /// Code rate x 1024
    double rate_;

    /// Constructor, with default set to "out of range CQI"
    CQIelem(LteMod mod = _QPSK, double rate = 0.0) : mod_(mod), rate_(rate)
    {
    }

};

/**
 * <CQI Index [0-15]> , <Modulation> , <Code Rate x 1024>
 * This table contains values taken from table 7.2.3-1 (TS 36.213)
 */
extern const CQIelem cqiTable[];

struct MCSelem
{
    LteMod mod_;       /// modulation
    Tbs iTbs_;         /// iTBS
    double threshold_; /// code rate threshold

    MCSelem(LteMod mod = _QPSK, Tbs iTbs = 0, double threshold = 0.0) : mod_(mod), iTbs_(iTbs), threshold_(threshold)
    {
    }

};

/**
 * <MCS Index> , <Modulation> , <I-TBS> , <threshold>
 * This table contains values taken from (TS 36.213)
 */
class McsTable
{
  public:

    MCSelem table[CQI2ITBSSIZE];

    McsTable();

    /// MCS table seek operator
    MCSelem& at(Tbs tbs)
    {
        return table[tbs];
    }

    /// MCS Table re-scaling function
    void rescale(double scale);
};

/********************************************
*      ITBS 2 TBS FRIGHTENING TABLES
********************************************/

extern const unsigned int itbs2tbs_qpsk_1[][110];
extern const unsigned int itbs2tbs_16qam_1[][110];
extern const unsigned int itbs2tbs_64qam_1[][110];
extern const unsigned int itbs2tbs_qpsk_2[][110];
extern const unsigned int itbs2tbs_16qam_2[][110];
extern const unsigned int itbs2tbs_64qam_2[][110];
extern const unsigned int itbs2tbs_qpsk_4[][110];
extern const unsigned int itbs2tbs_16qam_4[][110];
extern const unsigned int itbs2tbs_64qam_4[][110];
extern const unsigned int itbs2tbs_qpsk_8[][110];
extern const unsigned int itbs2tbs_16qam8[][110];
extern const unsigned int itbs2tbs_64qam8[][110];

/**
 * @param mod The modulation.
 * @param txMode The transmission mode.
 * @param layers The number of layers.
 * @param itbs The information block size.
 * @return A row of table 3-2 or table 3-3 specific for the given iTBS.
 */
const unsigned int *itbs2tbs(LteMod mod, TxMode txMode, unsigned char layers, unsigned char itbs);

/**
 * Gives the number of layers for each codeword.
 * @param txMode The transmission mode.
 * @param ri The Rank Indication.
 * @param antennaPorts Number of antenna ports
 * @return A vector containing the number of layers per codeword.
 */
std::vector<unsigned char> cwMapping(const TxMode& txMode, const Rank& ri, const unsigned int antennaPorts);

} //namespace

#endif

