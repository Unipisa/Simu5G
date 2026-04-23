package simu5g.mobility.geographic;

simple GeographicReferenceSystem
{
    parameters:
        @display("i=misc/globe");
        double x @unit(m) = default(0m);
        double y @unit(m) = default(0m);
        double z @unit(m) = default(0m);
        double referenceLatitude @unit(deg) = default(0deg);
        double referenceLongitude @unit(deg) = default(0deg);
        double referenceAltitude @unit(m) = default(0m);
}
