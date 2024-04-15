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
import inet.common.packet.Packet;
import inet.common.packet.chunk.Chunk;

cplusplus {{
   using namespace inet;
}};

namespace simu5g;

 
//
// This is the MAC message flowing through LTE stack.
//
class LteMacPdu extends inet::FieldsChunk
{
    @customize(true);
    abstract inet::Packet sdu[];
    unsigned int headerLength = 0;
    // need separate macPduId (e.g. for feedback packets) since OMNET PDU id is automatically updated
    // whenever a new packet is created, e.g. when this PDU is duplicated
    long macPduId = 0;

    //#
    //# Follows a list of elements only present in
    //# the customized class (see LteMacPdu.h):
    //#
    //# MacSduList sduList;
    //# MacControlElementsList ceList;
    //#
}

//
// Mac Control Element
//
class MacControlElement
{
    double timestamp;
}

//
// Mac Buffer Status Report
//
class MacBsr extends MacControlElement
{
    int size;
}
