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

#include "stack/mac/amc/AmcPilotAuto.h"

namespace simu5g {

using namespace inet;

const UserTxParams& AmcPilotAuto::computeTxParams(MacNodeId id, const Direction dir, double carrierFrequency)
{
    EV << NOW << " AmcPilot" << getName() << "::computeTxParams for UE " << id << ", direction " << dirToA(dir) << endl;

    // Check if user transmission parameters have been already allocated
    if (amc_->existTxParams(id, dir, carrierFrequency)) {
        EV << NOW << " AmcPilot" << getName() << "::computeTxParams The information for this user has already been assigned \n";
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
    const LteSummaryFeedback& sfb = amc_->getFeedback(id, MACRO, txMode, dir, carrierFrequency);

    if (TxMode(txMode) == MULTI_USER) // Initialize MuMiMoMatrix
        amc_->muMimoMatrixInit(dir, id);

    sfb.print(NODEID_NONE, id, dir, txMode, "AmcPilotAuto::computeTxParams");

    // get a vector of  CQI over first CW
    std::vector<Cqi> summaryCqi = sfb.getCqi(0);

    // get the usable bands for this user
    UsableBands *usableB = getUsableBands(id);

    Band chosenBand = 0;
    double chosenCqi = 0;
    BandSet bandSet;

    /// TODO collapse the following part into a single part (e.g. do not fork on mode_ or usableBandsList_ size)

    // check which CQI computation policy is to be used
    if (mode_ == MAX_CQI) {
        // if there are no usable bands, compute the final CQI through all the bands
        if (usableB == nullptr || usableB->empty()) {
            chosenBand = 0;
            chosenCqi = summaryCqi.at(chosenBand);
            unsigned int bands = summaryCqi.size();// number of bands
            // computing MAX
            for (Band b = 1; b < bands; ++b) {
                // For all Bands
                double s = (double)summaryCqi.at(b);
                if (chosenCqi < s) {
                    chosenBand = b;
                    chosenCqi = s;
                }

                Band cellWiseBand = amc_->getCellInfo()->getCellwiseBand(carrierFrequency, b);
                bandSet.insert(cellWiseBand);
            }
            EV << NOW << " AmcPilotAuto::computeTxParams - no UsableBand available for this user." << endl;
        }
        else {
            // TODO Add MIN and MEAN cqi computation methods
            chosenBand = (*usableB)[0];
            chosenCqi = summaryCqi.at(chosenBand);
            for (Band currBand : *usableB) {
                // For all available band
                double s = (double)summaryCqi.at(currBand);
                if (chosenCqi < s) {
                    chosenBand = currBand;
                    chosenCqi = s;
                }
                Band cellWiseBand = amc_->getCellInfo()->getCellwiseBand(carrierFrequency, currBand);
                bandSet.insert(cellWiseBand);
            }
            EV << NOW << " AmcPilotAuto::computeTxParams - UsableBand of size " << usableB->size() << " available for this user" << endl;
        }
    }
    else if (mode_ == MIN_CQI) {
        // if there are no usable bands, compute the final CQI through all the bands
        if (usableB == nullptr || usableB->empty()) {
            chosenBand = 0;
            chosenCqi = summaryCqi.at(chosenBand);
            unsigned int bands = summaryCqi.size();// number of bands
            // computing MIN
            for (Band b = 1; b < bands; ++b) {
                // For all LBs
                double s = (double)summaryCqi.at(b);
                if (chosenCqi > s) {
                    chosenBand = b;
                    chosenCqi = s;
                }
                Band cellWiseBand = amc_->getCellInfo()->getCellwiseBand(carrierFrequency, b);
                bandSet.insert(cellWiseBand);
            }
            EV << NOW << " AmcPilotAuto::computeTxParams - no UsableBand available for this user." << endl;
        }
        else {
            chosenBand = (*usableB)[0];
            chosenCqi = summaryCqi.at(chosenBand);
            for (Band currBand : *usableB) {
                // For all available band
                double s = (double)summaryCqi.at(currBand);
                if (chosenCqi > s) {
                    chosenBand = currBand;
                    chosenCqi = s;
                }
                Band cellWiseBand = amc_->getCellInfo()->getCellwiseBand(carrierFrequency, currBand);
                bandSet.insert(cellWiseBand);
            }

            EV << NOW << " AmcPilotAuto::computeTxParams - UsableBand of size " << usableB->size() << " available for this user" << endl;
        }
    }
    else if (mode_ == ROBUST_CQI) {
        int target = 0;
        int s;
        unsigned int bands = summaryCqi.size();// number of bands

        EV << "AmcPilotAuto::computeTxParams - computing ROBUST CQI" << endl;

        // computing MIN
        for (Band b = 0; b < bands; ++b) {
            // For all LBs
            s = summaryCqi.at(b);
            target += s;
        }
        target = target / bands;

        EV << "\t target value[" << target << "]" << endl;

        for (Band b = 0; b < bands; ++b) {
            if (summaryCqi.at(b) >= target) {
                EV << b << ")" << summaryCqi.at(b) << "yes" << endl;
                Band cellWiseBand = amc_->getCellInfo()->getCellwiseBand(carrierFrequency, b);
                bandSet.insert(cellWiseBand);
            }
            else
                EV << b << ")" << summaryCqi.at(b) << "no" << endl;
        }
        chosenBand = 0;
        chosenCqi = target;
    }
    else if (mode_ == AVG_CQI) {
        // MEAN cqi computation method
        chosenCqi = binder_->meanCqi(sfb.getCqi(0), id, dir);
        for (Band i = 0; i < sfb.getCqi(0).size(); ++i) {
            Band cellWiseBand = amc_->getCellInfo()->getCellwiseBand(carrierFrequency, i);
            bandSet.insert(cellWiseBand);
        }
        chosenBand = 0;
    }
    else if (mode_ == MEDIAN_CQI) {
        // MEAN cqi computation method
        chosenCqi = binder_->medianCqi(sfb.getCqi(0), id, dir);
        for (Band i = 0; i < sfb.getCqi(0).size(); ++i) {
            Band cellWiseBand = amc_->getCellInfo()->getCellwiseBand(carrierFrequency, i);
            bandSet.insert(cellWiseBand);
        }
        chosenBand = 0;
    }

    // Set user transmission parameters only for the best band
    UserTxParams info;
    info.writeTxMode(txMode);
    info.writeRank(sfb.getRi());
    info.writeCqi(std::vector<Cqi>(1, chosenCqi));
    info.writePmi(sfb.getPmi(chosenBand));
    info.writeBands(bandSet);
    RemoteSet antennas;
    antennas.insert(MACRO);
    info.writeAntennas(antennas);

    // DEBUG
    EV << NOW << " AmcPilot" << getName() << "::computeTxParams NEW values assigned! - CQI =" << chosenCqi << "\n";
    info.print("AmcPilotAuto::computeTxParams");

    return amc_->setTxParams(id, dir, info, carrierFrequency);
}

std::vector<Cqi> AmcPilotAuto::getMultiBandCqi(MacNodeId id, const Direction dir, double carrierFrequency)
{
    EV << NOW << " AmcPilot" << getName() << "::getMultiBandCqi for UE " << id << ", direction " << dirToA(dir) << endl;

    // TODO make it configurable from NED
    // default transmission mode
    TxMode txMode = TRANSMIT_DIVERSITY;

    /**
     *  Select the band which has the best summary
     *  Note: this pilot is not DAS aware, so only MACRO antenna
     *  is used.
     */
    const LteSummaryFeedback& sfb = amc_->getFeedback(id, MACRO, txMode, dir, carrierFrequency);

    // get a vector of  CQI over first CW
    return sfb.getCqi(0);
}

void AmcPilotAuto::setUsableBands(MacNodeId id, UsableBands usableBands)
{
    EV << NOW << " AmcPilotAuto::setUsableBands - setting usable bands: for node " << id << " [";
    for (unsigned short usableBand : usableBands) {
        EV << usableBand << ",";
    }
    EV << "]" << endl;
    UsableBandsList::iterator it = usableBandsList_.find(id);

    // if usable bands for this node are already set, delete it (probably unnecessary)
    if (it != usableBandsList_.end())
        usableBandsList_.erase(id);
    usableBandsList_.insert({id, usableBands});
}

UsableBands *AmcPilotAuto::getUsableBands(MacNodeId id)
{
    EV << NOW << " AmcPilotAuto::getUsableBands - getting usable bands for node " << id;

    bool found = false;
    UsableBandsList::iterator it = usableBandsList_.find(id);
    if (it != usableBandsList_.end()) {
        found = true;
    }
    else {
        // usable bands for this id not found
        if (getNodeTypeById(id) == UE) {
            // if it is a UE, look for its serving cell
            MacNodeId cellId = binder_->getNextHop(id);
            it = usableBandsList_.find(cellId);
            if (it != usableBandsList_.end())
                found = true;
        }
    }

    if (found) {
        EV << " [";
        for (Band i : it->second) {
            EV << i << ",";
        }
        EV << "]" << endl;

        return &(it->second);
    }

    EV << " [All bands are usable]" << endl;
    return nullptr;
}

} //namespace

