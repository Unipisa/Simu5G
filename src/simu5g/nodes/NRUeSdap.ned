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
// This module implements a User Equipment (UE) for a 5G LTE network with SDAP support
//
module NRUeSdap extends NrUe
{
    parameters:
        cellularNic.typename = default("NRNicUeSdap");
}
