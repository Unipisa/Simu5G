package simu5g.mobility.satellite;

import inet.mobility.base.MovingMobilityBase;

//
// Mobility module for LEO satellites based on the SGP4 propagation model.
//
simple LeoSatMobility extends MovingMobilityBase
{
    parameters:
        @class(LeoSatMobility);
        @display("i=block/cogwheel");
        string satelliteName = default("");
        string tleLine1 = default("");
        string tleLine2 = default("");
        string wallClockSimStartTimeUtc = default("");
        @signal[mobilityStateChanged](type=inet::MobilityBase);
}
