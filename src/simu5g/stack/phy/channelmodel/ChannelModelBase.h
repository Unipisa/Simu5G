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

#ifndef STACK_PHY_CHANNELMODEL_CHANNELMODELBASE_H_
#define STACK_PHY_CHANNELMODEL_CHANNELMODELBASE_H_

#include <inet/common/ModuleRefByPar.h>

#include "simu5g/common/LteCommon.h"
#include "simu5g/common/LteControlInfo.h"
#include "simu5g/common/carrierAggregation/ComponentCarrier.h"
#include "simu5g/common/binder/Binder.h"
#include "simu5g/common/cellInfo/CellInfo.h"
#include "simu5g/stack/phy/packet/AirFrame_m.h"
#include "simu5g/stack/phy/channelmodel/IRadioEndpoint.h"
#include "simu5g/stack/phy/medium/CellularRadioMedium.h"

namespace simu5g {

using namespace inet;
using namespace omnetpp;

class AirFrame;
class Binder;

/**
 * A radio link between two arbitrary endpoints, and the parameters the channel
 * model needs to evaluate it.
 *
 * The channel model used to be phrased as "a link between me (phy_->getCoord())
 * and one remote endpoint", with Direction selecting -- all at once -- which
 * endpoint was mobile, which antenna gains applied, and which noise figure
 * applied. A UE-to-UE link fits neither of those two shapes, which is why the
 * D2D channel model had to re-implement the whole propagation path rather than
 * reuse it.
 *
 * Here those things are data. `dir` survives only as a tag: it selects the
 * statistic to emit and dispatches the (genuinely cellular-topology-aware)
 * interference computation, but it no longer derives any of the link geometry
 * or the link budget.
 */
struct RadioLink
{
    // ---- geometry ----
    // The node ids of the two radios. The builders name both, the local one by
    // the evaluating model's own radio, except where a caller does not name the
    // other end (cellularLink() for DL leaves the base station NODEID_NONE).
    MacNodeId txId = NODEID_NONE;
    MacNodeId rxId = NODEID_NONE;
    inet::Coord txCoord;
    inet::Coord rxCoord;

    // ---- channel state ----
    // linkKey is the link itself: LinkKey(txId, rxId). It indexes the
    // state the radio medium keeps per link and shares among every channel
    // model evaluating the link: whether it is in line of sight, its shadowing
    // and its Jakes fading paths.
    LinkKey linkKey;

    // stateKey indexes the per-link entries of the evaluating channel model's
    // own channel state (ChannelState): the correlation point.
    LinkKey stateKey;

    // stateNodeId is the *node* the state belongs to -- the UE. It indexes the
    // position history, which is genuinely a node property because it defines
    // the node's speed, and it distinguishes background UEs.
    MacNodeId stateNodeId = NODEID_NONE;

    inet::Coord stateCoord;      // position feeding computeSpeed + correlation distance

    // ---- radios ----
    // The link budget is not part of the link: the antenna gains, the cable loss
    // and the noise figure are those of the two radios. They are resolved where
    // the link is built -- the local one is the evaluating model's own -- and not
    // from txId and rxId, which a frame evaluated as a broadcast leaves unset.
    IRadioEndpoint *txRadio = nullptr;
    IRadioEndpoint *rxRadio = nullptr;
    bool txIsBaseStation = false;   // gates angular attenuation

    // The cell this link belongs to. Only the interference computation needs it --
    // that model is genuinely cellular-topology-aware, since it asks which cell an
    // interferer is in. For a cellular link it is the base-station endpoint; for a
    // UE-to-UE link it is the transmitter's serving cell.
    MacNodeId cellId = NODEID_NONE;

    // ---- tag, not a switch ----
    Direction dir = UNKNOWN_DIRECTION;
};

/**
 * Abstract base for the channel models a PHY layer can be equipped with.
 *
 * This is not a propagation model: it is the interface to the entire PHY link
 * model. A channel model is asked for received power (getRSRP), for per-band
 * SINR (getSINR, getSINR_bgUe), and for the reception decision itself
 * (isReceptionSuccessful), so it owns propagation, fading, interference and
 * the SINR-to-error mapping alike. Path loss (getAttenuation,
 * computePathLoss) is one ingredient among those, not the subject of the
 * class.
 *
 * The class and interface names are RAT-neutral: the concrete models differ
 * in which 3GPP propagation study supplies their formulas (the pathLossType
 * parameter), not in whether they serve an LTE or an NR carrier, and any of
 * them can be plugged into the channelModelType slot of any NIC through the
 * IChannelModel interface -- the gNodeB NIC, for instance, selects the
 * Tr38901ChannelModel preset there by default. Everything
 * technology-dependent -- carrier frequency, bandwidth, numerology -- is
 * read from the ComponentCarrier module rather than encoded in the class.
 */
class ChannelModelBase : public cSimpleModule
{
  protected:
    // Reference to Binder module
    inet::ModuleRefByPar<Binder> binder_;

