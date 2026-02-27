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


package simu5g.stack.pdcp;

//
// Compound module wrapping the PDCP mux (packet dispatcher and entity manager)
// as a submodule. Implements ~ILtePdcp so it can be used as a drop-in
// replacement for the simple-module PDCP types (LtePdcpEnb, NrPdcpEnb, etc.)
// in any NIC module.
//
// All parameters are forwarded to the inner mux submodule.
//
module PdcpLayer like ILtePdcp
{
    parameters:
        @display("i=block/fork");
        int headerCompressedSize @unit(B) = default(-1B);
        string binderModule = default("binder");
        string rxEntityModuleType = default("simu5g.stack.pdcp.LteRxPdcpEntity");
        string txEntityModuleType = default("simu5g.stack.pdcp.LteTxPdcpEntity");
        string bypassRxEntityModuleType = default("simu5g.stack.pdcp.BypassRxPdcpEntity");
        string bypassTxEntityModuleType = default("simu5g.stack.pdcp.BypassTxPdcpEntity");
        bool isNR = default(false);
        bool hasD2DSupport = default(false);

    gates:
        input upperLayerIn;
        output upperLayerOut;
        input rlcIn;
        output rlcOut;
        input nrRlcIn @loose;
        output nrRlcOut @loose;
        input dcManagerIn @loose;
        output dcManagerOut @loose;

    submodules:
        mux: LtePdcp {
            headerCompressedSize = parent.headerCompressedSize;
            binderModule = parent.binderModule;
            rxEntityModuleType = parent.rxEntityModuleType;
            txEntityModuleType = parent.txEntityModuleType;
            bypassRxEntityModuleType = parent.bypassRxEntityModuleType;
            bypassTxEntityModuleType = parent.bypassTxEntityModuleType;
            isNR = parent.isNR;
            hasD2DSupport = parent.hasD2DSupport;
            @display("p=100,100");
        }

    connections:
        upperLayerIn --> mux.upperLayerIn;
        mux.upperLayerOut --> upperLayerOut;
        rlcIn --> mux.rlcIn;
        mux.rlcOut --> rlcOut;
        nrRlcIn --> mux.nrRlcIn if mux.isNR;
        mux.nrRlcOut --> nrRlcOut if mux.isNR;
        dcManagerIn --> mux.dcManagerIn if mux.isNR;
        mux.dcManagerOut --> dcManagerOut if mux.isNR;
}
