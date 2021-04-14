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

#ifndef DISTANCEBASEDCONFLICTGRAPH_H
#define	DISTANCEBASEDCONFLICTGRAPH_H

#include "stack/mac/conflict_graph/ConflictGraph.h"

class DistanceBasedConflictGraph : public ConflictGraph
{
    // path loss-based thresholds (used by default)
    double d2dDbmThreshold_;
    double d2dMultiTxDbmThreshold_;
    double d2dMultiInterfDbmThreshold_;

    // distance-based thresholds
    double d2dInterferenceRadius_;
    double d2dMultiTransmissionRadius_;
    double d2dMultiInterferenceRadius_;

    // reference to the phy layer
    LtePhyBase* phyEnb_;

    // utility function to convert a distance to dBm according to the channel model
    double getDbmFromDistance(double distance);

    // overridden functions
    virtual void findVertices(std::vector<CGVertex>& vertices);
    virtual void findEdges(const std::vector<CGVertex>& vertices);

public:
    DistanceBasedConflictGraph(LteMacEnbD2D* macEnb, bool reuseD2D, bool reuseD2DMulti, double dbmThresh);
    virtual ~DistanceBasedConflictGraph() { }

    // set distance thresholds
    void setThresholds(double d2dInterferenceRadius = -1.0, double d2dMultiTransmissionRadius = -1.0, double d2dMultiInterferenceRadius = -1.0);
};

#endif	/* DISTANCEBASEDCONFLICTGRAPH_H */

