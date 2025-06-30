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

#ifndef _LTE_LTEFEEDBACK_H_
#define _LTE_LTEFEEDBACK_H_

#include <map>
#include <vector>

#include "common/LteCommon.h"
#include "stack/mac/amc/UserTxParams.h"
#include "stack/phy/feedback/LteSummaryFeedback.h"

namespace simu5g {

class LteFeedback;
typedef std::vector<LteFeedback> LteFeedbackVector;
typedef std::vector<LteFeedbackVector> LteFeedbackDoubleVector;

//! LTE feedback message exchanged between PHY and AMC.
class LteFeedback
{
  protected:
    //! Feedback status type enumerator.
    enum
    {
        EMPTY           = 0x0000,
        RANK_INDICATION = 0x0001,
        WB_CQI          = 0x0002,
        WB_PMI          = 0x0004,
        BAND_CQI        = 0x0008,
        BAND_PMI        = 0x0010,
        PREFERRED_CQI   = 0x0020,
        PREFERRED_PMI   = 0x0040
    };

    //! Rank indicator.
    Rank rank_;

    //! Wide-band CQI, one per codeword.
    CqiVector wideBandCqi_;
    //! Wide-band PMI.
    Pmi wideBandPmi_;

    //! Per-band CQI. (bandCqi_[cw][band])
    std::vector<CqiVector> perBandCqi_;
    //! Per-band PMI.
    PmiVector perBandPmi_;

    //! Per subset of preferred bands CQI.
    CqiVector preferredCqi_;
    //! Per subset of preferred bands PMI.
    Pmi preferredPmi_;
    //! Set of preferred bands.
    BandSet preferredBands_;

    //! Current status.
    unsigned int status_ = EMPTY;
    //! Current transmission mode.
    TxMode txMode_ = SINGLE_ANTENNA_PORT0;

    //! Periodicity of the feedback message.
    bool periodicFeedback_ = true;
    //! \test DAS SUPPORT - Antenna identifier
    Remote remoteAntennaId_ = MACRO;

  public:

    //! Create an empty feedback message.

    //! Reset this feedback message as empty.
    void reset();

    //! Return true if feedback is empty.
    bool isEmptyFeedback() const
    {
        return !(status_);
    }

    //! Return true if it is periodic.
    bool isPeriodicFeedback() const
    {
        return periodicFeedback_;
    }

    /// check existence of elements
    bool hasRankIndicator() const
    {
        return status_ & RANK_INDICATION;
    }

    bool hasWbCqi() const
    {
        return status_ & WB_CQI;
    }

    bool hasWbPmi() const
    {
        return status_ & WB_PMI;
    }

    bool hasBandCqi() const
    {
        return status_ & BAND_CQI;
    }

    bool hasBandPmi() const
    {
        return status_ & BAND_PMI;
    }

    bool hasPreferredCqi() const
    {
        return status_ & PREFERRED_CQI;
    }

    bool hasPreferredPmi() const
    {
        return status_ & PREFERRED_PMI;
    }

    // **************** GETTERS ****************
    //! Get the rank indication. Does not check if valid.
    Rank getRankIndicator() const
    {
        return rank_;
    }

    //! Get the wide-band CQI. Does not check if valid.
    CqiVector getWbCqi() const
    {
        return wideBandCqi_;
    }

    //! Get the wide-band CQI for one codeword. Does not check if valid.
    Cqi getWbCqi(Codeword cw) const
    {
        return wideBandCqi_[cw];
    }

    //! Get the wide-band PMI. Does not check if valid.
    Pmi getWbPmi() const
    {
        return wideBandPmi_;
    }

    //! Get the per-band CQI. Does not check if valid.
    std::vector<CqiVector> getBandCqi() const
    {
        return perBandCqi_;
    }

    //! Get the per-band CQI for one codeword. Does not check if valid.
    CqiVector getBandCqi(Codeword cw) const
    {
        return perBandCqi_[cw];
    }

    //! Get the per-band PMI. Does not check if valid.
    PmiVector getBandPmi() const
    {
        return perBandPmi_;
    }

    //! Get the per preferred band CQI. Does not check if valid.
    CqiVector getPreferredCqi() const
    {
        return preferredCqi_;
    }

    //! Get the per preferred band CQI for one codeword. Does not check if valid.
    Cqi getPreferredCqi(Codeword cw) const
    {
        return preferredCqi_[cw];
    }

    //! Get the per preferred band PMI. Does not check if valid.
    Pmi getPreferredPmi() const
    {
        return preferredPmi_;
    }

    //! Get the set of preferred bands. Does not check if valid.
    BandSet getPreferredBands() const
    {
        return preferredBands_;
    }

    //! Get the transmission mode.
    TxMode getTxMode() const
    {
        return txMode_;
    }

