//
//                  Simu5G
//
// Copyright (C) 2012-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

package simu5g.nodes;

//
// The resource occupation of an ~ExtCell: which bands it allocates in each
// TTI, according to its allocation type, and the transmission on the radio
// medium that stands for it, which the cell's radio sends. The enclosing
// ~ExtCell sets every parameter.
//
simple ExtCellAllocator
{
    parameters:
        @display("i=block/control");
        string binderModule;
        string radioModule = default("^.radio"); // The cell's phantom radio, which sends its transmission
        double carrierFrequency @unit(GHz);
        int numBands;
        string bandAllocationType @enum(FULL_ALLOC,RANDOM_ALLOC,CONTIGUOUS_ALLOC);
        double bandUtilization;
        int startingOffset;
}
