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

import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;
import simu5g.common.LteCommon;
import simu5g.x2.packet.X2InformationElement;

namespace simu5g;

// add here new X2 message types
enum LteX2MessageType
{
    X2_COMP_MSG = 0;
    X2_HANDOVER_CONTROL_MSG = 1;
    X2_HANDOVER_DATA_MSG = 2;
    X2_DUALCONNECTIVITY_CONTROL_MSG = 3;
    X2_DUALCONNECTIVITY_DATA_MSG = 4;
    X2_UNKNOWN_MSG = 5;
};

//
// This is the X2 message flowing through X2 interface.
// It contains a vector of Information Elements (see LteX2Message.h),
// the total length of the message and the type of the message
//
class LteX2Message extends inet::FieldsChunk
{
    chunkLength = inet::B(11); // FIXME needs to be checked:
                               // realistic chunk length without X2 information elements?
                               // Must at least be 11 Bytes for serialization:
                               // Type (1 B), sourceId (4 B), destinationId (4 B), IE list size (2 B))
    MacNodeId sourceId;
    MacNodeId destinationId;

    /// type of the X2 message
    LteX2MessageType type = X2_UNKNOWN_MSG;

    /// Size of the X2 message
    int64_t msgLength = 0;

    /// List of X2 IEs
    X2InformationElement* ie[] @owned;
}

cplusplus(LteX2Message) {{
  public:
    /**
     * pushIe() stores an IE inside the
     * X2 IE list in back position and updates msg length
     *
     * @param ie IE to store
     */
    virtual void pushIe(X2InformationElement *ie) {
        appendIe(ie);
        msgLength += ie->getLength();
        // increase the chunk length by length of IE + 1 Byte (required to store the IE type)
        setChunkLength(getChunkLength() + inet::b(8 * (ie->getLength() + sizeof(uint8_t))));
        // EV << "pushIe: pushed an element of length: " << ie->getLength() << " new chunk length: " << getChunkLength() << std::endl;
    }

    /**
     * popIe() pops an IE from the front of
     * the IE list and returns it
     *
     * @return popped IE
     */
    virtual X2InformationElement *popIe() {
        X2InformationElement *ie = removeIe(0);
        eraseIe(0);

        msgLength -= ie->getLength();
        // chunk is immutable during serialization!
        // (chunk length can therefore not be adapted - we only adapt the separate msg_Length_)

        return ie;
    }

    /**
     * hasIe() verifies if there are other
     * IEs inside the IE list
     *
     * @return true if list is not empty, false otherwise
     */
    virtual bool hasIe() const { return ie_arraysize != 0; }

    int64_t getByteLength() const { return msgLength; }

    int64_t getBitLength() const { return getByteLength() * 8; }
}}
