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

package simu5g.stack.sdap;

//
// Implements the SDAP Protocol.
//
// For PDUs received from the PDCP layer, extracts SDAP headers (if present),
// restores the associated QoS Flow based on QFI, and forwards the resulting SDUs
// to the upper layer (IP, Ethernet, or application for unstructured sessions).
//
// For SDUs received from the upper layer, performs QFI-to-DRB mapping,
// encapsulates the data with SDAP headers if needed, and forwards the packets
// to the PDCP layer.
//
// Supports all 3GPP PDU session types (IPv4, IPv6, Ethernet, Unstructured)
// via the pduSessionType field in drbConfig.
//
// Gates:
//  - upperLayerIn/upperLayerOut: connects to the upper layer (Ip2Nic or app).
//  - pdcpIn/pdcpOut: connects to the PDCP layer.
//
simple NrSdap like INrSdap
{
    parameters:
        string nodeRole @enum("UE", "gNB");  // Node role: "UE" or "gNB" - determines reflective QoS behavior
        string binderModule = default("binder");
        object drbConfig = default([]);  // JSON array of DRB configurations: [{drb, ue, qfiList, rlcType, pduSessionType, upperProtocol}, ...]; rlcType defaults to UM, pduSessionType defaults to IPv4, upperProtocol defaults based on pduSessionType.
        string reflectiveQosTableModule = default("");  // UE only: path to ReflectiveQosTable module (empty = disabled)
        bool addSdapHeader = default(true);
        bool useReflectiveQos = default(false); // UE only
        @display("i=block/layer");
    gates:
        input upperLayerIn;     // from upper layer (Ip2Nic or app)
        output upperLayerOut;   // to upper layer (Ip2Nic or app)
        input pdcpIn;           // from PDCP layer
        output pdcpOut;         // to PDCP layer
}
