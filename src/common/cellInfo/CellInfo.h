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

#ifndef _CELLINFO_H_
#define _CELLINFO_H_

#include <omnetpp.h>

#include <inet/common/ModuleRefByPar.h>
#include <inet/networklayer/contract/ipv4/Ipv4Address.h>

#include "stack/phy/das/RemoteAntennaSet.h"
#include "common/binder/Binder.h"
#include "common/LteCommon.h"

namespace simu5g {

using namespace omnetpp;

class DasFilter;

/**
 * @class CellInfo
 * @brief There is one CellInfo module for each eNB (thus one for each cell). Keeps cross-layer information about the cell
 */
class CellInfo : public cSimpleModule
{
  private:
    /// reference to the global module binder
    inet::ModuleRefByPar<Binder> binder_;

    /// Remote Antennas for eNB
    RemoteAntennaSet *ruSet_ = new RemoteAntennaSet();

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

    /// Number of DAS RU
    int numRus_ = 0;
    /// Remote and its CW
    std::map<Remote, int> antennaCws_;

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
    std::vector<double> carriersVector_;

    // max numerology index used in this cell
    NumerologyIndex maxNumerologyIndex_ = 0;
    /************************************/

    // Position of each UE
    std::map<MacNodeId, inet::Coord> uePosition;

    std::map<MacNodeId, Lambda> lambdaMap_;

  protected:

    void initialize(int stage) override;
    int numInitStages() const override { return inet::INITSTAGE_LOCAL + 2; }

    /**
     * Deploys remote antennas.
     *
     * This is a virtual deployment: the cellInfo needs only to inform
     * the eNB NIC module about the position of the deployed antennas and
     * their TX power. These parameters are configured via the cellInfo, but
     * no NED module is created here.
     *
     * @param nodeX x coordinate of the center of the master node
     * @param nodeY y coordinate of the center of the master node
     * @param numRu number of remote units to be deployed
     * @param ruRange distance between eNB and RUs
     */
    virtual void deployRu(double nodeX, double nodeY, int numRu, int ruRange);
    virtual void calculateMCSScale(double *mcsUl, double *mcsDl);
    virtual void updateMCSScale(double *mcs, double signalRe, double signalCarriers = 0, Direction dir = DL);

    /**
     * Compute slot format object given the number of DL and UL symbols
     */
    virtual SlotFormat computeSlotFormat(bool useTdd, unsigned int tddNumSymbolsDl, unsigned int tddNumSymbolsUl);

  private:
    /**
     * Calculates node position around a circumference.
     *
     * @param centerX x coordinate of the center
     * @param centerY y coordinate of the center
     * @param nTh ordering number of the UE to be placed in this circumference
     * @param totalNodes total number of nodes that will be placed
     * @param range circumference range
     * @param startingAngle angle of the first deployed node (degrees)
     * @param[out] xPos calculated x coordinate
     * @param[out] yPos calculated y coordinate
     */
    // Used by remote units only
    void calculateNodePosition(double centerX, double centerY, int nTh,
            int totalNodes, int range, double startingAngle, double *xPos,
            double *yPos);

    void createAntennaCwMap();

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
    double getPrimaryCarrierFrequency();

    double getMcsScaleUl()
    {
        return mcsScaleUl_;
    }

    double getMcsScaleDl()
    {
        return mcsScaleDl_;
    }

    int getNumRus()
    {
        return numRus_;
    }

    std::map<Remote, int> getAntennaCws()
    {
        return antennaCws_;
    }

    int getNumPreferredBands()
    {
        return numPreferredBands_;
    }

    RemoteAntennaSet *getRemoteAntennaSet()
    {
        return ruSet_;
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

    const std::map<MacNodeId, inet::Coord> *getUePositionList()
    {
        return &uePosition;
    }

    void setUePosition(MacNodeId id, inet::Coord c)
    {
        uePosition[id] = c;
    }

    void lambdaUpdate(MacNodeId id, unsigned int index)
    {
        lambdaMap_[id].lambdaMax = binder_->phyPisaData.getLambda(index, 0);
        lambdaMap_[id].index = index;
        lambdaMap_[id].lambdaMin = binder_->phyPisaData.getLambda(index, 1);
        lambdaMap_[id].lambdaRatio = binder_->phyPisaData.getLambda(index, 2);
    }

    void lambdaIncrease(MacNodeId id, unsigned int i)
    {
        lambdaMap_[id].index = lambdaMap_[id].lambdaStart + i;
        lambdaUpdate(id, lambdaMap_[id].index);
    }

    void lambdaInit(MacNodeId id, unsigned int i)
    {
        lambdaMap_[id].lambdaStart = i;
        lambdaMap_[id].index = lambdaMap_[id].lambdaStart;
        lambdaUpdate(id, lambdaMap_[id].index);
    }

    void channelUpdate(MacNodeId id, unsigned int in)
    {
        unsigned int index = in % binder_->phyPisaData.maxChannel2();
        lambdaMap_[id].channelIndex = index;
    }

    void channelIncrease(MacNodeId id)
    {
        unsigned int i = getNumBands();
        channelUpdate(id, lambdaMap_[id].channelIndex + i);
    }

    Lambda *getLambda(MacNodeId id)
    {
        return &(lambdaMap_.at(id));
    }

    std::map<MacNodeId, Lambda> *getLambda()
    {
        return &lambdaMap_;
    }

    //---------------------------------------------------------------

    /*
     * Carrier Aggregation support
     */
    // register a new carrier for this node with the given number of bands
    void registerCarrier(double carrierFrequency, unsigned int carrierNumBands, unsigned int numerologyIndex,
            bool useTdd = false, unsigned int tddNumSymbolsDl = 0, unsigned int tddNumSymbolsUl = 0);

    const std::vector<double> *getCarriers();

    const CarrierInfoMap *getCarrierInfoMap();

    NumerologyIndex getMaxNumerologyIndex() { return maxNumerologyIndex_; }

    // convert a carrier-local band index to a cellwide band index
    unsigned int getCellwiseBand(double carrierFrequency, Band index);

    // returns the number of bands for the primary cell
    unsigned int getPrimaryCarrierNumBands();

    // returns the number of bands for the given carrier
    unsigned int getCarrierNumBands(double carrierFrequency);

    // returns the band limit vector for the given carrier
    // if arg is not specified, returns the info for the primary carrier
    BandLimitVector *getCarrierBandLimit(double carrierFrequency);

    unsigned int getCarrierStartingBand(double carrierFrequency);
    unsigned int getCarrierLastBand(double carrierFrequency);
    /*******************************************************************************/

    void detachUser(MacNodeId nodeId);
    void attachUser(MacNodeId nodeId);

    ~CellInfo() override;
};

} //namespace

#endif

