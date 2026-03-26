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

#include "simu5g/common/cellInfo/CellInfo.h"

#include <inet/mobility/static/StationaryMobility.h>

#include "simu5g/world/radio/ChannelControl.h"
#include "simu5g/world/radio/ChannelAccess.h"

namespace simu5g {

using namespace omnetpp;

using namespace std;

Define_Module(CellInfo);

void CellInfo::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        pgnMinX_ = par("constraintAreaMinX");
        pgnMinY_ = par("constraintAreaMinY");
        pgnMaxX_ = par("constraintAreaMaxX");
        pgnMaxY_ = par("constraintAreaMaxY");

        eNbType_ = par("microCell").boolValue() ? MICRO_ENB : MACRO_ENB;
        rbyDl_ = par("rbyDl");
        rbyUl_ = par("rbyUl");
        rbxDl_ = par("rbxDl");
        rbxUl_ = par("rbxUl");
        rbPilotDl_ = par("rbPilotDl");
        rbPilotUl_ = par("rbPilotUl");
        signalDl_ = par("signalDl");
        signalUl_ = par("signalUl");
        binder_.reference(this, "binderModule", true);

        // get the total number of bands in the system
        totalBands_ = binder_->getTotalBands();

        numPreferredBands_ = par("numPreferredBands");

        cModule *host = inet::getContainingNode(this);

        // register the containing eNB to the binder
        cellId_ = MacNodeId(host->par("macCellId").intValue());
        ASSERT(cellId_ != MacNodeId(-1));  // i.e. already set programmatically

        // MCS scaling factor
        calculateMcsScale();
    }
}


void CellInfo::calculateMcsScale()
{
    // RB subcarriers * (TTI Symbols - Signalling Symbols) - pilot REs
    int ulRbSubcarriers = par("rbyUl");
    int dlRbSubCarriers = par("rbyDl");
    int ulRbSymbols = par("rbxUl");
    ulRbSymbols *= 2; // slot --> RB
    int dlRbSymbols = par("rbxDl");
    dlRbSymbols *= 2; // slot --> RB
    int ulSigSymbols = par("signalUl");
    int dlSigSymbols = par("signalDl");
    int ulPilotRe = par("rbPilotUl");
    int dlPilotRe = par("rbPilotDl");

    mcsScaleUl_ = ulRbSubcarriers * (ulRbSymbols - ulSigSymbols) - ulPilotRe;
    mcsScaleDl_ = dlRbSubCarriers * (dlRbSymbols - dlSigSymbols) - dlPilotRe;
}

// unused:
void CellInfo::updateMCSScale(double& mcs, double signalRe,
        double signalCarriers, Direction dir)
{
    // RB subcarriers * (TTI Symbols - Signalling Symbols) - pilot REs

    int rbSubcarriers = (dir == DL) ? par("rbyDl") : par("rbyUl");
    int rbSymbols = (dir == DL) ? par("rbxDl") : par("rbxUl");

    rbSymbols *= 2; // slot --> RB

    int sigSymbols = signalRe;
    int pilotRe = signalCarriers;

    mcs = rbSubcarriers * (rbSymbols - sigSymbols) - pilotRe;
}

void CellInfo::detachUser(MacNodeId nodeId)
{
    auto pt = uePosition.find(nodeId);
    if (pt != uePosition.end())
        uePosition.erase(pt);
}

void CellInfo::attachUser(MacNodeId nodeId)
{
}

unsigned int CellInfo::getNumBands()
{
    return numBands_;
}

unsigned int CellInfo::getPrimaryCarrierNumBands()
{
    unsigned int primaryCarrierNumBands = 0;
    if (!carrierMap_.empty())
        primaryCarrierNumBands = carrierMap_.begin()->second.numBands;

    return primaryCarrierNumBands;
}

unsigned int CellInfo::getCarrierNumBands(GHz carrierFrequency)
{
    auto it = carrierMap_.find(carrierFrequency);
    if (it == carrierMap_.end())
        throw cRuntimeError("CellInfo::getCarrierNumBands - Carrier %f is not used on node %hu", carrierFrequency.get(), num(cellId_));

    return it->second.numBands;
}

GHz CellInfo::getPrimaryCarrierFrequency()
{
    GHz primaryCarrierFrequency = GHz(0.0);
    if (!carrierMap_.empty())
        primaryCarrierFrequency = carrierMap_.begin()->first;

    return primaryCarrierFrequency;
}

