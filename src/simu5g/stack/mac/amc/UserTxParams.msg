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


import simu5g.common.LteCommonEnum;
import simu5g.common.LteCommon;

cplusplus {{
#include "simu5g/stack/mac/amc/LteMcs.h"
}}

namespace simu5g;

//
// @class UserTxParams
//
// This class specifies the transmission mode and the resources
// used by a user for the transmission.
// The class also contains the RI, CQI, PMI actually used for transmission.
//
// UserTxParams are set by the AMC Pilot.
//
class UserTxParams
{
    TxMode txMode  = SINGLE_ANTENNA_PORT0 @getter(readTxMode) @setter(writeTxMode); // tx mode to use

    Rank ri @getter(readRank) @setter(writeRank) = NORANK;       // RI

    Cqi cqiVector[] @custom = CQI_INIT_VALUES; // vector with as many elements as there are cw: therefore it contains wb cqi

    Pmi pmi @getter(readPmi) @setter(writePmi) = NOPMI;       // WB pmi

    // std::set<Band> allowedBands_;     // bands on which the user can transmit
    // TODO make as visible

    bool valid = false @getter(isValid); // indicates whether the user info is set

    //! set of Remote Antennas in use for transmission  (DAS support)
    // std::set<Remote> antennaSet_;
    // TODO make as visible
}

cplusplus(cc) {{
std::vector<Cqi> UserTxParams::CQI_INIT_VALUES = { NOSIGNALCQI };
}}

cplusplus(UserTxParams) {{
  protected:
    static std::vector<Cqi> CQI_INIT_VALUES;

    std::vector<Cqi> cqiVector;

    std::set<Band> allowedBands_;     // bands on which the user can transmit

    //! set of Remote Antennas in use for transmission  (DAS support)
    std::set<Remote> antennaSet_;

  public:
    UserTxParams* dup() const { return new UserTxParams(*this); }
    size_t getCqiVectorArraySize() const { return cqiVector.size(); }
    void setCqiVectorArraySize(size_t size) { cqiVector.resize(size); }
    Cqi getCqiVector(size_t k) const { return cqiVector.at(k); } 
    void setCqiVector(size_t k, Cqi cqi) { cqiVector.at(k) = cqi; }
    void eraseCqiVector(size_t k) { cqiVector.clear(); }

    //! Get the per-codeword CQIs.
    const std::vector<Cqi>& readCqiVector() const { return cqiVector; }

    //! Set the per-codeword CQIs.
    void writeCqi(const std::vector<Cqi>& cqi) { cqiVector = cqi; }

    /** Get the modulation of the codeword. This function does not check if codeword is set.
     *  @param cw The codeword.
     *  @return The modulation.
     */
    LteMod getCwModulation(Codeword cw) const { return cqiTable[cqiVector.at(cw)].mod_; }

    /** Get the nominal code rate given the codeword. This function does not check if codeword is set.
     *  @param cw The codeword.
     *  @return The nominal code rate.
     */
    double getCwRate(Codeword cw) const { return cqiTable[cqiVector.at(cw)].rate_; }

    //! Get the assigned band vector.
    const std::set<Band>& readBands() const
    {
        return allowedBands_;
    }

    //! Get the remote antenna set - DAS
    const std::set<Remote>& readAntennaSet() const
    {
        return antennaSet_;
    }

    //! Set the remote antenna set - DAS
    void writeAntennas(const std::set<Remote>& antennas)
    {
        antennaSet_ = antennas;
    }

    //! Get the assigned band vector.
    void writeBands(const std::set<Band>& bands)
    {
        allowedBands_ = bands;
    }

    /** Gives the number of layers for each codeword.
     *  @return A vector containing the number of layers per codeword.
     */
    std::vector<unsigned char> getLayers() const
    {
        return cwMapping(txMode, ri, ri);
    }

    //! Reset to default values.
    void restoreDefaultValues()
    {
        allowedBands_.clear();

        txMode = SINGLE_ANTENNA_PORT0;
        ri = NORANK;
        pmi = NOPMI;
        cqiVector = CQI_INIT_VALUES;

        valid = false;
        antennaSet_.clear();
        // by default the system works with the MACRO antenna configured on all terminals
        antennaSet_.insert(MACRO);
    }

    /** Print debug information - FOR DEBUG ONLY
     *  @param s The name of the invoking function.
     */
    void print(const char *s) const;
}}

cplusplus(UserTxParams::copy) {{
    cqiVector = other.cqiVector;
    allowedBands_ = other.allowedBands_;
    antennaSet_ = other.antennaSet_;
}}

cplusplus(UserTxParams::parsimPack) {{
    doParsimPacking(b, cqiVector);
    doParsimPacking(b, allowedBands_);
    doParsimPacking(b, antennaSet_);
}}

cplusplus(UserTxParams::parsimUnpack) {{
    doParsimUnpacking(b, cqiVector);
    doParsimUnpacking(b, allowedBands_);
    doParsimUnpacking(b, antennaSet_);
}}
