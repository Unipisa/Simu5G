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
#include <inet/mobility/static/StationaryMobility.h>
#include "common/cellInfo/CellInfo.h"
#include "world/radio/ChannelControl.h"
#include "world/radio/ChannelAccess.h"

using namespace std;

Define_Module(CellInfo);

CellInfo::CellInfo()
{
    nodeX_ = 0;
    nodeY_ = 0;
    nodeZ_ = 0;
    binder_ = nullptr;
    mcsScaleDl_ = 0;
    mcsScaleUl_ = 0;
    numRus_ = 0;
    ruSet_ = new RemoteAntennaSet();
    binder_ = getBinder();
    numBands_ = 0;
    maxNumerologyIndex_ = 0;
}

CellInfo::~CellInfo()
{
    binder_ = nullptr;
    delete ruSet_;
}


void CellInfo::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL)
    {
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
    }
    if (stage == inet::INITSTAGE_LOCAL+1)
    {
        // get the total number of bands in the system
        totalBands_ = binder_->getTotalBands();

        numRus_ = par("numRus");

        numPreferredBands_ = par("numPreferredBands");

        if (numRus_ > NUM_RUS)
            throw omnetpp::cRuntimeError("The number of Antennas specified exceeds the limit of %d", NUM_RUS);

        // register the containing eNB  to the binder
        cellId_ = getParentModule()->par("macCellId");

        int ruRange = par("ruRange");
        double nodebTxPower = getAncestorPar("txPower");

        // first RU to be registered is the MACRO
        ruSet_->addRemoteAntenna(nodeX_, nodeY_, nodebTxPower);

        // REFACTORING: has no effect, as long as numRus_ == 0
        // deploy RUs
        deployRu(nodeX_, nodeY_, numRus_, ruRange);

        // MCS scaling factor
        calculateMCSScale(&mcsScaleUl_, &mcsScaleDl_);

        createAntennaCwMap();
    }
}



void CellInfo::calculateNodePosition(double centerX, double centerY, int nTh,
    int totalNodes, int range, double startingAngle, double *xPos,
    double *yPos)
{
    if (totalNodes == 0)
        error("CellInfo::calculateNodePosition: divide by 0");
    // radians (minus sign because position 0,0 is top-left, not bottom-left)
    double theta = -startingAngle * M_PI / 180;

    double thetaSpacing = (2 * M_PI) / totalNodes;
    // angle of n-th node
    theta += nTh * thetaSpacing;
    double x = centerX + (range * cos(theta));
    double y = centerY + (range * sin(theta));

    *xPos = (x < pgnMinX_) ? pgnMinX_ : (x > pgnMaxX_) ? pgnMaxX_ : x;
    *yPos = (y < pgnMinY_) ? pgnMinY_ : (y > pgnMaxY_) ? pgnMaxY_ : y;

    EV << NOW << " CellInfo::calculateNodePosition: Computed node position "
       << *xPos << " , " << *yPos << std::endl;

    return;
}


void CellInfo::deployRu(double nodeX, double nodeY, int numRu, int ruRange)
{
    if (numRu == 0)
        return;
    double x = 0;
    double y = 0;
    double angle = par("ruStartingAngle");
    std::string txPowersString = par("ruTxPower");
    int *txPowers = new int[numRu];
    parseStringToIntArray(txPowersString, txPowers, numRu, 0);
    for (int i = 0; i < numRu; i++)
    {
        calculateNodePosition(nodeX, nodeY, i, numRu, ruRange, angle, &x, &y);
        ruSet_->addRemoteAntenna(x, y, (double) txPowers[i]);
    }
    delete[] txPowers;
}

void CellInfo::calculateMCSScale(double *mcsUl, double *mcsDl)
{
    // RBsubcarriers * (TTISymbols - SignallingSymbols) - pilotREs
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

    *mcsUl = ulRbSubcarriers * (ulRbSymbols - ulSigSymbols) - ulPilotRe;
    *mcsDl = dlRbSubCarriers * (dlRbSymbols - dlSigSymbols) - dlPilotRe;
    return;
}

