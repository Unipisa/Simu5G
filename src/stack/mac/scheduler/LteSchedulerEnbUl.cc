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

#include "stack/mac/scheduler/LteSchedulerEnbUl.h"
#include "stack/mac/layer/LteMacEnb.h"
#include "stack/mac/layer/LteMacEnbD2D.h"
#include "stack/mac/buffer/harq/LteHarqBufferRx.h"
#include "stack/mac/allocator/LteAllocationModule.h"

using namespace omnetpp;

bool
LteSchedulerEnbUl::checkEligibility(MacNodeId id, Codeword& cw, double carrierFrequency)
{
    HarqRxBuffers* harqRxBuff = mac_->getHarqRxBuffers(carrierFrequency);
    if (harqRxBuff == NULL)  // a new HARQ buffer will be created at reception
        return true;

    // check if harq buffer have already been created for this node
    if (harqRxBuff->find(id) != harqRxBuff->end())
    {
        LteHarqBufferRx* ulHarq = harqRxBuff->at(id);

        // get current Harq Process for nodeId
        unsigned char currentAcid = harqStatus_[carrierFrequency].at(id);
        // get current Harq Process status
        std::vector<RxUnitStatus> status = ulHarq->getProcess(currentAcid)->getProcessStatus();
        // check if at least one codeword buffer is available for reception
        for (; cw < MAX_CODEWORDS; ++cw)
        {
            if (status.at(cw).second == RXHARQ_PDU_EMPTY)
            {
                return true;
            }
        }
    }
    return false;
}

void
LteSchedulerEnbUl::updateHarqDescs()
{
    EV << NOW << "LteSchedulerEnbUl::updateHarqDescs  cell " << mac_->getMacCellId() << endl;

    std::map<double, HarqRxBuffers>::iterator cit;
    HarqRxBuffers::iterator it;
    HarqStatus::iterator currentStatus;

    for (cit=harqRxBuffers_->begin();cit!=harqRxBuffers_->end();++cit)
    {
        for (it=cit->second.begin();it!=cit->second.end();++it)
        {
             if ((currentStatus=harqStatus_[cit->first].find(it->first)) != harqStatus_[cit->first].end())
             {
                 EV << NOW << "LteSchedulerEnbUl::updateHarqDescs UE " << it->first << " OLD Current Process is  " << (unsigned int)currentStatus->second << endl;
                 // updating current acid id
                 currentStatus->second = (currentStatus->second +1 ) % (it->second->getProcesses());

                 EV << NOW << "LteSchedulerEnbUl::updateHarqDescs UE " << it->first << "NEW Current Process is " << (unsigned int)currentStatus->second << "(total harq processes " << it->second->getProcesses() << ")" << endl;
             }
             else
             {
                 EV << NOW << "LteSchedulerEnbUl::updateHarqDescs UE " << it->first << " initialized the H-ARQ status " << endl;
                 harqStatus_[cit->first][it->first]=0;
             }
        }
    }
}

bool LteSchedulerEnbUl::racschedule(double carrierFrequency)
{
    EV << NOW << " LteSchedulerEnbUl::racschedule --------------------::[ START RAC-SCHEDULE ]::--------------------" << endl;
    EV << NOW << " LteSchedulerEnbUl::racschedule eNodeB: " << mac_->getMacCellId() << endl;
    EV << NOW << " LteSchedulerEnbUl::racschedule Direction: " << (direction_ == UL ? "UL" : "DL") << endl;

    RacStatus::iterator it=racStatus_.begin() , et=racStatus_.end();

    for (;it!=et;++it)
    {
        // get current nodeId
        MacNodeId nodeId = it->first;
        EV << NOW << " LteSchedulerEnbUl::racschedule handling RAC for node " << nodeId << endl;

        // Get number of logical bands
        unsigned int numBands = mac_->getCellInfo()->getNumBands();

        // FIXME default behavior
        //try to allocate one block to selected UE on at least one logical band of MACRO antenna, first codeword

        const unsigned int cw =0;
        const unsigned int blocks =1;

        bool allocation=false;

        for (Band b=0;b<numBands;++b)
        {
            if ( allocator_->availableBlocks(nodeId,MACRO,b) >0)
            {
                unsigned int bytes = mac_->getAmc()->computeBytesOnNRbs(nodeId,b,cw,blocks,UL,carrierFrequency);
                if (bytes > 0)
                {
                    allocator_->addBlocks(MACRO,b,nodeId,1,bytes);

                    EV << NOW << "LteSchedulerEnbUl::racschedule UE: " << nodeId << "Handled RAC on band: " << b << endl;

                    allocation=true;
                    break;
                }
            }
        }

        if (allocation)
        {
            // create scList id for current cid/codeword
            MacCid cid = idToMacCid(nodeId, SHORT_BSR);  // build the cid. Since this grant will be used for a BSR,
                                                         // we use the LCID corresponding to the SHORT_BSR
            std::pair<unsigned int,Codeword> scListId = std::pair<unsigned int,Codeword>(cid,cw);
            scheduleList_[carrierFrequency][scListId]=blocks;
        }
    }

    // clean up all requests
    racStatus_.clear();

    EV << NOW << " LteSchedulerEnbUl::racschedule --------------------::[  END RAC-SCHEDULE  ]::--------------------" << endl;

    int availableBlocks = allocator_->computeTotalRbs();

    return (availableBlocks==0);
}

