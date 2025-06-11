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


package simu5g.nodes;

import simu5g.corenetwork.statsCollector.NRueStatsCollector;
import simu5g.nodes.Ue;

//
// This module implements a User Equipment (UE) for a 5G LTE network. It extends the ~Ue module
// by implementing a ~NRNicUe as a Network Interface Card (NIC) module. This module can be
// used to model a device that can connect to either a 4G eNodeB or a 5G gNodeB, or both.
//
module NRUeDrb extends NrUe
{
    parameters:
        cellularNic.typename = default("NRNicUeDrb");
}