void CellInfo::updateMCSScale(double *mcs, double signalRe,
    double signalCarriers, Direction dir)
{
    // RBsubcarriers * (TTISymbols - SignallingSymbols) - pilotREs

    int rbSubcarriers = ((dir == DL) ? par("rbyDl") : par("rbyUl"));
    int rbSymbols = ((dir == DL) ? par("rbxDl") : par("rbxUl"));

    rbSymbols *= 2; // slot --> RB

    int sigSymbols = signalRe;
    int pilotRe = signalCarriers;

    *mcs = rbSubcarriers * (rbSymbols - sigSymbols) - pilotRe;
    return;
}

void CellInfo::createAntennaCwMap()
{
    std::string cws = par("antennaCws");
    // values for the RUs including the MACRO
    int dim = numRus_ + 1;
    int *values = new int[dim];
    // default for missing values is 1
    parseStringToIntArray(cws, values, dim, 1);
    for (int i = 0; i < dim; i++)
    {
        antennaCws_[(Remote) i] = values[i];
    }
    delete[] values;
}

void CellInfo::detachUser(MacNodeId nodeId)
{
    std::map<MacNodeId, inet::Coord>::iterator pt = uePosition.find(nodeId);
    if (pt != uePosition.end())
        uePosition.erase(pt);

    std::map<MacNodeId, Lambda>::iterator lt = lambdaMap_.find(nodeId);
    if (lt != lambdaMap_.end())
        lambdaMap_.erase(lt);
}

void CellInfo::attachUser(MacNodeId nodeId)
{
    // add UE to cellInfo's structures (lambda maps)
    // position will be added by the eNB while computing feedback

    int index = intuniform(0, binder_->phyPisaData.maxChannel() - 1);
    lambdaInit(nodeId, index);
}

unsigned int CellInfo::getNumBands()
{
    return numBands_;
}

unsigned int CellInfo::getPrimaryCarrierNumBands()
{
    unsigned int primaryCarrierNumBands = 0;
    CarrierInfoMap::iterator it = carrierMap_.begin();
    if (it != carrierMap_.end())
        primaryCarrierNumBands = it->second.numBands;

    return primaryCarrierNumBands;
}

unsigned int CellInfo::getCarrierNumBands(double carrierFrequency)
{
    CarrierInfoMap::iterator it = carrierMap_.find(carrierFrequency);
    if (it == carrierMap_.end())
        throw omnetpp::cRuntimeError("CellInfo::getCarrierNumBands - Carrier %f is not used on node %d", carrierFrequency, cellId_);

    return it->second.numBands;
}

double CellInfo::getPrimaryCarrierFrequency()
{
    double primaryCarrierFrequency = 0.0;
    CarrierInfoMap::iterator it = carrierMap_.begin();
    if (it != carrierMap_.end())
        primaryCarrierFrequency = it->first;

    return primaryCarrierFrequency;
}

void CellInfo::registerCarrier(double carrierFrequency, unsigned int carrierNumBands, unsigned int numerologyIndex, bool useTdd, unsigned int tddNumSymbolsDl, unsigned int tddNumSymbolsUl)
{
    CarrierInfoMap::iterator it = carrierMap_.find(carrierFrequency);
    if (it != carrierMap_.end())
        throw omnetpp::cRuntimeError("CellInfo::registerCarrier - Carrier [%fGHz] already exists on node %d", carrierFrequency, cellId_);
    else
    {
        carriersVector_.push_back(carrierFrequency);

        CarrierInfo cInfo;
        cInfo.carrierFrequency = carrierFrequency;
        cInfo.numBands = carrierNumBands;
        cInfo.numerologyIndex = numerologyIndex;
        cInfo.slotFormat = computeSlotFormat(useTdd,tddNumSymbolsDl,tddNumSymbolsUl);
        carrierMap_[carrierFrequency] = cInfo;

        maxNumerologyIndex_ = numerologyIndex > maxNumerologyIndex_ ? numerologyIndex : maxNumerologyIndex_;

        // update total number of bands in this cell
        numBands_ += carrierNumBands;

        EV << "CellInfo::registerCarrier - Registered carrier @ " << carrierFrequency << "GHz" << endl;

        // update carriers' bounds
        unsigned int b = 0;
        for (it = carrierMap_.begin(); it != carrierMap_.end(); ++it)
        {
            it->second.firstBand = b;
            it->second.lastBand = b + it->second.numBands - 1;
            b = it->second.lastBand + 1;
            EV << "* [" << it->first << "GHz] - range[" << it->second.firstBand << "-" << it->second.lastBand << "]" << endl;

            // --- create usable bands structure --- //

            it->second.bandLimit.clear();

            // check the number of codewords
            unsigned int numCodewords = 1;
            // for each band of the band vector provided
            for (unsigned int i = 0; i < numBands_; i++)
            {
                BandLimit elem;
                elem.band_ = Band(i);

                // check whether band i is in the set of usable bands
                int limit = (i >= it->second.firstBand && i <= it->second.lastBand) ? -1 : -2;

                elem.limit_.clear();
                for (unsigned int j = 0; j < numCodewords; j++)
                    elem.limit_.push_back(limit);

                it->second.bandLimit.push_back(elem);
            }
        }
    }
}

