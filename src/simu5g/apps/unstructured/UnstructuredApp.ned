//
//                  Simu5G
//
// Copyright (C) 2026 Andras Varga (OpenSim Ltd)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

package simu5g.apps.unstructured;

import inet.applications.contract.IApp;
import inet.queueing.contract.IActivePacketSource;
import inet.queueing.contract.IPacketFlow;
import inet.queueing.contract.IPassivePacketSink;

//
// An application of a UE whose session is of type Unstructured (the UE's
// sessionType parameter, see ~LteUe): it sends and receives opaque payload through
// the UE's cellular NIC, which carries it over the session to the data network (see
// ~UserPlaneNodeControl for the N6 end of the session). The traffic source and sink
// are built from queueing model elements, as INET's EthernetApp's; ~UnstructuredIo
// attaches them to the NIC. ~UnstructuredSourceApp and ~UnstructuredSinkApp only send
// and only receive. A UE has at most one such application, in its unstructuredApp
// submodule vector.
//
module UnstructuredApp like IApp
{
    parameters:
        string interfaceTableModule;  // Relative module path of the interface table
        *.interfaceTableModule = default(absPath(this.interfaceTableModule));
        @application;
        @display("i=block/app");
    gates:
        input socketIn;
        output socketOut;
    submodules:
        source: <default("ActivePacketSource")> like IActivePacketSource if typename != "" {
            parameters:
                @display("p=200,150");
        }
        measurementStarter: <default("")> like IPacketFlow {
            parameters:
                @display("p=200,300");
        }
        sink: <default("PassivePacketSink")> like IPassivePacketSink if typename != "" {
            parameters:
                @display("p=500,150");
        }
        measurementRecorder: <default("")> like IPacketFlow {
            parameters:
                @display("p=500,300");
        }
        io: UnstructuredIo {
            parameters:
                @display("p=350,450");
        }
    connections allowunconnected:
        source.out --> measurementStarter.in if exists(source);
        measurementStarter.out --> io.trafficIn;
        io.socketOut --> socketOut;

        socketIn --> io.socketIn;
        io.trafficOut --> measurementRecorder.in;
        measurementRecorder.out --> sink.in if exists(sink);
}
