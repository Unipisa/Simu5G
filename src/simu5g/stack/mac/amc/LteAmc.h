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

#ifndef _LTE_LTEAMC_H_
#define _LTE_LTEAMC_H_

#include <omnetpp.h>

#include "common/cellInfo/CellInfo.h"
#include "stack/phy/feedback/LteFeedback.h"
#include "stack/phy/feedback/LteSummaryBuffer.h"
#include "stack/mac/amc/AmcPilot.h"
#include "stack/mac/amc/LteMcs.h"
#include "stack/mac/amc/UserTxParams.h"
#include "stack/mac/LteMacEnb.h"
#include "common/binder/Binder.h"

namespace simu5g {

using namespace omnetpp;

/// Forward declaration of AmcPilot class, used by LteAmc.
class AmcPilot;
/// Forward declaration of CellInfo class, used by LteAmc.
class CellInfo;
/// Forward declaration of LteMacEnb class, used by LteAmc.
class LteMacEnb;

typedef std::map<Remote, std::vector<std::vector<LteSummaryBuffer>>> History_;

/**
 * @class LteAMC
 * @brief Lte AMC module for Omnet++ simulator
 *
 * TODO
 */
class LteAmc
{
  private:
    AmcPilot *getAmcPilot(const cPar& amcMode);
    MacNodeId getNextHop(MacNodeId dst);

  public:
    void printParameters();
    void printFbhb(Direction dir);
    void printTxParams(Direction dir, double carrierFrequency);
    void printMuMimoMatrix(const char *s);

  protected:
    opp_component_ptr<LteMacEnb> mac_;
    opp_component_ptr<Binder> binder_;
    opp_component_ptr<CellInfo> cellInfo_;
    AmcPilot *pilot_ = nullptr;
    RbAllocationType allocationType_;
    int numBands_;
    MacNodeId nodeId_;
    MacCellId cellId_;
    McsTable dlMcsTable_;
    McsTable ulMcsTable_;
    McsTable d2dMcsTable_;
    double mcsScaleDl_;
    double mcsScaleUl_;
    double mcsScaleD2D_;
    int numAntennas_;
    RemoteSet remoteSet_;
    ConnectedUesMap dlConnectedUe_;
    ConnectedUesMap ulConnectedUe_;
    ConnectedUesMap d2dConnectedUe_;
    std::map<MacNodeId, unsigned int> dlNodeIndex_;
    std::map<MacNodeId, unsigned int> ulNodeIndex_;
    std::map<MacNodeId, unsigned int> d2dNodeIndex_;
    std::vector<MacNodeId> dlRevNodeIndex_;
    std::vector<MacNodeId> ulRevNodeIndex_;
    std::vector<MacNodeId> d2dRevNodeIndex_;

    // one tx param per carrier
    std::map<double, std::vector<UserTxParams>> dlTxParams_;
    std::map<double, std::vector<UserTxParams>> ulTxParams_;
    std::map<double, std::vector<UserTxParams>> d2dTxParams_;

    int fType_; //CQI synchronization Debugging

    // one History per carrier
    std::map<double, History_> dlFeedbackHistory_;
    std::map<double, History_> ulFeedbackHistory_;
    std::map<double, std::map<MacNodeId, History_>> d2dFeedbackHistory_;

    unsigned int fbhbCapacityDl_;
    unsigned int fbhbCapacityUl_;
    unsigned int fbhbCapacityD2D_;
    simtime_t lb_;
    simtime_t ub_;
    double pmiComputationWeight_;
    double cqiComputationWeight_;
    LteMuMimoMatrix muMimoDlMatrix_;
    LteMuMimoMatrix muMimoUlMatrix_;
    LteMuMimoMatrix muMimoD2DMatrix_;

    History_ *getHistory(Direction dir, double carrierFrequency);

  public:
    LteAmc(LteMacEnb *mac, Binder *binder, CellInfo *cellInfo, int numAntennas);
    LteAmc(const LteAmc& other) { operator=(other); }
    LteAmc& operator=(const LteAmc& other);
    void initialize();
    virtual ~LteAmc();
    void setfType(int f)
    {
        fType_ = f;
    }

    int getfType()
    {
        return fType_;
    }

    // CodeRate MCS rescaling
    void rescaleMcs(double rePerRb, Direction dir = DL);

    void pushFeedback(MacNodeId id, Direction dir, LteFeedback fb, double carrierFrequency);
    void pushFeedbackD2D(MacNodeId id, LteFeedback fb, MacNodeId peerId, double carrierFrequency);
    const LteSummaryFeedback& getFeedback(MacNodeId id, Remote antenna, TxMode txMode, const Direction dir, double carrierFrequency);
    const LteSummaryFeedback& getFeedbackD2D(MacNodeId id, Remote antenna, TxMode txMode, MacNodeId peerId, double carrierFrequency);

