/*
 * DQos.cc
 *
 *  Created on: Apr 23, 2023
 *      Author: devika
 */


#include "stack/mac/scheduling_modules/DQos.h"
#include "stack/mac/scheduler/LteSchedulerEnb.h"
#include "stack/backgroundTrafficGenerator/BackgroundTrafficManager.h"

using namespace omnetpp;

DQos::DQos(Direction dir) {

    LteScheduler();
    this->dir = dir;
}
void DQos::notifyActiveConnection(MacCid cid) {
    //std::cout << "NRQoSModel::notifyActiveConnection start at " << simTime().dbl() << std::endl;

    activeConnectionSet_->insert(cid);

    //std::cout << "NRQoSModel::notifyActiveConnection end at " << simTime().dbl() << std::endl;
}

void DQos::removeActiveConnection(MacCid cid) {
    //std::cout << "NRQoSModel::removeActiveConnection start at " << simTime().dbl() << std::endl;

    activeConnectionSet_->erase(cid);

    //std::cout << "NRQoSModel::removeActiveConnection end at " << simTime().dbl() << std::endl;
}
void DQos::prepareSchedule(){
    EV << NOW << " LteMaxCI::schedule " << eNbScheduler_->mac_->getMacNodeId() << endl;

        if (binder_ == nullptr)
            binder_ = getBinder();

        activeConnectionTempSet_ = *activeConnectionSet_;

        //find out which cid has highest priority via qosHandler
        std::map<double, std::vector<QosInfo>> sortedCids = mac_->getQosHandler()->getEqualPriorityMap(dir);
        std::map<double, std::vector<QosInfo>>::reverse_iterator sortedCidsIter = sortedCids.rbegin();

        //create a map with cids which are in the racStatusMap
        std::map<double, std::vector<QosInfo>> activeCids;
        for (auto &var : sortedCids) {
            for (auto &qosinfo : var.second) {
                for (auto &cid : activeConnectionTempSet_) {
                    MacCid realCidQosInfo;
                    if(dir == DL){
                        realCidQosInfo = idToMacCid(qosinfo.destNodeId, qosinfo.lcid);
                    }else{
                        realCidQosInfo = idToMacCid(qosinfo.senderNodeId, qosinfo.lcid);
                    }
                    if (realCidQosInfo == cid) {
                        //we have the QosInfos for the cid in the racStatus
                        activeCids[var.first].push_back(qosinfo);
                    }
                }
            }
        }
        // Build the score list by cycling through the active connections.
                //ScoreList score;
                //MacCid cid =0;
                //unsigned int blocks =0;
                //unsigned int byPs = 0;

               // for ( ActiveSet::iterator it1 = carrierActiveConnectionSet_.begin ();it1 != carrierActiveConnectionSet_.end (); )
                //{
            for (auto &var : activeCids) {

                    for (auto &qosinfos : var.second) {
                    // Current connection.
                    //auto qosInfoD = activeCids[*it1];
                    //cid = *it1;

                    //++it1;
                MacCid cid;
                if (dir == DL) {
                   cid = idToMacCid(qosinfos.destNodeId, qosinfos.lcid);
                  } else {
                   cid = idToMacCid(qosinfos.senderNodeId, qosinfos.lcid);
                    }
                    MacNodeId nodeId = MacCidToNodeId(cid);
                    OmnetId id = binder_->getOmnetId(nodeId);
                    if(nodeId == 0 || id == 0){
                            // node has left the simulation - erase corresponding CIDs
                            activeConnectionSet_->erase(cid);
                            activeConnectionTempSet_.erase(cid);
                            //carrierActiveConnectionSet_.erase(cid);
                            continue;
                    }

                    // if we are allocating the UL subframe, this connection may be either UL or D2D
                    Direction dir;
                    if (direction_ == UL)
                        dir = (MacCidToLcid(cid) == D2D_SHORT_BSR) ? D2D : (MacCidToLcid(cid) == D2D_MULTI_SHORT_BSR) ? D2D_MULTI : direction_;
                    else
                        dir = DL;

                    // compute available blocks for the current user
                    const UserTxParams& info = eNbScheduler_->mac_->getAmc()->computeTxParams(nodeId,dir,carrierFrequency_);
                    const std::set<Band>& bands = info.readBands();
                    std::set<Band>::const_iterator it = bands.begin(),et=bands.end();
                    unsigned int codeword=info.getLayers().size();
                    bool cqiNull=false;
                    for (unsigned int i=0;i<codeword;i++)
                    {
                        if (info.readCqiVector()[i]==0)
                        cqiNull=true;
                    }
                    if (cqiNull)
                    continue;
                    //no more free cw
                    if (eNbScheduler_->allocatedCws(nodeId)==codeword)
                    continue;

                    std::set<Remote>::iterator antennaIt = info.readAntennaSet().begin(), antennaEt=info.readAntennaSet().end();

                    // compute score based on total available bytes
                    unsigned int availableBlocks=0;
                    unsigned int availableBytes =0;
                    // for each antenna
                    for (;antennaIt!=antennaEt;++antennaIt)
                    {
                        // for each logical band
                        for (;it!=et;++it)
                        {
                            availableBlocks += eNbScheduler_->readAvailableRbs(nodeId,*antennaIt,*it);
                            availableBytes += eNbScheduler_->mac_->getAmc()->computeBytesOnNRbs(nodeId,*it, availableBlocks, dir,carrierFrequency_);
                        }
                    }


                    //////////
                    //blocks = availableBlocks;
                    // current user bytes per slot
                    //byPs = (blocks>0) ? (availableBytes/blocks ) : 0;

                    // Create a new score descriptor for the connection, where the score is equal to the ratio between bytes per slot and long term rate
                    //ScoreDesc desc(cid,byPs);
                    // insert the cid score
                    //score.push (desc);

                    //EV << NOW << " LteMaxCI::schedule computed for cid " << cid << " score of " << desc.score_ << endl;
                //}

                if (direction_ == UL || direction_ == DL)  // D2D background traffic not supported (yet?)
                {
                    // query the BgTrafficManager to get the list of backlogged bg UEs to be added to the scorelist. This work
                    // is done by this module itself, so that backgroundTrafficManager is transparent to the scheduling policy in use

                    BackgroundTrafficManager* bgTrafficManager = eNbScheduler_->mac_->getBackgroundTrafficManager(carrierFrequency_);
                    std::list<int>::const_iterator it = bgTrafficManager->getBackloggedUesBegin(direction_),
                                                     et = bgTrafficManager->getBackloggedUesEnd(direction_);

                    int bgUeIndex;
                    int bytesPerBlock;
                    MacNodeId bgUeId;
                    MacCid bgCid;
                    for (; it != et; ++it)
                    {
                        bgUeIndex = *it;
                        bgUeId = BGUE_MIN_ID + bgUeIndex;

                        // the cid for a background UE is a 32bit integer composed as:
                        // - the most significant 16 bits are set to the background UE id (BGUE_MIN_ID+index)
                        // - the least significant 16 bits are set to 0 (lcid=0)
                        bgCid = bgUeId << 16;

                        bytesPerBlock = bgTrafficManager->getBackloggedUeBytesPerBlock(bgUeId, direction_);

                        //ScoreDesc bgDesc(bgCid, bytesPerBlock);
                        //score.push(bgDesc);
                    }
                }

                // Schedule the connections in score order.
                //while ( ! score.empty () )
                //{
                    // Pop the top connection from the list.
                  //  ScoreDesc current = score.top ();

                    bool terminate = false;
                    bool active = true;
                    bool eligible = true;
                    unsigned int granted;
                    granted = requestGrant (cid, 4294967295U, terminate, active, eligible);
                    grantedBytes_[cid] += granted;
                    //if ( MacCidToNodeId(current.x_) >= BGUE_MIN_ID)
                    //{
                      //  EV << NOW << " LteMaxCI::schedule scheduling background UE " << MacCidToNodeId(current.x_) << " with score of " << current.score_ << endl;

                        // Grant data to that background connection.
                    //granted = requestGrantBackground (current.x_, 4294967295U, terminate, active, eligible);

                    //    EV << NOW << "LteMaxCI::schedule granted " << granted << " bytes to background UE " << MacCidToNodeId(current.x_) << endl;
                    //}
                    //else
                    //{
                      //  EV << NOW << " LteMaxCI::schedule scheduling connection " << current.x_ << " with score of " << current.score_ << endl;

                        // Grant data to that connection.
                        //granted = requestGrant (current.x_, 4294967295U, terminate, active, eligible);

                        //EV << NOW << "LteMaxCI::schedule granted " << granted << " bytes to connection " << current.x_ << endl;
                    //}

                    // Exit immediately if the terminate flag is set.
                    if ( terminate ) break;

                    // Pop the descriptor from the score list if the active or eligible flag are clear.
                    if ( ! active || ! eligible )
                    {
                        activeConnectionTempSet_.erase(cid);
                        //score.pop ();
                        //EV << NOW << "LteMaxCI::schedule  connection " << current.x_ << " was found ineligible" << endl;
                    }

                    // Set the connection as inactive if indicated by the grant ().
                    //if ( ! active )
                    //{
                     //   EV << NOW << "LteMaxCI::schedule scheduling connection " << current.x_ << " set to inactive " << endl;

                     //   if ( MacCidToNodeId(current.x_) <= BGUE_MIN_ID)
                       // {
                         //   carrierActiveConnectionSet_.erase(current.x_);
                           // activeConnectionTempSet_.erase (current.x_);
                        //}
                    //}
                }
}
/*
        // Build the score list by cycling through the active connections.
        ScoreList score;
        MacCid cid =0;
        unsigned int blocks =0;
        unsigned int byPs = 0;

        for ( ActiveSet::iterator it1 = carrierActiveConnectionSet_.begin ();it1 != carrierActiveConnectionSet_.end (); )
        {
            // Current connection.
            cid = *it1;

            ++it1;

            MacNodeId nodeId = MacCidToNodeId(cid);
            OmnetId id = binder_->getOmnetId(nodeId);
            if(nodeId == 0 || id == 0){
                    // node has left the simulation - erase corresponding CIDs
                    activeConnectionSet_->erase(cid);
                    activeConnectionTempSet_.erase(cid);
                    carrierActiveConnectionSet_.erase(cid);
                    continue;
            }

            // if we are allocating the UL subframe, this connection may be either UL or D2D
            Direction dir;
            if (direction_ == UL)
                dir = (MacCidToLcid(cid) == D2D_SHORT_BSR) ? D2D : (MacCidToLcid(cid) == D2D_MULTI_SHORT_BSR) ? D2D_MULTI : direction_;
            else
                dir = DL;

            // compute available blocks for the current user
            const UserTxParams& info = eNbScheduler_->mac_->getAmc()->computeTxParams(nodeId,dir,carrierFrequency_);
            const std::set<Band>& bands = info.readBands();
            std::set<Band>::const_iterator it = bands.begin(),et=bands.end();
            unsigned int codeword=info.getLayers().size();
            bool cqiNull=false;
            for (unsigned int i=0;i<codeword;i++)
            {
                if (info.readCqiVector()[i]==0)
                cqiNull=true;
            }
            if (cqiNull)
            continue;
            //no more free cw
            if (eNbScheduler_->allocatedCws(nodeId)==codeword)
            continue;

            std::set<Remote>::iterator antennaIt = info.readAntennaSet().begin(), antennaEt=info.readAntennaSet().end();

            // compute score based on total available bytes
            unsigned int availableBlocks=0;
            unsigned int availableBytes =0;
            // for each antenna
            for (;antennaIt!=antennaEt;++antennaIt)
            {
                // for each logical band
                for (;it!=et;++it)
                {
                    availableBlocks += eNbScheduler_->readAvailableRbs(nodeId,*antennaIt,*it);
                    availableBytes += eNbScheduler_->mac_->getAmc()->computeBytesOnNRbs(nodeId,*it, availableBlocks, dir,carrierFrequency_);
                }
            }

            blocks = availableBlocks;
            // current user bytes per slot
            byPs = (blocks>0) ? (availableBytes/blocks ) : 0;

            // Create a new score descriptor for the connection, where the score is equal to the ratio between bytes per slot and long term rate
            ScoreDesc desc(cid,byPs);
            // insert the cid score
            score.push (desc);

            EV << NOW << " LteMaxCI::schedule computed for cid " << cid << " score of " << desc.score_ << endl;
        }

        if (direction_ == UL || direction_ == DL)  // D2D background traffic not supported (yet?)
        {
            // query the BgTrafficManager to get the list of backlogged bg UEs to be added to the scorelist. This work
            // is done by this module itself, so that backgroundTrafficManager is transparent to the scheduling policy in use

            BackgroundTrafficManager* bgTrafficManager = eNbScheduler_->mac_->getBackgroundTrafficManager(carrierFrequency_);
            std::list<int>::const_iterator it = bgTrafficManager->getBackloggedUesBegin(direction_),
                                             et = bgTrafficManager->getBackloggedUesEnd(direction_);

            int bgUeIndex;
            int bytesPerBlock;
            MacNodeId bgUeId;
            MacCid bgCid;
            for (; it != et; ++it)
            {
                bgUeIndex = *it;
                bgUeId = BGUE_MIN_ID + bgUeIndex;

                // the cid for a background UE is a 32bit integer composed as:
                // - the most significant 16 bits are set to the background UE id (BGUE_MIN_ID+index)
                // - the least significant 16 bits are set to 0 (lcid=0)
                bgCid = bgUeId << 16;

                bytesPerBlock = bgTrafficManager->getBackloggedUeBytesPerBlock(bgUeId, direction_);

                ScoreDesc bgDesc(bgCid, bytesPerBlock);
                score.push(bgDesc);
            }
        }

        // Schedule the connections in score order.
        while ( ! score.empty () )
        {
            // Pop the top connection from the list.
            ScoreDesc current = score.top ();

            bool terminate = false;
            bool active = true;
            bool eligible = true;
            unsigned int granted;

            if ( MacCidToNodeId(current.x_) >= BGUE_MIN_ID)
            {
                EV << NOW << " LteMaxCI::schedule scheduling background UE " << MacCidToNodeId(current.x_) << " with score of " << current.score_ << endl;

                // Grant data to that background connection.
                granted = requestGrantBackground (current.x_, 4294967295U, terminate, active, eligible);

                EV << NOW << "LteMaxCI::schedule granted " << granted << " bytes to background UE " << MacCidToNodeId(current.x_) << endl;
            }
            else
            {
                EV << NOW << " LteMaxCI::schedule scheduling connection " << current.x_ << " with score of " << current.score_ << endl;

                // Grant data to that connection.
                granted = requestGrant (current.x_, 4294967295U, terminate, active, eligible);

                EV << NOW << "LteMaxCI::schedule granted " << granted << " bytes to connection " << current.x_ << endl;
            }

            // Exit immediately if the terminate flag is set.
            if ( terminate ) break;

            // Pop the descriptor from the score list if the active or eligible flag are clear.
            if ( ! active || ! eligible )
            {
                score.pop ();
                EV << NOW << "LteMaxCI::schedule  connection " << current.x_ << " was found ineligible" << endl;
            }

            // Set the connection as inactive if indicated by the grant ().
            if ( ! active )
            {
                EV << NOW << "LteMaxCI::schedule scheduling connection " << current.x_ << " set to inactive " << endl;

                if ( MacCidToNodeId(current.x_) <= BGUE_MIN_ID)
                {
                    carrierActiveConnectionSet_.erase(current.x_);
                    activeConnectionTempSet_.erase (current.x_);
                }
            }
        }*/

}

void DQos::commitSchedule(){
    *activeConnectionSet_ = activeConnectionTempSet_;
}