void CellInfo::registerCarrier(GHz carrierFrequency, unsigned int carrierNumBands, unsigned int numerologyIndex, bool useTdd, unsigned int tddNumSymbolsDl, unsigned int tddNumSymbolsUl)
{
    auto it = carrierMap_.find(carrierFrequency);
    if (it != carrierMap_.end())
        throw cRuntimeError("CellInfo::registerCarrier - Carrier [%fGHz] already exists on node %hu", carrierFrequency.get(), num(cellId_));
    else {
        carriersVector_.push_back(carrierFrequency);

        CarrierInfo cInfo;
        cInfo.carrierFrequency = carrierFrequency;
        cInfo.numBands = carrierNumBands;
        cInfo.numerologyIndex = numerologyIndex;
        cInfo.slotFormat = computeSlotFormat(useTdd, tddNumSymbolsDl, tddNumSymbolsUl);
        carrierMap_[carrierFrequency] = cInfo;

        maxNumerologyIndex_ = numerologyIndex > maxNumerologyIndex_ ? numerologyIndex : maxNumerologyIndex_;

        // update total number of bands in this cell
        numBands_ += carrierNumBands;

        EV << "CellInfo::registerCarrier - Registered carrier @ " << carrierFrequency << "GHz" << endl;

        // update carriers' bounds
        unsigned int b = 0;
        for (auto& [cKey, carrierInfo] : carrierMap_) {
            carrierInfo.firstBand = b;
            carrierInfo.lastBand = b + carrierInfo.numBands - 1;
            b = carrierInfo.lastBand + 1;
            EV << "* [" << cKey << "GHz] - range[" << carrierInfo.firstBand << "-" << carrierInfo.lastBand << "]" << endl;

            // --- create usable bands structure --- //

            carrierInfo.bandLimit.clear();

            // for each band of the band vector provided
            for (unsigned int i = 0; i < numBands_; i++) {
                BandLimit elem;
                elem.band_ = Band(i);

                // check whether band i is in the set of usable bands
                int limit = (i >= carrierInfo.firstBand && i <= carrierInfo.lastBand) ? -1 : -2;

                elem.limit_.clear();
                for (unsigned int j = 0; j < MAX_CODEWORDS; j++)
                    elem.limit_.push_back(limit);

                carrierInfo.bandLimit.push_back(elem);
            }
        }
    }
}

const std::vector<GHz>& CellInfo::getCarriers()
{
    return carriersVector_;
}

const CarrierInfoMap& CellInfo::getCarrierInfoMap()
{
    return carrierMap_;
}

unsigned int CellInfo::getCellwiseBand(GHz carrierFrequency, Band index)
{
    auto it = carrierMap_.find(carrierFrequency);
    if (it == carrierMap_.end())
        throw cRuntimeError("CellInfo::getCellwiseBand - Carrier %f is not used on node %hu", carrierFrequency.get(), num(cellId_));

    if (index > it->second.numBands)
        throw cRuntimeError("CellInfo::getCellwiseBand - Selected band [%d] is greater than the number of available bands on this carrier [%d]", index, it->second.numBands);

    return it->second.firstBand + index;
}

const BandLimitVector& CellInfo::getCarrierBandLimit(GHz carrierFrequency)
{
    if (carrierFrequency == GHz(0.0)) {
        auto it = carrierMap_.begin();
        if (it != carrierMap_.end())
            return it->second.bandLimit;
    }
    auto it = carrierMap_.find(carrierFrequency);
    if (it == carrierMap_.end())
        throw cRuntimeError("CellInfo::getCarrierBandLimit - Carrier %f is not used on node %hu", carrierFrequency.get(), num(cellId_));

    return it->second.bandLimit;
}

unsigned int CellInfo::getCarrierStartingBand(GHz carrierFrequency)
{
    auto it = carrierMap_.find(carrierFrequency);
    if (it == carrierMap_.end())
        throw cRuntimeError("CellInfo::getCarrierStartingBand - Carrier [%fGHz] not found", carrierFrequency.get());

    return it->second.firstBand;
}

unsigned int CellInfo::getCarrierLastBand(GHz carrierFrequency)
{
    auto it = carrierMap_.find(carrierFrequency);
    if (it == carrierMap_.end())
        throw cRuntimeError("CellInfo::getCarrierStartingBand - Carrier [%fGHz] not found", carrierFrequency.get());

    return it->second.lastBand;
}

SlotFormat CellInfo::computeSlotFormat(bool useTdd, unsigned int tddNumSymbolsDl, unsigned int tddNumSymbolsUl)
{
    SlotFormat sf;
    if (!useTdd) {
        sf.tdd = false;

        // these values are not used when TDD is false
        sf.numDlSymbols = 0;
        sf.numUlSymbols = 0;
        sf.numFlexSymbols = 0;
    }
    else {
        sf.tdd = true;
        unsigned int numSymbols = rbxDl_ * 2;
        if (tddNumSymbolsDl + tddNumSymbolsUl > numSymbols)
            throw cRuntimeError("CellInfo::computeSlotFormat - Number of symbols not valid - DL[%d] UL[%d]", tddNumSymbolsDl, tddNumSymbolsUl);

        sf.numDlSymbols = tddNumSymbolsDl;
        sf.numUlSymbols = tddNumSymbolsUl;
        sf.numFlexSymbols = numSymbols - tddNumSymbolsDl - tddNumSymbolsUl;
    }
    return sf;
}

} //namespace
