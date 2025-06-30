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

#include "stack/mac/conflict_graph/ConflictGraph.h"

namespace simu5g {

using namespace omnetpp;

/*!
 * \fn ConflictGraph()
 * \memberof ConflictGraph
 * \brief class constructor;
 */
ConflictGraph::ConflictGraph(Binder *binder, LteMacEnbD2D *macEnb, bool reuseD2D, bool reuseD2DMulti) : binder_(binder), macEnb_(macEnb), cellInfo_(macEnb_->getCellInfo()), reuseD2D_(reuseD2D), reuseD2DMulti_(reuseD2DMulti)
{
}

/*!
 * \fn ~ConflictGraph()
 * \memberof ConflictGraph
 * \brief class destructor
 */

// reset Conflict Graph
void ConflictGraph::clearConflictGraph()
{
    conflictGraph_.clear();
}

void ConflictGraph::computeConflictGraph()
{
    EV << " ConflictGraph::computeConflictGraph - START " << endl;

    // --- remove the old one --- //
    clearConflictGraph();

    // --- find the vertices of the graph by scanning the peering map --- //
    std::vector<CGVertex> vertices;
    findVertices(vertices);
    EV << " ConflictGraph::computeConflictGraph - " << vertices.size() << " vertices found" << endl;

    // --- for each CGVertex, find the interfering vertices --- //
    findEdges(vertices);

    EV << " ConflictGraph::computeConflictGraph - END " << endl;
}

void ConflictGraph::printConflictGraph()
{
    EV << " ConflictGraph::printConflictGraph " << endl;

    if (conflictGraph_.empty()) {
        EV << " ConflictGraph::printConflictGraph - No reuse enabled " << endl;
        return;
    }

    EV << "              ";
    for (const auto& [key, value] : conflictGraph_) {
        if (key.isMulticast())
            EV << "| (" << key.srcId << ", *  ) ";
        else
            EV << "| (" << key.srcId << "," << key.dstId << ") ";
    }
    EV << endl;

    for (const auto& [key, value] : conflictGraph_) {
        if (key.isMulticast())
            EV << "| (" << key.srcId << ", *  ) ";
        else
            EV << "| (" << key.srcId << "," << key.dstId << ") ";
        for (const auto& [innerKey, innerValue] : value) {
            if (key == innerKey) {
                EV << "|      -      ";
            }
            else {
                EV << "|      " << innerValue << "      ";
            }
        }
        EV << endl;
    }
    EV << endl;
}

} //namespace

