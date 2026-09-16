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

package simu5g.stack.pdcp;

//
// Base type of the PDCP entity of one data radio bearer: packages the
// transmitting and the receiving side of TS 36.323 / 38.323, plus the leg
// split/join stage of a multi-leg bearer (e.g. an NR dual connectivity split
// bearer). Created dynamically by BearerManagement, one per (peer, DRB) pair.
//
// The lower boundary is a vector of legs, each serving a cell group (the legs
// parameter). A plain (MCG-only) bearer has one leg, and the TX/RX entities
// connect straight to it; a bearer with an SCG leg routes the TX side through
// a splitter (per-PDU leg choice on a split bearer, and per-leg id mapping on
// any SCG leg -- see ~DcPdcpLegSplitter), and a multi-leg bearer additionally
// merges all legs through a joiner into the single RX entity, whose one
// reorder window restores SN order across legs. BearerManagement wires each
// leg to a local RLC entity (UE legs, a master's MCG leg) or to the DcMux/X2
// path (a DC master's SCG leg).
//
// This base type binds no TX/RX entity types; the concrete subclasses do
// (~LtePdcpEntity, ~NrPdcpEntity), so it is not instantiable on its own. A
// mixed entity (e.g. an EN-DC master eNB running an NR TX side with an LTE RX
// side) overrides just tx.typename or rx.typename from the configuration.
//
module PdcpEntityBase
{
    parameters:
        @dynamic(true);
        @display("i=block/wheelbarrow");
        int numLegs = default(1);                  // Number of RLC legs of this bearer
        string legs = default(numLegs == 1 ? "MCG" : "MCG SCG");  // Cell group of each leg, space-separated, in leg-index order
        bool hasSplitter = numLegs > 1 || legs != "MCG";  // The TX side routes through the splitter whenever a leg needs its per-leg id mapping
        int headerCompressedSize @unit(B) = default(-1B);
        *.headerCompressedSize = default(this.headerCompressedSize);
        string rlcMode @enum(TM,UM,AM) = default("UM");   // Forwarded to tx only; determines the PDCP header size
        *.rlcMode = default(this.rlcMode);
        *.numLegs = this.numLegs;
        *.legs = this.legs;
        *.emitPerSduSignals = !this.hasSplitter;   // Where the splitter is present, it emits the per-leg statistics
        *.reorderingEnabled = this.numLegs > 1;    // Reorder the SN stream merged from several legs
    gates:
        input upperIn;            // SDUs from IP (via PdcpMux)
        output upperOut;          // Reassembled SDUs to IP (via PdcpMux)
        output legOut[numLegs];   // PDUs to leg k (RLC entity, or DcMux/X2)
        input legIn[numLegs];     // PDUs from leg k
    submodules:
        tx: <> like IPdcpTxEntity {
            @display("p=50,50");
        }
        rx: <> like IPdcpRxEntity {
            @display("p=150,50");
        }
        splitter: <default("simu5g.stack.pdcp.DcPdcpLegSplitter")> like IPdcpLegSplitter if hasSplitter {
            @display("p=50,130;is=s");
        }
        joiner: <default("simu5g.stack.pdcp.PdcpLegJoiner")> like IPdcpLegJoiner if numLegs > 1 {
            @display("p=150,130;is=s");
        }
    connections:
        upperIn --> { @display("m=n"); } --> tx.in;
        rx.out --> { @display("m=n"); } --> upperOut;

        // TX side: straight to the leg, or through the splitter (leg choice + id mapping)
        tx.out --> { @display("m=s"); } --> legOut[0] if !hasSplitter;
        tx.out --> splitter.in if hasSplitter;
        for i=0..numLegs-1 {
            splitter.out++ --> { @display("m=s"); } --> legOut[i] if hasSplitter;
        }

        // RX side: straight from the single leg, or all legs merged through the joiner
        legIn[0] --> { @display("m=s"); } --> rx.in if numLegs == 1;
        joiner.out --> rx.in if numLegs > 1;
        for i=0..numLegs-1 {
            legIn[i] --> { @display("m=s"); } --> joiner.in++ if numLegs > 1;
        }
}
