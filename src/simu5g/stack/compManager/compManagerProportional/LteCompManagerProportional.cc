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

#include "stack/compManager/compManagerProportional/LteCompManagerProportional.h"

namespace simu5g {

Define_Module(LteCompManagerProportional);
using namespace inet;

void LteCompManagerProportional::initialize()
{
    LteCompManagerBase::initialize();
}

void LteCompManagerProportional::provisionalSchedule()
{
    EV << NOW << " LteCompManagerProportional::provisionalSchedule - Start " << endl;

    // TODO check if correct
    double primaryCarrierFrequency = mac_->getCellInfo()->getCarriers()->front();

    provisionedBlocks_ = 0;

    Direction dir = DL;
    LteMacBufferMap *vbuf = mac_->getMacBuffers();
    ActiveSet *activeSet = mac_->getActiveSet(dir);
    for (MacCid cid : *activeSet) {
        MacNodeId ueId = MacCidToNodeId(cid);

        unsigned int queueLength = vbuf->at(cid)->getQueueOccupancy();

        // Compute the number of bytes available in one block for this UE
        unsigned int bytesPerBlock = 0;
        unsigned int blocks = 1;
        Codeword cw = 0;
        Band band = 0;
        bytesPerBlock = mac_->getAmc()->computeBytesOnNRbs(ueId, band, cw,
                blocks, dir, primaryCarrierFrequency); // The index of the band is useless
        EV << NOW << " LteCompManagerProportional::provisionalSchedule - For the node: " << ueId << " available: " << bytesPerBlock << " bytes in one block" << endl;

        // Compute the number of blocks required to satisfy the UE's buffer
        unsigned int reqBlocks;
        if (bytesPerBlock == 0)
            reqBlocks = 0;
        else
            reqBlocks = (queueLength + bytesPerBlock - 1) / bytesPerBlock;
        EV << NOW << " LteCompManagerProportional::provisionalSchedule - For the node: " << ueId << " required: " << reqBlocks << " blocks" << endl;

        provisionedBlocks_ += reqBlocks;
    }

    EV << NOW << " LteCompManagerProportional::provisionalSchedule - End " << endl;
}

void LteCompManagerProportional::doCoordination()
{
    EV << NOW << " LteCompManagerProportional::doCoordination - Start " << endl;

    partitioning_.clear();
    offset_.clear();

    unsigned int requestsSum = 0;
    for (const auto& [nodeId, reqCount] : reqBlocksMap_)
        requestsSum += reqCount;

    // assign a number of blocks that is proportional to the requests received from each eNB
    unsigned int totalReservedBlocks = 0;
    std::vector<double> reservation;
    for (const auto& [nodeId, req] : reqBlocksMap_) {

        // requests from the current node

        // compute the number of blocks to reserve
        double percentage;
        if (requestsSum == 0)
            percentage = 1.0 / (clientList_.size() + 1);                                                            // slaves + master
        else
            percentage = (double)req / requestsSum;

        double toReserve = numBands_ * percentage;
        totalReservedBlocks += toReserve;

        reservation.push_back(toReserve);
    }

    // round vector to integer
    partitioning_ = roundVector(reservation, totalReservedBlocks);

    offset_.push_back(0);
    for (unsigned int i = 0; i < partitioning_.size() - 1; i++) {
        offset_.push_back(partitioning_[i]);
    }

    EV << NOW << " LteCompManagerProportional::doCoordination - End " << endl;
}

X2CompProportionalRequestIE *LteCompManagerProportional::buildClientRequest()
{
    // build IE
    X2CompProportionalRequestIE *requestIe = new X2CompProportionalRequestIE();
    requestIe->setNumBlocks(provisionedBlocks_);

    return requestIe;
}

X2CompProportionalReplyIE *LteCompManagerProportional::buildCoordinatorReply(X2NodeId clientId)
{
    // find the correct entry in the partitioning vector
    bool found = false;
    unsigned int index = 0;
    for (const auto& [key, value] : reqBlocksMap_) {
        if (key == clientId) {
            found = true;
            break;
        }
        index++;
    }

    unsigned int numBlocks = 0;
    unsigned int band = 0;
    if (found) {
        numBlocks = partitioning_[index];
        band = offset_[index];
    }
    else {
        EV << "LteCompManagerProportional::buildCoordinatorReply: no information for " << clientId << " available (number of requested blocks unknown)" << std::endl;
    }

    // build map for each node
    std::vector<CompRbStatus> allowedBlocks;

    // set "numBlocks" contiguous blocks for this node
    allowedBlocks.resize(numBands_, NOT_AVAILABLE_RB);
    unsigned int lb = band;
    unsigned int ub = band + numBlocks;
    for (unsigned int b = lb; b < ub; b++) {
        allowedBlocks[b] = AVAILABLE_RB;
        band++;
    }

    // build IE
    X2CompProportionalReplyIE *replyIe = new X2CompProportionalReplyIE();
    replyIe->setAllowedBlocksMap(allowedBlocks);

    return replyIe;
}

void LteCompManagerProportional::handleClientRequest(inet::Ptr<X2CompMsg> compMsg)
{
    X2NodeId sourceId = compMsg->getSourceId();
    while (compMsg->hasIe()) {
        X2InformationElement *ie = compMsg->popIe();
        if (ie->getType() != COMP_PROP_REQUEST_IE)
            throw cRuntimeError("LteCompManagerProportional::handleClientRequest - Expected COMP_PROP_REQUEST_IE");

        X2CompProportionalRequestIE *requestIe = check_and_cast<X2CompProportionalRequestIE *>(ie);
        unsigned int reqBlocks = requestIe->getNumBlocks();

        EV << "LteCompManagerProportional::handleClientRequest: " << this->getFullPath() << " received request from " << sourceId << " for " << reqBlocks << " blocks." << std::endl;

        // update map entry for this node
        if (reqBlocksMap_.find(sourceId) == reqBlocksMap_.end())
            reqBlocksMap_.insert({sourceId, reqBlocks});
        else
            reqBlocksMap_[sourceId] = reqBlocks;

        delete requestIe;
    }
}

void LteCompManagerProportional::handleCoordinatorReply(inet::Ptr<X2CompMsg> compMsg)
{
    while (compMsg->hasIe()) {
        X2InformationElement *ie = compMsg->popIe();
        if (ie->getType() != COMP_PROP_REPLY_IE)
            throw cRuntimeError(
                    "LteCompManagerProportional::handleCoordinatorReply - Expected COMP_PROP_REPLY_IE");

        // parse reply message
        X2CompProportionalReplyIE *replyIe = check_and_cast<X2CompProportionalReplyIE *>(ie);
        std::vector<CompRbStatus> allowedBlocksMap = replyIe->getAllowedBlocksMap();
        UsableBands usableBands = parseAllowedBlocksMap(allowedBlocksMap);

        EV << "At " << this->getFullPath() << " LteCompManagerProportional::handleCoordinatorReply: " << usableBands.size() << " bands usable." << std::endl;

        setUsableBands(usableBands);

        delete replyIe;
    }
}

UsableBands LteCompManagerProportional::parseAllowedBlocksMap(std::vector<CompRbStatus>& allowedBlocksMap)
{
    unsigned int reservedBlocks = 0;
    UsableBands usableBands;
    for (unsigned int b = 0; b < allowedBlocksMap.size(); b++) {
        if (allowedBlocksMap[b] == AVAILABLE_RB) {
            // eNB is allowed to use this block
            usableBands.push_back(b);
            reservedBlocks++;
        }
    }

    emit(compReservedBlocksSignal_, reservedBlocks);

    return usableBands;
}

std::vector<unsigned int> LteCompManagerProportional::roundVector(std::vector<double>& vec, int sum)
{
    // the rounding algorithm needs that the vector is sorted in ascending order

    // we sort the vector, then put the elements in the original order
    // to do this, we use an auxiliary vector to remember the original positions

    unsigned int len = vec.size();
    std::vector<unsigned int> integerVec;
    integerVec.resize(len, 0);

    if (len > 0) {
        // auxiliary vector
        std::vector<unsigned int> auxVec;
        auxVec.resize(len, 0);
        for (unsigned int i = 0; i < auxVec.size(); i++)
            auxVec[i] = i;

        // sort vector in ascending order
        bool swap = true;
        while (swap) {
            swap = false;
            for (unsigned int i = 0; i < len - 1; i++) {
                if (vec[i] > vec[i + 1]) {
                    double tmp = vec[i + 1];
                    vec[i + 1] = vec[i];
                    vec[i] = tmp;

                    unsigned int auxTmp = auxVec[i + 1];
                    auxVec[i + 1] = auxVec[i];
                    auxVec[i] = auxTmp;

                    swap = true;
                }
            }
        }

        // round vector (the sum of the elements is preserved)
        int integerTot = 0;
        double doubleTot = 0;
        for (unsigned int i = 0; i < len; i++) {
            doubleTot += vec[i];
            vec[i] = (int)(doubleTot - integerTot);
            integerTot += vec[i];
        }

        // TODO -  check numerical errors

        // set the integer vector with the elements in their original positions
        for (unsigned int i = 0; i < integerVec.size(); i++) {
            unsigned int index = auxVec[i];
            integerVec[index] = vec[i];
        }
    }
    return integerVec;
}

} //namespace