    //! \test DAS support - Get the remote Antenna identifier
    Remote getAntennaId() const
    {
        return remoteAntennaId_;
    }

    // *****************************************

    // **************** SETTERS ****************
    //! Set the rank indicator.
    void setRankIndicator(const Rank rank)
    {
        rank_ = rank;
        status_ |= RANK_INDICATION;
    }

    //! Set the wide-band CQI.
    void setWideBandCqi(const CqiVector wbCqi)
    {
        wideBandCqi_ = wbCqi;
        status_ |= WB_CQI;
    }

    //! Set the wide-band CQI for one codeword. Does not check if valid.
    void setWideBandCqi(const Cqi cqi, const Codeword cw)
    {
        if (wideBandCqi_.size() == cw)
            wideBandCqi_.push_back(cqi);
        else
            wideBandCqi_[cw] = cqi;

        status_ |= WB_CQI;
    }

    //! Set the wide-band PMI.
    void setWideBandPmi(const Pmi wbPmi)
    {
        wideBandPmi_ = wbPmi;
        status_ |= WB_PMI;
    }

    //! Set the per-band CQI.
    void setPerBandCqi(const std::vector<CqiVector> bandCqi)
    {
        perBandCqi_ = bandCqi;
        status_ |= BAND_CQI;
    }

    //! Set the per-band CQI for one codeword. Does not check if valid.
    void setPerBandCqi(const CqiVector bandCqi, const Codeword cw)
    {
        if (perBandCqi_.size() == cw)
            perBandCqi_.push_back(bandCqi);
        else
            perBandCqi_[cw] = bandCqi;

        status_ |= BAND_CQI;
    }

    //! Set the per-band PMI.
    void setPerBandPmi(const PmiVector bandPmi)
    {
        perBandPmi_ = bandPmi;
        status_ |= BAND_PMI;
    }

    //! Set the per preferred band CQI.
    void setPreferredCqi(const CqiVector preferredCqi)
    {
        preferredCqi_ = preferredCqi;
        status_ |= PREFERRED_CQI;
    }

    //! Set the per preferred band CQI for one codeword. Does not check if valid.
    void setPreferredCqi(const Cqi preferredCqi, const Codeword cw)
    {
        if (preferredCqi_.size() == cw)
            preferredCqi_.push_back(preferredCqi);
        else
            preferredCqi_[cw] = preferredCqi;

        status_ |= PREFERRED_CQI;
    }

    //! Set the per preferred band PMI. Invoke this function only after setPreferredCqi().
    void setPreferredPmi(const Pmi preferredPmi)
    {
        preferredPmi_ = preferredPmi;
        status_ |= PREFERRED_PMI;
    }

    //! Set the per preferred bands. Invoke this function every time you invoke setPreferredCqi().
    void setPreferredBands(const BandSet preferredBands)
    {
        preferredBands_ = preferredBands;
    }

    //! Set the transmission mode.
    void setTxMode(const TxMode txMode)
    {
        txMode_ = txMode;
    }

    //! \test DAS support - Set the remote Antenna identifier
    void setAntenna(const Remote antenna)
    {
        remoteAntennaId_ = antenna;
    }

    //! Set the periodicity type.
    void setPeriodicity(const bool type)
    {
        periodicFeedback_ = type;
    }

    // *****************************************

    /** Print debug information about the LteFeedback instance.
     *  @param cellId The cell ID.
     *  @param nodeId The node ID.
     *  @param dir The link direction.
     *  @param s The name of the function that requested the debug.
     */
    void print(MacCellId cellId, MacNodeId nodeId, Direction dir, const char *s) const;
};

/**
 * @class LteMuMimoMatrix
 *
 * MU-MIMO compatibility matrix structure.
 * It holds MU-MIMO pairings computed by the MU-MIMO matching function.
 */
class LteMuMimoMatrix
{
    typedef std::vector<MacNodeId> MuMatrix;

  protected:
    MuMatrix muMatrix_;
    MacNodeId maxNodeId_ = NODEID_NONE;

    MacNodeId toNodeId(unsigned int i)
    {
        return UE_MIN_ID + i;
    }

    unsigned int toIndex(MacNodeId nodeId)
    {
        return nodeId - UE_MIN_ID;
    }

  public:

    void initialize(MacNodeId node)
    {
        muMatrix_.clear();
        muMatrix_.resize(toIndex(node) + 1, NODEID_NONE);
    }

    void addPair(MacNodeId id1, MacNodeId id2)
    {
        muMatrix_[toIndex(id1)] = id2;
        muMatrix_[toIndex(id2)] = id1;
    }

    MacNodeId getMuMimoPair(MacNodeId id)
    {
        return muMatrix_[toIndex(id)];
    }

    void print(const char *s) const;
};

} //namespace

#endif

