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
// This module is only used in conjunction with UEs running instances of the
// ~MultihopD2D application. It collects statistics related to notification messages
// broadcast by the ~MultihopD2D applications by gathering data through method
// calls from these applications.
//
// Each time a ~MultihopD2D application generates a new message (upon notification
// by the ~EventGenerator module), a new event is registered via the
// recordNewBroadcast() function. Each event is tied to a *target area*, which
// represents a defined set of nodes intended to receive the notification of
// the event. The receiving nodes report the event back to this module in order
// to update the statistics related to that event. For instance, upon receiving a
// message, the node will call the recordReception() function to record the
// 'reception' event.
//
simple MultihopD2DStatistics
{
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