    //used when it is necessary to know if the requested feedback exists or not
    // LteSummaryFeedback getFeedback(MacNodeId id, Remote antenna, TxMode txMode, const Direction dir,bool& valid);

    MacNodeId computeMuMimoPairing(const MacNodeId nodeId, Direction dir = DL);

    const RemoteSet *getAntennaSet()
    {
        return &remoteSet_;
    }

    bool existTxParams(MacNodeId id, const Direction dir, double carrierFrequency);
    const UserTxParams& getTxParams(MacNodeId id, const Direction dir, double carrierFrequency);
    const UserTxParams& setTxParams(MacNodeId id, const Direction dir, UserTxParams& info, double carrierFrequency);
    const UserTxParams& computeTxParams(MacNodeId id, const Direction dir, double carrierFrequency);
    virtual unsigned int computeBitsOnNRbs(MacNodeId id, Band b, unsigned int blocks, const Direction dir, double carrierFrequency);
    virtual unsigned int computeBitsOnNRbs(MacNodeId id, Band b, Codeword cw, unsigned int blocks, const Direction dir, double carrierFrequency);
    virtual unsigned int computeBytesOnNRbs(MacNodeId id, Band b, unsigned int blocks, const Direction dir, double carrierFrequency);
    virtual unsigned int computeBytesOnNRbs(MacNodeId id, Band b, Codeword cw, unsigned int blocks, const Direction dir, double carrierFrequency);

    virtual unsigned int computeBitsPerRbBackground(Cqi cqi, const Direction dir, double carrierFrequency);

    // multiband version of the above function. It returns the number of bytes that can fit in the given "blocks" of the given "band"
    virtual unsigned int computeBytesOnNRbs_MB(MacNodeId id, Band b, unsigned int blocks, const Direction dir, double carrierFrequency);
    virtual unsigned int computeBitsOnNRbs_MB(MacNodeId id, Band b, unsigned int blocks, const Direction dir, double carrierFrequency);
    bool setPilotUsableBands(MacNodeId id, UsableBands usableBands);
    UsableBands *getPilotUsableBands(MacNodeId id);

    // utilities - do not involve pilot invocation
    unsigned int getItbsPerCqi(Cqi cqi, const Direction dir);

    /*
     * Access the correct itbs2tbs conversion table given cqi and layer number
     */
    const unsigned int *readTbsVect(Cqi cqi, unsigned int layers, Direction dir);

    /*
     * given <cqi> and <layers> returns bytes allocable in <blocks>
     */
    unsigned int blockGain(Cqi cqi, unsigned int layers, unsigned int blocks, Direction dir);

    /*
     * given <cqi> and <layers> returns blocks capable of carrying  <bytes>
     */
    unsigned int bytesGain(Cqi cqi, unsigned int layers, unsigned int bytes, Direction dir);

    // ---------------------------
    void writeCqiWeight(double weight);
    Cqi readWbCqi(const CqiVector& cqi);
    void writePmiWeight(double weight);
    Pmi readWbPmi(const PmiVector& pmi);
    void detachUser(MacNodeId nodeId, Direction dir);
    void attachUser(MacNodeId nodeId, Direction dir);
    void testUe(MacNodeId nodeId, Direction dir);
    AmcPilot *getPilot() const
    {
        return pilot_;
    }

    CellInfo *getCellInfo()
    {
        return cellInfo_;
    }

    inet::Coord getUePosition(MacNodeId id)
    {
        return cellInfo_->getUePosition(id);
    }

    void muMimoMatrixInit(Direction dir, MacNodeId nodeId)
    {
        if (dir == DL)
            muMimoDlMatrix_.initialize(nodeId);
        else if (dir == UL)
            muMimoUlMatrix_.initialize(nodeId);
        else if (dir == D2D)
            muMimoD2DMatrix_.initialize(nodeId);
    }

    void addMuMimoPair(Direction dir, MacNodeId id1, MacNodeId id2)
    {
        if (dir == DL)
            muMimoDlMatrix_.addPair(id1, id2);
        else if (dir == UL)
            muMimoUlMatrix_.addPair(id1, id2);
        else if (dir == D2D)
            muMimoD2DMatrix_.addPair(id1, id2);
    }

    std::vector<Cqi> readMultiBandCqi(MacNodeId id, const Direction dir, double carrierFrequency);

    int getSystemNumBands() { return numBands_; }

    void setPilotMode(PilotComputationModes mode);
};

} //namespace

#endif

