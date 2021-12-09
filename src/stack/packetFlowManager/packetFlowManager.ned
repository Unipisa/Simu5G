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

package simu5g.stack.packetFlowManager;

moduleinterface PacketFlowManager {
    string pfmType;
}

simple PacketFlowManagerEnb like PacketFlowManager {
    @class("PacketFlowManagerEnb");
    
    string pfmType = default("PacketFlowManagerEnb");

}

simple PacketFlowManagerUe like PacketFlowManager {
    @class("PacketFlowManagerUe");
    
      string pfmType = default("PacketFlowManagerUe");
}

simple NRPacketFlowManagerUe like PacketFlowManager {
    @class("PacketFlowManagerUe");
    
      string pfmType = default("NRPacketFlowManagerUe");
}

simple NRPacketFlowManagerGnb like PacketFlowManager {
    @class("PacketFlowManagerEnb");
    
      string pfmType = default("NRPacketFlowManagerGnb");
}
