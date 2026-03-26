//
//                  Simu5G
//
// Copyright (C) 2012-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

import inet.common.packet.chunk.Chunk;

namespace simu5g;

//
// Represents the PDCP header in LTE and NR.
//
class LtePdcpHeader extends inet::FieldsChunk
{
    chunkLength = inet::B(1);

    unsigned int sequenceNumber = 0;  // PDCP sequence number (7, 12 or 18 bits)
}