bool
LteSchedulerEnbUl::rtxschedule(double carrierFrequency, BandLimitVector* bandLim)
{
    // try to handle RAC requests first and abort rtx scheduling if no OFDMA space is left after
    if (racschedule(carrierFrequency))
        return true;

    try
    {
        EV << NOW << " LteSchedulerEnbUl::rtxschedule --------------------::[ START RTX-SCHEDULE ]::--------------------" << endl;
        EV << NOW << " LteSchedulerEnbUl::rtxschedule eNodeB: " << mac_->getMacCellId() << endl;
        EV << NOW << " LteSchedulerEnbUl::rtxschedule Direction: " << (direction_ == UL ? "UL" : "DL") << endl;

        if (harqRxBuffers_->find(carrierFrequency) != harqRxBuffers_->end())
        {
            HarqRxBuffers::iterator it= harqRxBuffers_->at(carrierFrequency).begin() , et=harqRxBuffers_->at(carrierFrequency).end();
            for(; it != et; ++it)
            {
                // get current nodeId
                MacNodeId nodeId = it->first;

                if(nodeId == 0){
                    // UE has left the simulation - erase queue and continue
                    harqRxBuffers_->at(carrierFrequency).erase(nodeId);
                    continue;
                }
                OmnetId id = binder_->getOmnetId(nodeId);
                if(id == 0){
                    harqRxBuffers_->at(carrierFrequency).erase(nodeId);
                    continue;
                }

                // get current Harq Process for nodeId
                unsigned char currentAcid = harqStatus_[carrierFrequency].at(nodeId);

                // check whether the UE has a H-ARQ process waiting for retransmission. If not, skip UE.
                bool skip = true;
                unsigned char acid = (currentAcid + 2) % (it->second->getProcesses());
                LteHarqProcessRx* currentProcess = it->second->getProcess(acid);
                std::vector<RxUnitStatus> procStatus = currentProcess->getProcessStatus();
                std::vector<RxUnitStatus>::iterator pit = procStatus.begin();
                for (; pit != procStatus.end(); ++pit )
                {
                    if (pit->second == RXHARQ_PDU_CORRUPTED)
                    {
                        skip = false;
                        break;
                    }
                }
                if (skip)
                    continue;

                EV << NOW << "LteSchedulerEnbUl::rtxschedule UE: " << nodeId << "Acid: " << (unsigned int)currentAcid << endl;

                // Get user transmission parameters
                const UserTxParams& txParams = mac_->getAmc()->computeTxParams(nodeId, direction_,carrierFrequency);// get the user info

                unsigned int codewords = txParams.getLayers().size();// get the number of available codewords
                unsigned int allocatedBytes =0;

                // TODO handle the codewords join case (sizeof(cw0+cw1) < currentTbs && currentLayers ==1)

                for(Codeword cw = 0; (cw < MAX_CODEWORDS) && (codewords>0); ++cw)
                {
                    unsigned int rtxBytes=0;
                    // FIXME PERFORMANCE: check for rtx status before calling rtxAcid

                    // perform a retransmission on available codewords for the selected acid
                    rtxBytes=LteSchedulerEnbUl::schedulePerAcidRtx(nodeId, carrierFrequency, cw,acid,bandLim);
                    if (rtxBytes>0)
                    {
                        --codewords;
                        allocatedBytes+=rtxBytes;
                    }
                }
                EV << NOW << "LteSchedulerEnbUl::rtxschedule user " << nodeId << " allocated bytes : " << allocatedBytes << endl;
            }
        }
        if (mac_->isD2DCapable())
        {
            // --- START Schedule D2D retransmissions --- //
            Direction dir = D2D;
            HarqBuffersMirrorD2D* harqBuffersMirrorD2D = check_and_cast<LteMacEnbD2D*>(mac_)->getHarqBuffersMirrorD2D(carrierFrequency);
            if (harqBuffersMirrorD2D != NULL)
            {
                HarqBuffersMirrorD2D::iterator it_d2d = harqBuffersMirrorD2D->begin() , et_d2d=harqBuffersMirrorD2D->end();
                while (it_d2d != et_d2d)
                {

                    // get current nodeIDs
                    MacNodeId senderId = (it_d2d->first).first; // Transmitter
                    MacNodeId destId = (it_d2d->first).second;  // Receiver

                    if(senderId == 0 || binder_->getOmnetId(senderId) == 0) {
                        // UE has left the simulation - erase queue and continue
                        harqBuffersMirrorD2D->erase(it_d2d++);
                        continue;
                    }
                    if(destId == 0 || binder_->getOmnetId(destId) == 0) {
                        // UE has left the simulation - erase queue and continue
                        harqBuffersMirrorD2D->erase(it_d2d++);
                        continue;
                    }

                    // get current Harq Process for nodeId
                    unsigned char currentAcid = harqStatus_[carrierFrequency].at(senderId);

                    // check whether the UE has a H-ARQ process waiting for retransmission. If not, skip UE.
                    bool skip = true;
                    unsigned char acid = (currentAcid + 2) % (it_d2d->second->getProcesses());
                    LteHarqProcessMirrorD2D* currentProcess = it_d2d->second->getProcess(acid);
                    std::vector<TxHarqPduStatus> procStatus = currentProcess->getProcessStatus();
                    std::vector<TxHarqPduStatus>::iterator pit = procStatus.begin();
                    for (; pit != procStatus.end(); ++pit )
                    {
                        if (*pit == TXHARQ_PDU_BUFFERED)
                        {
                            skip = false;
                            break;
                        }
                    }
                    if (skip)
                    {
                        ++it_d2d;
                        continue;
                    }

                    EV << NOW << " LteSchedulerEnbUl::rtxschedule - D2D UE: " << senderId << " Acid: " << (unsigned int)currentAcid << endl;

                    // Get user transmission parameters
                    const UserTxParams& txParams = mac_->getAmc()->computeTxParams(senderId, dir,carrierFrequency);// get the user info

                    unsigned int codewords = txParams.getLayers().size();// get the number of available codewords
                    unsigned int allocatedBytes =0;

                    // TODO handle the codewords join case (size of(cw0+cw1) < currentTbs && currentLayers ==1)

                    for(Codeword cw = 0; (cw < MAX_CODEWORDS) && (codewords>0); ++cw)
                    {
                        unsigned int rtxBytes=0;
                        // FIXME PERFORMANCE: check for rtx status before calling rtxAcid

                        // perform a retransmission on available codewords for the selected acid
                        rtxBytes = LteSchedulerEnbUl::schedulePerAcidRtxD2D(destId, senderId,  carrierFrequency, cw, acid, bandLim);
                        if (rtxBytes>0)
                        {
                            --codewords;
                            allocatedBytes+=rtxBytes;
                        }
                    }
                    EV << NOW << " LteSchedulerEnbUl::rtxschedule - D2D UE: " << senderId << " allocated bytes : " << allocatedBytes << endl;
                    ++it_d2d;
                }
            }
            // --- END Schedule D2D retransmissions --- //
        }

        int availableBlocks = allocator_->computeTotalRbs();

        EV << NOW << " LteSchedulerEnbUl::rtxschedule residual OFDM Space: " << availableBlocks << endl;

        EV << NOW << " LteSchedulerEnbUl::rtxschedule --------------------::[  END RTX-SCHEDULE  ]::--------------------" << endl;

        return (availableBlocks == 0);
    }
    catch(std::exception& e)
    {
        throw cRuntimeError("Exception in LteSchedulerEnbUl::rtxschedule(): %s", e.what());
    }
    return 0;
}

