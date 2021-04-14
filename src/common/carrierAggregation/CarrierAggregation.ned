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

module CarrierAggregation 
{
  parameters:
    @display("i=block/control");
    int numComponentCarriers = default(1);
 
  submodules:
    componentCarrier[numComponentCarriers] : ComponentCarrier {}
}

simple ComponentCarrier 
{
  parameters:
    @display("i=block/control");
    @class("ComponentCarrier");
    
    // Carrier Frequency (GHz) -->
    double carrierFrequency @unit("GHz")= default(2GHz);
    
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

