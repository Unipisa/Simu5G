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

//
// This is the RLC Data Pdu (base for all RLC modes)
// flowing through LTE stack: basically
// it contains the fields
// that receiver needs in order to
// correctly decode the original packets
//

import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;
import inet.common.packet.Packet;

import simu5g.stack.rlc.LteRlcDefs;

class size_t {
    @existingClass; // size_t
    @primitive;
    @fromString(string2uint64($));
    @toString(uint642string($));
    @defaultValue(0);
    @toValue((omnetpp::intval_t)($));
    @fromValue(omnetpp::checked_int_cast<size_t>($.intValue()));
}

namespace simu5g;

class LteRlcDataPdu extends inet::FieldsChunk
{
    chunkLength = inet::b(1);

    // Length of the PDU
    int64_t rlcPduLength = 0;

    inet::Packet* sdu[] @sizeGetter(getNumSdu) @owned;
    size_t sduSize[];

    // Sequence number of the PDU
    unsigned int pduSequenceNumber = 0;

    // define the segmentation info for the PDU
    FramingInfo framingInfo;
}

cplusplus(LteRlcDataPdu) {{
  public:
    /**
     * pushSdu() gets ownership of the packet
     * and stores it inside the RLC SDU list
     * in back position
     *
     * @param pkt packet to store
     */
    virtual void pushSdu(inet::Packet *pkt);
    virtual void pushSdu(inet::Packet *pkt, int size);

    /**
     * popSdu() pops a packet from the front of
     * the SDU list and drops ownership before
     * returning it
     *
     * @return popped packet
     */
    virtual inet::Packet *popSdu(size_t& size);
}}

//
// Lte RLC UM Data Pdu
//
// Define additional fields for UM PDU
//
class LteRlcUmDataPdu extends LteRlcDataPdu
{
    rlcPduLength = RLC_HEADER_UM;
    chunkLength = inet::B(RLC_HEADER_UM);
};

//
// Lte RLC AM Data Pdu.
//
// Define additional fields for AM PDU
//
class LteRlcAmDataPdu extends LteRlcDataPdu
{
    rlcPduLength = RLC_HEADER_UM;
    chunkLength = inet::B(RLC_HEADER_UM);

    // if true, a status PDU is required
    bool pollStatus;
};