unsigned int
LteSchedulerEnbUl::schedulePerAcidRtx(MacNodeId nodeId, double carrierFrequency, Codeword cw, unsigned char acid,
    std::vector<BandLimit>* bandLim, Remote antenna, bool limitBl)
{
    try
    {
        const UserTxParams& txParams = mac_->getAmc()->computeTxParams(nodeId, direction_,carrierFrequency);    // get the user info
        const std::set<Band>& allowedBands = txParams.readBands();
        BandLimitVector tempBandLim;
        tempBandLim.clear();
        std::string bands_msg = "BAND_LIMIT_SPECIFIED";
        if (bandLim == nullptr)
        {
            // Create a vector of band limit using all bands
            // FIXME: bandlim is never deleted

            unsigned int numBands = mac_->getCellInfo()->getNumBands();
            // for each band of the band vector provided
            for (unsigned int i = 0; i < numBands; i++)
            {
                BandLimit elem;
                // copy the band
                elem.band_ = Band(i);
                EV << "Putting band " << i << endl;
                for (unsigned int j = 0; j < MAX_CODEWORDS; j++)
                {
                    if( allowedBands.find(elem.band_)!= allowedBands.end() )
                    {
                        EV << "\t" << i << " " << "yes" << endl;
                        elem.limit_[j]=-1;
                    }
                    else
                    {
                        EV << "\t" << i << " " << "no" << endl;
                        elem.limit_[j]=-2;
                    }
                }
                tempBandLim.push_back(elem);
            }
            bandLim = &tempBandLim;
        }
        else
        {
            unsigned int numBands = mac_->getCellInfo()->getNumBands();
            // for each band of the band vector provided
            for (unsigned int i = 0; i < numBands; i++)
            {
                BandLimit& elem = bandLim->at(i);
                for (unsigned int j = 0; j < MAX_CODEWORDS; j++)
                {
                    if (elem.limit_[j] == -2)
                        continue;

                    if (allowedBands.find(elem.band_)!= allowedBands.end() )
                    {
                        EV << "\t" << i << " " << "yes" << endl;
                        elem.limit_[j]=-1;
                    }
                    else
                    {
                        EV << "\t" << i << " " << "no" << endl;
                        elem.limit_[j]=-2;
                    }
                }
            }
        }

        EV << NOW << "LteSchedulerEnbUl::rtxAcid - Node[" << mac_->getMacNodeId() << ", User[" << nodeId << ", Codeword[ " << cw << "], ACID[" << (unsigned int)acid << "] " << endl;

        LteHarqProcessRx* currentProcess = harqRxBuffers_->at(carrierFrequency).at(nodeId)->getProcess(acid);

        if (currentProcess->getUnitStatus(cw) != RXHARQ_PDU_CORRUPTED)
        {
            // exit if the current active HARQ process is not ready for retransmission
            EV << NOW << " LteSchedulerEnbUl::rtxAcid User is on ACID " << (unsigned int)acid << " HARQ process is IDLE. No RTX scheduled ." << endl;
            return 0;
        }

        Codeword allocatedCw = 0;
        // search for already allocated codeword
        // create "mirror" scList ID for other codeword than current
        std::pair<unsigned int, Codeword> scListMirrorId = std::pair<unsigned int, Codeword>(idToMacCid(nodeId,SHORT_BSR), MAX_CODEWORDS - cw - 1);
        if (scheduleList_.find(carrierFrequency) != scheduleList_.end())
        {
            if (scheduleList_[carrierFrequency].find(scListMirrorId) != scheduleList_[carrierFrequency].end())
            {
                allocatedCw = MAX_CODEWORDS - cw - 1;
            }
        }
        // get current process buffered PDU byte length
        unsigned int bytes = currentProcess->getByteLength(cw);
        // bytes to serve
        unsigned int toServe = bytes;
        // blocks to allocate for each band
        std::vector<unsigned int> assignedBlocks;
        // bytes which blocks from the preceding vector are supposed to satisfy
        std::vector<unsigned int> assignedBytes;

        // end loop signal [same as bytes>0, but more secure]
        bool finish = false;
        // for each band
        unsigned int size = bandLim->size();
        for (unsigned int i = 0; (i < size) && (!finish); ++i)
        {
            // save the band and the relative limit
            Band b = bandLim->at(i).band_;
            int limit = bandLim->at(i).limit_.at(cw);

            // TODO add support to multi CW
//            unsigned int bandAvailableBytes = // if a codeword has been already scheduled for retransmission, limit available blocks to what's been  allocated on that codeword
//                    ((allocatedCw == MAX_CODEWORDS) ? availableBytes(nodeId,antenna, b, cw) : mac_->getAmc()->blocks2bytes(nodeId, b, cw, allocator_->getBlocks(antenna,b,nodeId) , direction_));    // available space
            unsigned int bandAvailableBytes = availableBytes(nodeId, antenna, b, cw, direction_, carrierFrequency);

            // use the provided limit as cap for available bytes, if it is not set to unlimited
            if (limit >= 0)
                bandAvailableBytes = limit < (int) bandAvailableBytes ? limit : bandAvailableBytes;

            EV << NOW << " LteSchedulerEnbUl::rtxAcid BAND " << b << endl;
            EV << NOW << " LteSchedulerEnbUl::rtxAcid total bytes:" << bytes << " still to serve: " << toServe << " bytes" << endl;
            EV << NOW << " LteSchedulerEnbUl::rtxAcid Available: " << bandAvailableBytes << " bytes" << endl;

            unsigned int servedBytes = 0;
            // there's no room on current band for serving the entire request
            if (bandAvailableBytes < toServe)
            {
                // record the amount of served bytes
                servedBytes = bandAvailableBytes;
                // the request can be fully satisfied
            }
            else
            {
                // record the amount of served bytes
                servedBytes = toServe;
                // signal end loop - all data have been serviced
                finish = true;
            }
            unsigned int servedBlocks = mac_->getAmc()->computeReqRbs(nodeId, b, cw, servedBytes, direction_,carrierFrequency);
            // update the bytes counter
            toServe -= servedBytes;
            // update the structures
            assignedBlocks.push_back(servedBlocks);
            assignedBytes.push_back(servedBytes);
        }

        if (toServe > 0)
        {
            // process couldn't be served - no sufficient space on available bands
            EV << NOW << " LteSchedulerEnbUl::rtxAcid Unavailable space for serving node " << nodeId << " ,HARQ Process " << (unsigned int)acid << " on codeword " << cw << endl;
            return 0;
        }
        else
        {
            // record the allocation
            unsigned int size = assignedBlocks.size();
            unsigned int cwAllocatedBlocks =0;

            // create scList id for current node/codeword
            std::pair<unsigned int,Codeword> scListId = std::pair<unsigned int,Codeword>(idToMacCid(nodeId, SHORT_BSR), cw);

            for(unsigned int i = 0; i < size; ++i)
            {
                // For each LB for which blocks have been allocated
                Band b = bandLim->at(i).band_;

                cwAllocatedBlocks +=assignedBlocks.at(i);
                EV << "\t Cw->" << allocatedCw << "/" << MAX_CODEWORDS << endl;
                //! handle multi-codeword allocation
                if (allocatedCw!=MAX_CODEWORDS)
                {
                    EV << NOW << " LteSchedulerEnbUl::rtxAcid - adding " << assignedBlocks.at(i) << " to band " << i << endl;
                    allocator_->addBlocks(antenna,b,nodeId,assignedBlocks.at(i),assignedBytes.at(i));
                }
                //! TODO check if ok bandLim->at.limit_.at(cw) = assignedBytes.at(i);
            }

            // signal a retransmission
            // schedule list contains number of granted blocks

            scheduleList_[carrierFrequency][scListId] = cwAllocatedBlocks;
            // mark codeword as used
            if (allocatedCws_.find(nodeId)!=allocatedCws_.end())
            {
                allocatedCws_.at(nodeId)++;
            }
            else
            {
                allocatedCws_[nodeId]=1;
            }

            EV << NOW << " LteSchedulerEnbUl::rtxAcid HARQ Process " << (unsigned int)acid << " : " << bytes << " bytes served! " << endl;

            return bytes;
        }
    }
    catch(std::exception& e)
    {
        throw cRuntimeError("Exception in LteSchedulerEnbUl::rtxAcid(): %s", e.what());
    }
    return 0;
}

