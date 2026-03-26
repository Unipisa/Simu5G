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

#ifndef _CELLINFO_H_
#define _CELLINFO_H_

#include <inet/common/ModuleRefByPar.h>
#include <inet/networklayer/contract/ipv4/Ipv4Address.h>

#include "simu5g/common/binder/Binder.h"
#include "simu5g/common/LteCommon.h"

namespace simu5g {

using namespace omnetpp;

/**
 * @class CellInfo
 * @brief There is one CellInfo module for each eNB (thus one for each cell). Keeps cross-layer information about the cell
 */
class CellInfo : public cSimpleModule
{
  private:
    /// reference to the global module binder
    inet::ModuleRefByPar<Binder> binder_;

    /// Cell Id
    MacCellId cellId_;

    // MACRO_ENB or MICRO_ENB
    EnbType eNbType_;

    /// x playground lower bound
    double pgnMinX_;
    /// y playground lower bound
    double pgnMinY_;
    /// x playground upper bound
    double pgnMaxX_;
    /// y playground upper bound
    double pgnMaxY_;

    /// x eNB position
    double nodeX_ = 0;
    /// y eNB position
    double nodeY_ = 0;

    /// number of logical bands in the system
    int totalBands_;
    /// number of preferred bands to use (meaningful only in PREFERRED mode)
    int numPreferredBands_;

    // TODO these should be parameters per carrier, move to CarrierComponent

    /// number of sub-carriers per RB, DL
    int rbyDl_;
    /// number of sub-carriers per RB, UL
    int rbyUl_;
    /// number of OFDM symbols per slot, DL
    int rbxDl_;
    /// number of OFDM symbols per slot, UL
    int rbxUl_;
    /// number of pilot REs per RB, DL
    int rbPilotDl_;
    /// number of pilot REs per RB, UL
    int rbPilotUl_;
    /// number of signaling symbols for RB, DL
    int signalDl_;
    /// number of signaling symbols for RB, UL
    int signalUl_;
    /// MCS scale UL
    double mcsScaleUl_ = 0;
    /// MCS scale DL
    double mcsScaleDl_ = 0;

    /*
     * Carrier Aggregation support
     */
    // total number of logical bands *in this cell* (sum of bands used by carriers enabled in this cell)
    unsigned int numBands_ = 0;
    CarrierInfoMap carrierMap_;

    // store the carrier frequencies used by this cell
    std::vector<GHz> carriersVector_;

    // max numerology index used in this cell
    NumerologyIndex maxNumerologyIndex_ = 0;
    /************************************/

    // Position of each UE
    std::map<MacNodeId, inet::Coord> uePosition;

  protected:

    void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }

    virtual void calculateMcsScale();
    virtual void updateMCSScale(double& mcs, double signalRe, double signalCarriers = 0, Direction dir = DL);

    /**
     * Compute slot format object given the number of DL and UL symbols
     */
    virtual SlotFormat computeSlotFormat(bool useTdd, unsigned int tddNumSymbolsDl, unsigned int tddNumSymbolsUl);

  public:


    MacCellId getMacCellId()
    {
        return cellId_;
    }

    int getRbyDl()
    {
        return rbyDl_;
    }

    int getRbyUl()
    {
        return rbyUl_;
    }

    int getRbxDl()
    {
        return rbxDl_;
    }

    int getRbxUl()
    {
        return rbxUl_;
    }

    int getRbPilotDl()
    {
        return rbPilotDl_;
    }

    int getRbPilotUl()
    {
        return rbPilotUl_;
    }

    int getSignalDl()
    {
        return signalDl_;
    }

    int getSignalUl()
    {
        return signalUl_;
    }

    int getTotalBands()
    {
        return totalBands_;
    }

    // returns the number of bands in the cell
    unsigned int getNumBands();

    // returns the carrier frequency of the primary cell
    GHz getPrimaryCarrierFrequency();

    double getMcsScaleUl()
    {
        return mcsScaleUl_;
    }

    double getMcsScaleDl()
    {
        return mcsScaleDl_;
    }

    int getNumPreferredBands()
    {
        return numPreferredBands_;
    }


    void setEnbType(EnbType t)
    {
        eNbType_ = t;
    }

    EnbType getEnbType()
    {
        return eNbType_;
    }

    inet::Coord getUePosition(MacNodeId id)
    {
        if (uePosition.find(id) != uePosition.end())
            return uePosition[id];
        else
            return inet::Coord::ZERO;
    }

    const std::map<MacNodeId, inet::Coord>& getUePositionList()
    {
        return uePosition;
    }

    void setUePosition(MacNodeId id, inet::Coord c)
    {
        uePosition[id] = c;
    }

    //---------------------------------------------------------------

    /*
     * Carrier Aggregation support
     */
    // register a new carrier for this node with the given number of bands
    void registerCarrier(GHz carrierFrequency, unsigned int carrierNumBands, unsigned int numerologyIndex,
            bool useTdd = false, unsigned int tddNumSymbolsDl = 0, unsigned int tddNumSymbolsUl = 0);

    const std::vector<GHz>& getCarriers();

    const CarrierInfoMap& getCarrierInfoMap();

    NumerologyIndex getMaxNumerologyIndex() { return maxNumerologyIndex_; }

    // convert a carrier-local band index to a cellwide band index
    unsigned int getCellwiseBand(GHz carrierFrequency, Band index);

    // returns the number of bands for the primary cell
    unsigned int getPrimaryCarrierNumBands();

    // returns the number of bands for the given carrier
    unsigned int getCarrierNumBands(GHz carrierFrequency);

    // returns the band limit vector for the given carrier
    // if arg is not specified, returns the info for the primary carrier
    const BandLimitVector& getCarrierBandLimit(GHz carrierFrequency);

    unsigned int getCarrierStartingBand(GHz carrierFrequency);
    unsigned int getCarrierLastBand(GHz carrierFrequency);
    /*******************************************************************************/

    void detachUser(MacNodeId nodeId);
    void attachUser(MacNodeId nodeId);

};

} //namespace

#endif