const std::vector<double>* CellInfo::getCarriers()
{
    return &carriersVector_;
}

const CarrierInfoMap* CellInfo::getCarrierInfoMap()
{
    return &carrierMap_;
}

unsigned int CellInfo::getCellwiseBand(double carrierFrequency, Band index)
{
    CarrierInfoMap::iterator it = carrierMap_.find(carrierFrequency);
    if (it == carrierMap_.end())
        throw omnetpp::cRuntimeError("CellInfo::getCellwiseBand - Carrier %f is not used on node %d", carrierFrequency, cellId_);

    if (index > it->second.numBands)
        throw omnetpp::cRuntimeError("CellInfo::getCellwiseBand - Selected band [%d] is greater than the number of available band on this carrier [%d]", index, it->second.numBands);

    return (it->second.firstBand + index);
}

BandLimitVector* CellInfo::getCarrierBandLimit(double carrierFrequency)
{
    if (carrierFrequency == 0.0)
    {
        CarrierInfoMap::iterator it = carrierMap_.begin();
        if (it != carrierMap_.end())
            return &(it->second.bandLimit);
    }
    CarrierInfoMap::iterator it = carrierMap_.find(carrierFrequency);
    if (it == carrierMap_.end())
        throw omnetpp::cRuntimeError("CellInfo::getCarrierBandLimit - Carrier %f is not used on node %d", carrierFrequency, cellId_);

    return &(it->second.bandLimit);
}

unsigned int CellInfo::getCarrierStartingBand(double carrierFrequency)
{
    CarrierInfoMap::iterator it = carrierMap_.find(carrierFrequency);
    if (it == carrierMap_.end())
        throw omnetpp::cRuntimeError("CellInfo::getCarrierStartingBand - Carrier [%fGHz] not found", carrierFrequency);

    return it->second.firstBand;
}

unsigned int CellInfo::getCarrierLastBand(double carrierFrequency)
{
    CarrierInfoMap::iterator it = carrierMap_.find(carrierFrequency);
    if (it == carrierMap_.end())
        throw omnetpp::cRuntimeError("CellInfo::getCarrierStartingBand - Carrier [%fGHz] not found", carrierFrequency);

    return it->second.lastBand;
}

SlotFormat CellInfo::computeSlotFormat(bool useTdd, unsigned int tddNumSymbolsDl, unsigned int tddNumSymbolsUl)
{
    SlotFormat sf;
    if (!useTdd)
    {
        sf.tdd = false;

        // these values are not used when tdd is false
        sf.numDlSymbols = 0;
        sf.numUlSymbols = 0;
        sf.numFlexSymbols = 0;
    }
    else
    {
        sf.tdd = true;
        unsigned int numSymbols = rbxDl_ * 2;
        if (tddNumSymbolsDl+tddNumSymbolsUl > numSymbols)
            throw omnetpp::cRuntimeError("CellInfo::computeSlotFormat - Number of symbols not valid - DL[%d] UL[%d]", tddNumSymbolsDl,tddNumSymbolsUl);

        sf.numDlSymbols = tddNumSymbolsDl;
        sf.numUlSymbols = tddNumSymbolsUl;
        sf.numFlexSymbols = numSymbols - tddNumSymbolsDl - tddNumSymbolsUl;
    }
    return sf;
}