unsigned int
LteSchedulerEnbUl::schedulePerAcidRtxD2D(MacNodeId destId,MacNodeId senderId, double carrierFrequency, Codeword cw, unsigned char acid,
    std::vector<BandLimit>* bandLim, Remote antenna, bool limitBl)
{
    Direction dir = D2D;
    try
    {
        const UserTxParams& txParams = mac_->getAmc()->computeTxParams(senderId, dir,carrierFrequency);    // get the user info
        const std::set<Band>& allowedBands = txParams.readBands();
        BandLimitVector tempBandLim;
        tempBandLim.clear();
        std::string bands_msg = "BAND_LIMIT_SPECIFIED";
        if (bandLim == nullptr)
        {
            // Create a vector of band limit using all bands
            // FIXME: bandlim is never deleted

            unsigned int numBands = mac_->getCellInfo()->getNumBands();
            // for each band of the band vector provided
            for (unsigned int i = 0; i < numBands; i++)
            {
                BandLimit elem;
                // copy the band
                elem.band_ = Band(i);
                EV << "Putting band " << i << endl;
                for (unsigned int j = 0; j < MAX_CODEWORDS; j++)
                {
                    if( allowedBands.find(elem.band_)!= allowedBands.end() )
                    {
                        EV << "\t" << i << " " << "yes" << endl;
                        elem.limit_[j]=-1;
                    }
                    else
                    {
                        EV << "\t" << i << " " << "no" << endl;
                        elem.limit_[j]=-2;
                    }
                }
                tempBandLim.push_back(elem);
            }
            bandLim = &tempBandLim;
        }
        else
        {
            unsigned int numBands = mac_->getCellInfo()->getNumBands();
            // for each band of the band vector provided
            for (unsigned int i = 0; i < numBands; i++)
            {
                BandLimit& elem = bandLim->at(i);
                for (unsigned int j = 0; j < MAX_CODEWORDS; j++)
                {
                    if (elem.limit_[j] == -2)
                        continue;

                    if (allowedBands.find(elem.band_)!= allowedBands.end() )
                    {
                        EV << "\t" << i << " " << "yes" << endl;
                        elem.limit_[j]=-1;
                    }
                    else
                    {
                        EV << "\t" << i << " " << "no" << endl;
                        elem.limit_[j]=-2;
                    }
                }
            }
        }

        EV << NOW << "LteSchedulerEnbUl::schedulePerAcidRtxD2D - Node[" << mac_->getMacNodeId() << ", User[" << senderId << ", Codeword[ " << cw << "], ACID[" << (unsigned int)acid << "] " << endl;

        D2DPair pair(senderId, destId);

        // Get the current active HARQ process
        HarqBuffersMirrorD2D* harqBuffersMirrorD2D = check_and_cast<LteMacEnbD2D*>(mac_)->getHarqBuffersMirrorD2D(carrierFrequency);
        EV << "\t the acid that should be considered is " << (unsigned int)acid << endl;

        LteHarqProcessMirrorD2D* currentProcess = harqBuffersMirrorD2D->at(pair)->getProcess(acid);
        if (currentProcess->getUnitStatus(cw) != TXHARQ_PDU_BUFFERED)
        {
            // exit if the current active HARQ process is not ready for retransmission
            EV << NOW << " LteSchedulerEnbUl::schedulePerAcidRtxD2D User is on ACID " << (unsigned int)acid << " HARQ process is IDLE. No RTX scheduled ." << endl;
            return 0;
        }

        Codeword allocatedCw = 0;
        //search for already allocated codeword
        //create "mirror" scList ID for other codeword than current
        std::pair<unsigned int, Codeword> scListMirrorId = std::pair<unsigned int, Codeword>(idToMacCid(senderId, D2D_SHORT_BSR), MAX_CODEWORDS - cw - 1);
        if (scheduleList_.find(carrierFrequency) != scheduleList_.end())
        {
            if (scheduleList_[carrierFrequency].find(scListMirrorId) != scheduleList_[carrierFrequency].end())
            {
                allocatedCw = MAX_CODEWORDS - cw - 1;
            }
        }
        // get current process buffered PDU byte length
        unsigned int bytes = currentProcess->getPduLength(cw);
        // bytes to serve
        unsigned int toServe = bytes;
        // blocks to allocate for each band
        std::vector<unsigned int> assignedBlocks;
        // bytes which blocks from the preceding vector are supposed to satisfy
        std::vector<unsigned int> assignedBytes;

        // end loop signal [same as bytes>0, but more secure]
        bool finish = false;
        // for each band
        unsigned int size = bandLim->size();
        for (unsigned int i = 0; (i < size) && (!finish); ++i)
        {
            // save the band and the relative limit
            Band b = bandLim->at(i).band_;
            int limit = bandLim->at(i).limit_.at(cw);

            // TODO add support to multi CW
            //unsigned int bandAvailableBytes = // if a codeword has been already scheduled for retransmission, limit available blocks to what's been  allocated on that codeword
            //((allocatedCw == MAX_CODEWORDS) ? availableBytes(nodeId,antenna, b, cw) : mac_->getAmc()->blocks2bytes(nodeId, b, cw, allocator_->getBlocks(antenna,b,nodeId) , direction_));    // available space
            unsigned int bandAvailableBytes = availableBytes(senderId, antenna, b, cw, dir, carrierFrequency);

            // use the provided limit as cap for available bytes, if it is not set to unlimited
            if (limit >= 0)
                bandAvailableBytes = limit < (int) bandAvailableBytes ? limit : bandAvailableBytes;

            EV << NOW << " LteSchedulerEnbUl::schedulePerAcidRtxD2D BAND " << b << endl;
            EV << NOW << " LteSchedulerEnbUl::schedulePerAcidRtxD2D total bytes:" << bytes << " still to serve: " << toServe << " bytes" << endl;
            EV << NOW << " LteSchedulerEnbUl::schedulePerAcidRtxD2D Available: " << bandAvailableBytes << " bytes" << endl;

            unsigned int servedBytes = 0;
            // there's no room on current band for serving the entire request
            if (bandAvailableBytes < toServe)
            {
                // record the amount of served bytes
                servedBytes = bandAvailableBytes;
                // the request can be fully satisfied
            }
            else
            {
                // record the amount of served bytes
                servedBytes = toServe;
                // signal end loop - all data have been serviced
                finish = true;
                EV << NOW << " LteSchedulerEnbUl::schedulePerAcidRtxD2D ALL DATA HAVE BEEN SERVICED"<< endl;
            }
            unsigned int servedBlocks = mac_->getAmc()->computeReqRbs(senderId, b, cw, servedBytes, dir,carrierFrequency);
            // update the bytes counter
            toServe -= servedBytes;
            // update the structures
            assignedBlocks.push_back(servedBlocks);
            assignedBytes.push_back(servedBytes);
        }

        if (toServe > 0)
        {
            // process couldn't be served - no sufficient space on available bands
            EV << NOW << " LteSchedulerEnbUl::schedulePerAcidRtxD2D Unavailable space for serving node " << senderId << " ,HARQ Process " << (unsigned int)acid << " on codeword " << cw << endl;
            return 0;
        }
        else
        {
            // record the allocation
            unsigned int size = assignedBlocks.size();
            unsigned int cwAllocatedBlocks = 0;

            // create scList id for current cid/codeword
            std::pair<unsigned int,Codeword> scListId = std::pair<unsigned int,Codeword>(idToMacCid(senderId, D2D_SHORT_BSR), cw);

            for(unsigned int i = 0; i < size; ++i)
            {
                // For each LB for which blocks have been allocated
                Band b = bandLim->at(i).band_;

                cwAllocatedBlocks += assignedBlocks.at(i);
                EV << "\t Cw->" << allocatedCw << "/" << MAX_CODEWORDS << endl;
                //! handle multi-codeword allocation
                if (allocatedCw!=MAX_CODEWORDS)
                {
                    EV << NOW << " LteSchedulerEnbUl::schedulePerAcidRtxD2D - adding " << assignedBlocks.at(i) << " to band " << i << endl;
                    allocator_->addBlocks(antenna,b,senderId,assignedBlocks.at(i),assignedBytes.at(i));
                }
                //! TODO check if ok bandLim->at.limit_.at(cw) = assignedBytes.at(i);
            }

            // signal a retransmission
            // schedule list contains number of granted blocks
            scheduleList_[carrierFrequency][scListId] = cwAllocatedBlocks;
            // mark codeword as used
            if (allocatedCws_.find(senderId)!=allocatedCws_.end())
            {
                allocatedCws_.at(senderId)++;
            }
            else
            {
                allocatedCws_[senderId]=1;
            }

            EV << NOW << " LteSchedulerEnbUl::schedulePerAcidRtxD2D HARQ Process " << (unsigned int)acid << " : " << bytes << " bytes served! " << endl;

            currentProcess->markSelected(cw);

            return bytes;
        }

    }
    catch(std::exception& e)
    {
        throw cRuntimeError("Exception in LteSchedulerEnbUl::schedulePerAcidRtxD2D(): %s", e.what());
    }
    return 0;
}

void LteSchedulerEnbUl::removePendingRac(MacNodeId nodeId)
{
    racStatus_.erase(nodeId);
}
