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

package simu5g.stack.sdap;

//
// Classifies a UE's uplink packets to QoS flows: the uplink counterpart of
// ~TrafficFlowFilter's QFI assignment, sitting where uplink traffic enters the
// UE's stack just as the TFF sits where downlink traffic enters the tunnel at
// the core network. Each direction is classified once, at its ingress.
//
// The classification rules are delivered by the ~BearerConfigurator (see its
// ulQfiRules parameter, where they can be scoped per UE), through the UE's RRC:
// the same rule grammar the downlink rules use, evaluated in order, first match
// wins. They model the QoS rules that NAS signaling installs into a UE at PDU
// session establishment (TS 23.501 5.7.1.4); this module never authors rules of
// its own.
//
// A matched packet is stamped with a QfiReq tag, which ~NrSdap consumes for its
// QFI-to-DRB mapping. A packet no rule covers is left untagged -- deliberately
// not tagged with 0, because QFI 0 is a real classification (the default flow)
// and absence is what lets SDAP fall back to reflective QoS or the default DRB.
// A packet that already carries a QfiReq (an application that set the QFI
// directly) is not second-guessed. With no rules delivered, SDAP behaves as if
// this module were not there.
//
simple QosFlowClassifier like IQosFlowClassifier
{
    parameters:
        string sessionType;  // The type of the UE's session (see ~LteUe); a non-IP session has one QoS flow, the default one, which every packet is classified to without evaluating the rules
        @display("i=block/classifier");
    gates:
        input upperLayerIn;
        output lowerLayerOut;
}
