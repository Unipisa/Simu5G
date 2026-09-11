//
//                  Simu5G
//
// Copyright (C) 2026 Andras Varga (OpenSim Ltd)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef TESTS_UNIT_LIB_MACTESTSTUBS_H_
#define TESTS_UNIT_LIB_MACTESTSTUBS_H_

#include "simu5g/stack/mac/LteMacEnb.h"
#include "simu5g/stack/mac/LteMacUe.h"
#include "simu5g/stack/mac/buffer/LteMacBuffer.h"

namespace simu5g {
namespace unittest {

/**
 * A bare LteMacUe carrying the inputs of UE-side MAC components under unit
 * test (the LCP scheduler, the BSR size computation). Never initialize()d:
 * the constructor sets the two fields the exercised code paths read, and
 * connections are registered through the REAL API (configureLogicalChannel +
 * createOutgoingConnection), so the registration order a test produces is
 * exactly the order bearer establishment produces in the running model. The
 * virtual buffers are filled directly with PacketInfo entries.
 */
class StubUeMac : public LteMacUe
{
  public:
    // protected seams under test, exposed
    using LteMacUe::computeUlBsrSize;
    using LteMacUe::computeUlBsrSizes;

    StubUeMac()
    {
        nodeId_ = MacNodeId(1025);
        queueSize_ = 1 << 20; // real-buffer cap, read by createOutgoingConnection
    }

    MacCid addConnection(unsigned short lcid, Lcg lcg, Direction dir, bool soFraming = false, RlcMode rlcMode = UM)
    {
        MacCid cid(MacNodeId(1), LogicalCid(lcid));
        LogicalChannelConfig cfg;
        cfg.rlcMode = rlcMode;
        cfg.soFraming = soFraming;
        cfg.snFieldLength = 12;
        cfg.lcg = lcg;
        configureLogicalChannel(cid, cfg);
        // qualified: Binder.h used to leak a global-scope forward declaration
        // of the name; the qualification is kept for robustness
        simu5g::FlowDescriptor flow;
        flow.setSourceId(nodeId_);
        flow.setDestId(MacNodeId(1));
        flow.setDirection(dir);
        createOutgoingConnection(cid, flow);
        return cid;
    }

    void queueSdus(MacCid cid, int count, int bytes)
    {
        LteMacBuffer *vq = connDescOut_.at(cid).buffer;
        for (int i = 0; i < count; i++)
            vq->pushBack(PacketInfo(bytes, omnetpp::simTime()));
    }

    unsigned int occupancy(MacCid cid)
    {
        return connDescOut_.at(cid).buffer->getQueueOccupancy();
    }

    void clearQueue(MacCid cid)
    {
        LteMacBuffer *vq = connDescOut_.at(cid).buffer;
        while (!vq->isEmpty())
            vq->popFront();
    }
};

/**
 * A bare LteMacEnb carrying the inputs of eNB-side MAC components under unit
 * test (the schedule-entry fold, the uplink group's QoS join). Never
 * initialize()d: logical channels are registered through the REAL API
 * (configureLogicalChannel), the way RRC pushes them at bearer establishment,
 * and the folded schedule entries are handed in directly.
 */
class StubEnbMac : public LteMacEnb
{
  public:
    // protected seam under test, exposed
    using LteMacEnb::PerUeGrantBlocks;
    using LteMacEnb::foldScheduleEntries;

    StubEnbMac()
    {
        nodeId_ = MacNodeId(1);
    }

    /// Registers a UE's channel the way bearer establishment does: at the eNB an
    /// uplink flow's channel is keyed by the SENDER's node id.
    void configureChannel(MacNodeId ueId, DrbId drbId, Lcg lcg, RlcMode rlcMode = UM)
    {
        LogicalChannelConfig cfg;
        cfg.rlcMode = rlcMode;
        cfg.soFraming = false;
        cfg.snFieldLength = 12;
        cfg.lcg = lcg;
        configureLogicalChannel(MacCid(ueId, drbIdToLcid(drbId)), cfg);
    }
};

} //namespace unittest
} //namespace simu5g

#endif
