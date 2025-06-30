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
import common.LteCommon;

namespace simu5g;

class LteHarqFeedback extends inet::FieldsChunk
{
    // H-ARQ acid to which this fb is addressed
    unsigned char acid;
    // H-ARQ cw id to which this fb is addressed
    unsigned char cw;
    // H-ARQ feedback: true for ACK, false for NACK
    bool result;
    // Id of the pdu to which the feedback is addressed
    long fbMacPduId;
    // if this flag is true, then the feedback refers to a D2D connection
    bool d2dFeedback = false;
    chunkLength = inet::B(1); // TODO: size 0
}

class LteHarqFeedbackMirror extends LteHarqFeedback
{
    d2dFeedback = true;

    // Id of the D2D Transmitter
    MacNodeId d2dSenderId;
    // Id of the D2D Receiver
    MacNodeId d2dReceiverId;
    // Length (in bytes) of the corresponding PDU
    long pduLength;
}
