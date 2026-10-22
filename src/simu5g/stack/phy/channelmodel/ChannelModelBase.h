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
 * The channel model used to be phrased as "a link between me (the local radio's
 * position) and one remote endpoint", with Direction selecting -- all at once --
 * which endpoint was mobile, which antenna gains applied, and which noise figure
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
    // the radio the evaluation is at, except where a caller does not name the
    // other end (cellularLink() for DL leaves the base station NODEID_NONE).
    MacNodeId txId = NODEID_NONE;
    MacNodeId rxId = NODEID_NONE;
    inet::Coord txCoord;
    inet::Coord rxCoord;

    // When the transmission evaluated on the link started to arrive at the
    // receiver; for a what-if, the instant of the measurement.
    simtime_t startTime;

    // ---- channel state ----
    // linkKey is the link itself: LinkKey(txId, rxId). It indexes the
    // state the radio medium keeps per link and shares among every channel
    // model evaluating the link: whether it is in line of sight, its shadowing
    // and its Jakes fading paths.
    LinkKey linkKey;

    // stateNodeId is the link's mobile end -- the UE, or a D2D link's
    // transmitter. It distinguishes background UEs' links, which have no
    // shadowing. (The Rayleigh fading takes it but ignores it: its value is the
    // trace's, the same for every node.)
    MacNodeId stateNodeId = NODEID_NONE;

    // ---- radios ----
    // The link budget is not part of the link: the antenna gains, the cable loss
    // and the noise figure are those of the two radios. They are resolved where
    // the link is built -- the local one is the radio the evaluation is at --
    // and not from txId and rxId, which a frame evaluated as a broadcast leaves
    // unset.
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

    // Where the two radios of linkKey are, in the order of the key
    const inet::Coord& positionA() const { return txId == linkKey.a ? txCoord : rxCoord; }
    const inet::Coord& positionB() const { return txId == linkKey.a ? rxCoord : txCoord; }

    // How far the link has moved since its ends were at positionA0 and
    // positionB0: the farther of its two ends
    double displacementSince(const inet::Coord& positionA0, const inet::Coord& positionB0) const
    {
        return std::max(positionA().distance(positionA0), positionB().distance(positionB0));
    }
};

/**
 * Abstract base for the channel models of the radio medium.
 *
 * This is not a propagation model: it is the interface to the PHY link model
 * up to the SINR. A channel model is asked for received power (getRSRP), for
 * per-band SINR (getSINR, and computeSinr for a what-if), and to run a
 * reception (isReceptionSuccessful), whose packet error rate and decision are
 * the receiving radio's error model's and receiver's; it owns propagation,
 * fading and interference. Path loss (getAttenuation,
 * computePathLoss) is one ingredient among those, not the subject of the
 * class.
 *
 * The class and interface names are RAT-neutral: the concrete models differ
 * in which 3GPP propagation study supplies their formulas (the pathLossType
 * parameter), not in whether they serve an LTE or an NR carrier, and any of
 * them can be plugged into the medium's channelModelType or nrChannelModelType
 * slot through the IChannelModel interface -- the NR slot, for instance,
 * selects the Tr38901ChannelModel preset by default. Everything
 * technology-dependent -- carrier frequency, bandwidth, numerology -- is
 * read from the ComponentCarrier module rather than encoded in the class.
 *
 * The model is the radio medium's, shared by every radio of its RAT on its
 * carrier, so each evaluation entry point takes the radio the evaluation is at
 * as its first argument: the receiver of a frame (the base station, for the
 * uplink CQI of a UE's report). A what-if, a transmission that is not on the
 * medium (computeRsrp(), computeSinr()), takes the radio it would arrive at.
 */
class ChannelModelBase : public cSimpleModule
{
  protected:
    // Reference to Binder module
    inet::ModuleRefByPar<Binder> binder_;

    // The radio medium, whose registry holds the radios of all nodes
    inet::ModuleRefByPar<CellularRadioMedium> radioMedium_;

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

    /*
     * Compute the error probability of the transmitted packet according to CQI used, TX mode, and the received power
     * After that, it generates a random number to check if this packet will be corrupted or not
     *
     * @param radio the radio the evaluation is at: the receiver of a frame, or the
     *        base station computing a CQI
     * @param frame pointer to the packet
     * @param lteInfo pointer to the user control info
     * @param rsrpVector the per-band received power captured for this frame, when the
     *        caller already has it (the D2D one-to-many capture-effect path). Empty
     *        otherwise; models that do not need it ignore it.
     */
    virtual bool isReceptionSuccessful(IRadioEndpoint *radio, AirFrame *frame, UserControlInfo *lteInfo, const std::vector<double>& rsrpVector = {}) = 0;

    /*
     * Compute attenuation (path loss + optional shadowing) over a radio link.
     *
     * @param link the two endpoints and the channel-state key to evaluate against
     */
    virtual double getAttenuation(const RadioLink& link) = 0;
    /*
     * Compute the path-loss attenuation according to the selected scenario,
     * outside any link: outdoors, at an environment height of 1 m (the D2D
     * conflict graph's estimate)
     *
     * @param distance between UE and eNodeB
     * @param los line-of-sight flag
     */
    virtual double computePathLoss(double distance, double dbp, bool los) = 0;
    /*
     * Compute SINR for each band for user nodeId according to path loss, shadowing (optional), and multipath fading
     *
     * @param radio the radio the evaluation is at: the receiver of a frame, or the
     *        base station computing a CQI
     * @param frame pointer to the packet
     * @param lteInfo pointer to the user control info
     */
    virtual std::vector<double> getSINR(IRadioEndpoint *radio, AirFrame *frame, UserControlInfo *lteInfo) = 0;

    /*
     * What the receiver would receive of a transmission that is not on the
     * medium (a what-if: a cell's broadcast during cell selection, a CQI): the
     * transmission starts at its start time from its start position, and
     * arrives where the receiver is now. The received power per band, in dBm.
     *
     * @param receiver the radio the transmission would arrive at, which uses this model on the transmission's carrier
     * @param transmission the hypothetical transmission, never added to the medium
     */
    virtual std::vector<double> computeRsrp(IRadioEndpoint *receiver, const CellularTransmission& transmission) = 0;

    /*
     * As computeRsrp(), the SINR per band, in dB: the interference and the
     * noise at the receiver added.
     */
    virtual std::vector<double> computeSinr(IRadioEndpoint *receiver, const CellularTransmission& transmission) = 0;

    /*
     * The power per band, in dBm, with which a frame received at the receiver
     * arrived: its transmission's from where its transmitter was as it started,
     * to where the receiver was as the frame began to arrive (the radio's
     * arrival stamp), at the frame's start time -- a real reception, not a
     * what-if. So far for a D2D frame only (the capture effect of a
     * one-to-many D2D receiver ranks the frames of a slot by it).
     *
     * @param receiver the radio the frame arrived at, which uses this model on the frame's carrier
     * @param frame the received frame, with its control info
     */
    virtual std::vector<double> computeReceptionRsrp(IRadioEndpoint *receiver, AirFrame *frame) = 0;

    virtual bool isUplinkInterferenceEnabled() { return false; }
};

} //namespace

#endif