    // The radio medium, whose registry holds the radios of all nodes
    inet::ModuleRefByPar<CellularRadioMedium> radioMedium_;

    // Reference to cell info module
    inet::ModuleRefByPar<CellInfo> cellInfo_;

    // The radio endpoint this channel model belongs to -- in a running model,
    // its node's PHY. Everything the channel model needs from it is in
    // IRadioEndpoint, which is what lets a test put a stub here. A plain pointer
    // rather than an opp_component_ptr, because an interface is not a
    // cComponent; the PHY and its channel models are submodules of the same NIC
    // and are torn down together, so the pointer cannot outlive its target.
    IRadioEndpoint *phy_ = nullptr;

    // Reference to the component carrier
    inet::ModuleRefByPar<ComponentCarrier> componentCarrier_;

    // Carrier Frequency and its base-10 logarithm
    GHz carrierFrequency_;
    double carrierFrequencyHz_;
    double carrierFrequencyGHz_;
    double log10CarrierFrequencyGHz_;

    // Number of bands for this carrier
    unsigned int numBands_ = -1;

  public:

    void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }

    /*
     * Returns the carrier frequency
     */
    virtual GHz getCarrierFrequency() const { return GHz(carrierFrequencyGHz_); }

    /*
     * Returns the number of logical bands
     */
    virtual unsigned int getNumBands() const { return numBands_; }

    /*
     * Returns the numerology index
     */
    virtual unsigned int getNumerologyIndex() const { return componentCarrier_->getNumerologyIndex(); }

    virtual void setPhy(IRadioEndpoint *phy) { phy_ = phy; }

    /*
     * Compute the error probability of the transmitted packet according to CQI used, TX mode, and the received power
     * After that, it generates a random number to check if this packet will be corrupted or not
     *
     * @param frame pointer to the packet
     * @param lteInfo pointer to the user control info
     * @param rsrpVector the per-band received power captured for this frame, when the
     *        caller already has it (the D2D one-to-many capture-effect path). Empty
     *        otherwise; models that do not need it ignore it.
     */
    virtual bool isReceptionSuccessful(AirFrame *frame, UserControlInfo *lteInfo, const std::vector<double>& rsrpVector = {}) = 0;

    /*
     * Compute attenuation (path loss + optional shadowing) over a radio link.
     *
     * @param link the two endpoints and the channel-state key to evaluate against
     */
    virtual double getAttenuation(const RadioLink& link) = 0;
    /*
     * Compute the path-loss attenuation according to the selected scenario
     *
     * @param distance between UE and eNodeB
     * @param los line-of-sight flag
     */
    virtual double computePathLoss(double distance, double dbp, bool los) = 0;
    /*
     * Compute SINR for each band for user nodeId according to path loss, shadowing (optional), and multipath fading
     *
     * @param frame pointer to the packet
     * @param lteInfo pointer to the user control info
     */
    virtual std::vector<double> getSINR(AirFrame *frame, UserControlInfo *lteInfo) = 0;
    /*
     * Compute SINR for each band for a background UE according to path loss
     *
     * @param frame pointer to the packet
     * @param lteInfo pointer to the user control info
     */
    virtual std::vector<double> getSINR_bgUe(AirFrame *frame, UserControlInfo *lteInfo) = 0;

    /*
     * Compute received useful signal for each band for user nodeId according to path loss, shadowing (optional), and multipath fading
     *
     * @param frame pointer to the packet
     * @param lteInfo pointer to the user control info
     */
    virtual std::vector<double> getRSRP(AirFrame *frame, UserControlInfo *lteInfo) = 0;

    virtual bool isUplinkInterferenceEnabled() { return false; }

    /// Whether transmissions must be recorded in the Binder's UL transmission map
    /// (used by interference computation on the receive path).
    virtual bool recordsUlTransmissionMap() { return isUplinkInterferenceEnabled(); }
};

} //namespace

#endif

