//
//                  Simu5G
//
// Authors: Mohamed Seliem (University College Cork)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//
import inet.common.packet.chunk.Chunk;

namespace simu5g;

//
// SDAP header used to represent an SDAP PDU in the NR stack.
// This header is attached to the payload in the SDAP layer.
//
// The D/C (data/control) is not represented, as the simulator
// only models data traffic.
//
class NrSdapHeader extends inet::FieldsChunk {
    chunkLength = inet::B(1);
    uint8_t qfi;                // QoS Flow Identifier (6-bit field in SDAP spec)
    bool reflectiveQoS = false; // Reflective QoS enabled
}
