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

package simu5g.common.carrierAggregation;

//
// This module serves as a descriptor for one Component Carrier (CC) of a Carrier Aggregation (CA)-enabled 4G/5G network.
// The module is assigned a carrier frequency (which also acts as a unique identifier
// for the CC in the system), a bandwidth, and a numerology index (in the range of 0 to 4).
// It can be configured with either Frequency Division Duplex (FDD) or Time
// Division Duplex (TDD) mode. In the latter case, the number of uplink and
// downlink symbols per slot can be specified (the sum of uplink and downlink symbols
// must be less than or equal to 14, which is the total number of symbols in one
// slot).
//
simple ComponentCarrier
{
    parameters:
        @display("i=block/control");
        @class("ComponentCarrier");

        string binderModule = default("binder");

        // Carrier Frequency (GHz) -->
        double carrierFrequency @unit("GHz") = default(2GHz);

        // number of bands for this channel
        int numBands = default(6);

        // numerology index
        int numerologyIndex = default(0);

    // TDD slot format index
    // values in the range [0,61], according to 3GPP TS38.213
    // THIS IS TO BE DONE
//    int tddSlotFormatIndex = default(-1); // -1 stands for FDD

        bool useTdd = default(false);
        int tddNumSymbolsDl = default(7);
        int tddNumSymbolsUl = default(7);
}

