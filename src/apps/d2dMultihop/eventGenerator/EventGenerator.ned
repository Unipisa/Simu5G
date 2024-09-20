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

package simu5g.apps.d2dMultihop.eventGenerator;

//
// This module is only used in conjunction with UEs running
// a ~MultihopD2D application instance. It generates periodic
// "events" (e.g., a collision between vehicles) at random
// positions within the network (or by randomly selecting a node)
// and notifies a ~MultihopD2D application via a direct method
// call. The ~MultihopD2D application, in turn, broadcasts
// the notification message to surrounding UEs.
//
simple EventGenerator
{
    parameters:
        string binderModule = default("binder");
        double startTime @unit(s) = default(0.1s);
        volatile double eventPeriod @unit(s) = default(1s);
        bool singleEventSource = default(true);

        bool forceStartingCoord = default(false);
        double startingCoordX @unit(m) = default(-1.0m);
        double startingCoordY @unit(m) = default(-1.0m);

        int startingUe = default(0);

        @display("i=block/cogwheel");
}
