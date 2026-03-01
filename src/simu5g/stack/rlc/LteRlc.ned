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


package simu5g.stack.rlc;



//
// The LteRlc module implements the functionalities of the Radio Link Control (RLC) layer.
// It provides mechanisms for end-to-end connectivity over the radio interface. It
// sends and receives RLC Service Data Units (SDUs) to and from the Packet Data Convergence
// Protocol (PDCP) layer, and sends and receives RLC Protocol Data Units (PDUs) to and from the
// Medium Access Control (MAC) layer. Depending on the mode, this layer can provide
// segmentation, concatenation, in-sequence delivery, and retransmission of PDUs.
// Currently, only the Unacknowledged Mode (UM) has been implemented.
//
module LteRlc
{
    parameters:
        @display("i=block/transport");
        bool d2dCapable;                                             // inherit the value from the parent module

    gates:
        input upperLayerIn;     // from PDCP
        output upperLayerOut;   // to PDCP
        input macIn;            // from MAC
        output macOut;          // to MAC

    submodules:
        upperMux: RlcUpperMux {
            @display("p=200,50");
        }

        entityManager: RlcEntityManager {
            hasD2DSupport = parent.d2dCapable;
            @display("p=200,100;");
        }

        lowerMux: RlcLowerMux {
            @display("p=200,200");
        }

    connections:
        upperMux.upperLayerIn <-- upperLayerIn;
        upperMux.upperLayerOut --> upperLayerOut;

        lowerMux.macIn <-- macIn;
        lowerMux.macOut --> macOut;
        lowerMux.toUpperMux --> upperMux.fromLowerMux;
}

