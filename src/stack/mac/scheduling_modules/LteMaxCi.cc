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

#include "stack/mac/scheduling_modules/LteMaxCi.h"
#include "stack/mac/scheduler/LteSchedulerEnb.h"

using namespace omnetpp;

void LteMaxCi::prepareSchedule()
{
    EV << NOW << " LteMaxCI::schedule " << eNbScheduler_->mac_->getMacNodeId() << endl;

    if (binder_ == nullptr)
        binder_ = getBinder();

    activeConnectionTempSet_ = *activeConnectionSet_;

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

    // Schedule the connections in score order.
    while ( ! score.empty () )
    {
        // Pop the top connection from the list.
        ScoreDesc current = score.top ();

        EV << NOW << " LteMaxCI::schedule scheduling connection " << current.x_ << " with score of " << current.score_ << endl;

        // Grant data to that connection.
        bool terminate = false;
        bool active = true;
        bool eligible = true;
        unsigned int granted = requestGrant (current.x_, 4294967295U, terminate, active, eligible);

        EV << NOW << "LteMaxCI::schedule granted " << granted << " bytes to connection " << current.x_ << endl;

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

            carrierActiveConnectionSet_.erase(current.x_);
            activeConnectionTempSet_.erase (current.x_);
        }
    }
}

void LteMaxCi::commitSchedule()
{
    *activeConnectionSet_ = activeConnectionTempSet_;
}

