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

class LteRlcDataPdu extends inet::FieldsChunk
{
    @customize(true);
}
