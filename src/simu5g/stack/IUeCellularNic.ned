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

package simu5g.stack;

//
// Interface for the 3GPP stack of a UE: the parts of the contract that only
// a UE's NIC has.
//
moduleinterface IUeCellularNic extends ICellularNic
{
    parameters:
        string sessionType;  // The type of the UE's session (see ~LteUe)
}
