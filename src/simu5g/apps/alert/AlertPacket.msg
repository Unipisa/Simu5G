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


import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;

namespace simu5g;

//
// Packet used by the ~AlertSender and ~AlertReceiver modules.
//
class AlertPacket extends inet::FieldsChunk
{
    unsigned int sno;
    simtime_t payloadTimestamp;
}
