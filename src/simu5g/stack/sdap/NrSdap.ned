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
// to the PDCP layer. On the gNB DL path, the QFI is always taken from the
// QfiReq tag (set by GtpUser). On the UE UL path two sources may answer: the
// classified QFI (the QfiReq tag, stamped by ~QosFlowClassifier's rules or set
// by the application directly) and a reflective QoS match, the rule derived
// from observed downlink traffic. When both answer, reflectiveQosOverridesQfi
// says which wins; one alone is used as-is; a packet with neither joins the
// default QoS flow (QFI 0, the default DRB).
//
// Whether a bearer's packets carry an SDAP header is the control plane's
// decision, delivered per bearer in the pushed configuration (computed by
// ~BearerConfigurator: a header is needed exactly when the QFI could not be
// recovered from the DRB alone on the RX side, unless the configuration
// suppresses it -- see the suppressSdapHeader entry field there). On a
// headerless bearer both ends agree on the one QFI it carries -- its sole
// mapped QFI, or QFI 0 when none is mapped: RX assigns that value to every
// packet, and TX verifies each packet against it, stopping with an error when
// another QoS flow shows up.
//
// Flows of the D2D machinery are outside SDAP and pass through this module
// untouched in both directions: their bearers are peer-keyed, carry no QoS
// flows, and ~Ip2Nic assigns them (3GPP sidelink has its own SL-SDAP over
// PC5, which is not modeled). This holds in either mode -- a D2D-capable
// pair's flow keeps its ownership while in infrastructure mode, since the
// mode switch tears down and re-establishes within that machinery.
//
// Supports all 3GPP PDU session types (IPv4, IPv6, Ethernet, Unstructured)
// via the pduSessionType field of the bearer configuration.
//
// SDAP does not author its own configuration: the QFI-to-DRB mapping and the
// per-bearer SDAP settings are authored network-wide (see the staticDrbs
// parameter of ~BearerConfigurator), delivered to RRC, and pushed into this module from
// there at initialization.
//
// Gates:
//  - upperLayerIn/upperLayerOut: connects to the upper layer (Ip2Nic or app).
//  - pdcpIn/pdcpOut: connects to the PDCP layer.
//
simple NrSdap like INrSdap
{
    parameters:
        string nodeRole @enum("UE", "gNB");  // Node role: "UE" or "gNB" - determines reflective QoS behavior
        string bearerConfiguratorModule = default("bearerConfigurator");
        bool establishBearersOnDemand = default(true);  // When false, a packet that finds no established bearer raises an error instead of establishing one (static bearers are established up front; see the staticDrbs parameter of ~BearerConfigurator)
        string reflectiveQosTableModule = default("");  // UE only: path to ReflectiveQosTable module (empty = disabled)
        bool useReflectiveQos = default(false); // UE only
        bool reflectiveQosOverridesQfi = default(false);  // UE only: when a packet has both a classified QFI (QfiReq tag) and a reflective QoS match, true = the reflective QFI wins, false = the classified one; a packet with only one of the two always uses it
        @display("i=block/layer");
    gates:
        input upperLayerIn;     // From upper layer (Ip2Nic or app)
        output upperLayerOut;   // To upper layer (Ip2Nic or app)
        input pdcpIn;           // From PDCP layer
        output pdcpOut;         // To PDCP layer
}
