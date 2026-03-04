//
//                  Simu5G
//
// Authors: Andras Varga (OpenSim Ltd)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

package simu5g.stack.sdap.common;

//
// Interface for Reflective QoS Table modules.
//
// This interface defines the contract for modules that implement reflective QoS
// functionality according to 3GPP TS 37.324. Reflective QoS allows UEs to derive
// uplink QoS flow characteristics from downlink traffic patterns.
//
// Implementing modules should provide:
// - Flow learning from downlink packets with reflective QoS flags
// - Uplink QFI derivation based on learned flow patterns
// - Automatic flow expiration and cleanup
// - Configurable timeout and cleanup parameters
//
// The interface allows for different implementations of reflective QoS tables
// while maintaining a consistent API for SDAP modules.
//
moduleinterface IReflectiveQosTable
{
    parameters:
        @display("i=block/table");
}
