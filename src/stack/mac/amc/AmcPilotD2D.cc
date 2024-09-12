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

#include "stack/mac/amc/AmcPilotD2D.h"

namespace simu5g {

using namespace inet;

void AmcPilotD2D::setPreconfiguredTxParams(Cqi cqi)
{
    usePreconfiguredTxParams_ = true;
    preconfiguredTxParams_ = new UserTxParams();

    // default parameters for D2D
    preconfiguredTxParams_->isSet() = true;
    preconfiguredTxParams_->writeTxMode(TRANSMIT_DIVERSITY);
    Rank ri = 1;                                              // rank for TxD is one
    preconfiguredTxParams_->writeRank(ri);
    preconfiguredTxParams_->writePmi(intuniform(getEnvir()->getRNG(0), 1, pow(ri, (double)2)));   // taken from LteFeedbackComputationRealistic::computeFeedback

    if (cqi < 0 || cqi > 15)
        throw cRuntimeError("AmcPilotD2D::setPreconfiguredTxParams - CQI %hu is not a valid value. Aborting", cqi);
    preconfiguredTxParams_->writeCqi(std::vector<Cqi>(1, cqi));

    BandSet b;
    for (Band i = 0; i < binder_->getTotalBands(); ++i)
        b.insert(i);

    preconfiguredTxParams_->writeBands(b);

    RemoteSet antennas;
    antennas.insert(MACRO);
    preconfiguredTxParams_->writeAntennas(antennas);
}

const UserTxParams& AmcPilotD2D::computeTxParams(MacNodeId id, const Direction dir, double carrierFrequency)
{
    EV << NOW << " AmcPilot" << getName() << "::computeTxParams for UE " << id << ", direction " << dirToA(dir) << endl;

    if ((dir == D2D || dir == D2D_MULTI) && usePreconfiguredTxParams_) {
        EV << NOW << " AmcPilot" << getName() << "::computeTxParams Use preconfigured Tx params for D2D connections" << endl;
        return *preconfiguredTxParams_;
    }

    // Check if user transmission parameters have been already allocated
    if (amc_->existTxParams(id, dir, carrierFrequency)) {
        EV << NOW << " AmcPilot" << getName() << "::computeTxParams The information for this user has already been assigned" << endl;
        return amc_->getTxParams(id, dir, carrierFrequency);
    }
    // TODO make it configurable from NED
    // default transmission mode
    TxMode txMode = TRANSMIT_DIVERSITY;

    /**
     *  Select the band which has the best summary
     *  Note: this pilot is not DAS aware, so only MACRO antenna
     *  is used.
     */

    // FIXME The eNodeB knows the CQI for N D2D links originating from this UE, but it does not
    // know which UE will be addressed in this transmission. Which feedback should it select?
    // This is an open issue.
    // Here, we will get the feedback of the first peer in the map. However, algorithms may be
    // designed to solve this problem.

    MacNodeId peerId = NODEID_NONE;  // FIXME this way, the getFeedbackD2D() function will return the first feedback available

    const LteSummaryFeedback& sfb = (dir == UL || dir == DL) ? amc_->getFeedback(id, MACRO, txMode, dir, carrierFrequency) : amc_->getFeedbackD2D(id, MACRO, txMode, peerId, carrierFrequency);

    if (TxMode(txMode) == MULTI_USER) // Initialize MuMiMoMatrix
        amc_->muMimoMatrixInit(dir, id);

    sfb.print(NODEID_NONE, id, dir, txMode, "AmcPilotD2D::computeTxParams");

    // get a vector of  CQI over first CW
    std::vector<Cqi> summaryCqi = sfb.getCqi(0);

    Cqi chosenCqi;
    BandSet b;
    if (mode_ == AVG_CQI) {
        // MEAN cqi computation method
        chosenCqi = binder_->meanCqi(sfb.getCqi(0), id, dir);
        for (Band i = 0; i < sfb.getCqi(0).size(); ++i) {
            Band cellWiseBand = amc_->getCellInfo()->getCellwiseBand(carrierFrequency, i);
            b.insert(cellWiseBand);
        }
    }
    else {
        // MIN/MAX cqi computation method
        Band band = 0;

        // translate carrier-local band index to cell-wise band index
        Band cellWiseBand = amc_->getCellInfo()->getCellwiseBand(carrierFrequency, band);
        chosenCqi = summaryCqi.at(band);
        unsigned int bands = summaryCqi.size();// number of bands
        for (Band i = 1; i < bands; ++i) {
            // For all LBs
            double s = (double)summaryCqi.at(i);
            if ((mode_ == MIN_CQI && s < chosenCqi) || (mode_ == MAX_CQI && s > chosenCqi)) {
                band = i;
                chosenCqi = s;
            }

            // add (all) bands to the bandset
            // TODO check if you want to add only the bands with CQI equal to the MIN/MAX
            cellWiseBand++;
            b.insert(cellWiseBand);
        }
    }

    // Set user transmission parameters
    UserTxParams info;
    info.writeTxMode(txMode);
    info.writeRank(sfb.getRi());
    info.writeCqi(std::vector<Cqi>(1, chosenCqi));
    info.writePmi(sfb.getPmi(0));
    info.writeBands(b);
    RemoteSet antennas;
    antennas.insert(MACRO);
    info.writeAntennas(antennas);

    // DEBUG
    EV << NOW << " AmcPilot" << getName() << "::computeTxParams NEW values assigned! - CQI =" << chosenCqi << "\n";
    info.print("AmcPilotD2D::computeTxParams");

    //return amc_->setTxParams(id, dir, info,user_type); OLD solution
    // Debug
    const UserTxParams& info2 = amc_->setTxParams(id, dir, info, carrierFrequency);

    return info2;
}

} //namespace

