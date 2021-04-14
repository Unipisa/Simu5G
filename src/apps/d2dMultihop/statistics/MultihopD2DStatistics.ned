//
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.

package simu5g.apps.d2dMultihop.statistics;

//
// MultihopD2DStatistics module
//
// This module is only used to gather statistics related to events generated
// by the Multihop D2D apps. It is accessed via method calls.
//
simple MultihopD2DStatistics {
  
  parameters:

    @signal[d2dMultihopEventDeliveryRatio];
    @statistic[d2dMultihopEventDeliveryRatio](title="Avg delivery ratio within the area"; unit=""; source="d2dMultihopEventDeliveryRatio"; record=mean,sum,min,max);   
    @signal[d2dMultihopEventDelay];
    @statistic[d2dMultihopEventDelay](title="Avg delay of broadcasting within the area"; unit=""; source="d2dMultihopEventDelay"; record=mean,sum,min,max);    
    @signal[d2dMultihopEventDelay95Per];
    @statistic[d2dMultihopEventDelay95Per](title="95th percentile of the delay of broadcasting within the area"; unit=""; source="d2dMultihopEventDelay95Per"; record=mean,sum,min,max);    
    @signal[d2dMultihopEventSentMsg];
    @statistic[d2dMultihopEventSentMsg](title="Avg transmissions for broadcasting a message"; unit=""; source="d2dMultihopEventSentMsg"; record=mean,sum,min,max);  
    @signal[d2dMultihopEventTrickleSuppressedMsg];
    @statistic[d2dMultihopEventTrickleSuppressedMsg](title="Avg suppressed transmissions for broadcasting a message"; unit=""; source="d2dMultihopEventTrickleSuppressedMsg"; record=mean,sum,min,max);  
    @signal[d2dMultihopEventRcvdDupMsg];
    @statistic[d2dMultihopEventRcvdDupMsg](title="Avg num of duplicates within the area"; unit=""; source="d2dMultihopEventRcvdDupMsg"; record=stats);   
    @signal[d2dMultihopEventCompleteDeliveries];
    @statistic[d2dMultihopEventCompleteDeliveries](title="Avg complete-deliveries within the area"; unit=""; source="d2dMultihopEventCompleteDeliveries"; record=stats);   
    
    
    @display("i=block/cogwheel");
}