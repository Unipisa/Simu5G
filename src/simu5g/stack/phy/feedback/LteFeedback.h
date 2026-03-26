//
//                  Simu5G
//
// Copyright (C) 2012-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
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

#include "simu5g/common/LteCommon.h"
#include "simu5g/stack/mac/amc/UserTxParams.h"
#include "simu5g/stack/phy/feedback/LteSummaryFeedback.h"

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
        WB_PMI          = 0x0004, // unused, PMI not modeled
        BAND_CQI        = 0x0008,
        BAND_PMI        = 0x0010, // unused, PMI not modeled
        PREFERRED_CQI   = 0x0020,
        PREFERRED_PMI   = 0x0040  // unused, PMI not modeled
    };

    //! Rank indicator.
    Rank rank_;

    //! Wide-band CQI, one per codeword.
    CqiVector wideBandCqi_;

    //! Per-band CQI. (bandCqi_[cw][band])
    std::vector<CqiVector> perBandCqi_;

    //! Per subset of preferred bands CQI.
    CqiVector preferredCqi_;
    //! Set of preferred bands.
    BandSet preferredBands_;

    //! Current status.
    unsigned int status_ = EMPTY;
    //! Current transmission mode.
    TxMode txMode_ = UNKNOWN_TX_MODE;

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
        return txMode_ == UNKNOWN_TX_MODE || status_ == EMPTY;
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

    bool hasBandCqi() const
    {
        return status_ & BAND_CQI;
    }

    bool hasPreferredCqi() const
    {
        return status_ & PREFERRED_CQI;
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

} //namespace

#endif
