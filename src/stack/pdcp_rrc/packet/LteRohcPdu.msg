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

cplusplus {{
   using namespace inet;
}};

namespace simu5g;

//
// This is the RObust Header Compression PDU
// 
// ROHC is modeled by reducing the compressed headers to a lower size
// as indicated by the headerCompressedSize_ parameter. The additional ROHC header
// is added an allows to restore the compressed headers to their full size when
// decompressing.
//
class LteRohcPdu extends inet::FieldsChunk
{
    inet::B origSizeTransportHeader;
    inet::B origSizeIpHeader;
}
