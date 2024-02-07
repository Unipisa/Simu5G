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


package simu5g.common.binder;

//
// Binder module
//
// It is used to store any information related to
// the network that needs to be accessed via method calls, e.g.
// for control-plane functionalities.
// There must be one (and one only) instance of the module in the network
//
simple Binder
{
    parameters:
        int blerShift = default(0);
        double maxDataRatePerRb @unit("Mbps") = default(1.16Mbps);
        bool printTrafficGeneratorConfig = default(false);
        
        //Qos Characteristics, TS 23.501, V 16.0.2, Chapter 5.7
        string qiValue = "1 2 3 4 65 66 67 75 71 72 73 74 75 5 6 7 8 9 69 70 79 80 82 83 84 85 86"; //
        string resourceType = "GBR GBR GBR GBR GBR GBR GBR GBR GBR GBR GBR GBR GBR NGBR NGBR NGBR NGBR NGBR NGBR NGBR NGBR NGBR DCGBR DCGBR DCGBR DCGBR DCGBR";
        string priorityLevel = "20 40 30 50 7 20 15 0 56 56 56 56 56 10 60 70 80 90 5 55 65 68 19 22 24 21 18";
        string packetDelayBudgetNR = "0.1 0.15 0.05 0.3 0.075 0.1 0.1 0 0.15 0.3 0.3 0.5 0.5 0.1 0.3 0.1 0.3 0.3 0.06 0.2 0.05 0.01 0.01 0.01 0.03 0.005 0.005";//@unit(s)
        string packetErrorRate = "1e-2 1e-3 1e-3 1e-6 1e-2 1e-2 1e-3 0 1e-6 1e-4 1e-8 1e-8 1e-4 1e-6 1e-6 1e-3 1e-6 1e-6 1e-6 1e-6 1e-2 1e-6 1e-4 1e-4 1e-5 1e-5 1e-4";
        string maxDataBurstVolume = "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 255 1354 1354 255 1354";
        string defAveragingWindow = "2 2 2 2 2 2 2 0 2 2 2 2 2 0 0 0 0 0 0 0 0 0 2 2 2 2 2";//@unit(s)
        
        bool exchangeBuffersOnHandover = default(false);
        @display("i=block/cogwheel");
}
