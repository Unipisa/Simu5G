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

using namespace omnetpp;

/*!
 * \fn ConflictGraph()
 * \memberof ConflictGraph
 * \brief class constructor;
 */
ConflictGraph::ConflictGraph(LteMacEnbD2D* macEnb, bool reuseD2D, bool reuseD2DMulti)
{
    conflictGraph_.clear();
    macEnb_ = macEnb;
    cellInfo_ = macEnb_->getCellInfo();

    reuseD2D_ = reuseD2D;
    reuseD2DMulti_ = reuseD2DMulti;
}

/*!
 * \fn ~ConflictGraph()
 * \memberof ConflictGraph
 * \brief class destructor
 */
ConflictGraph::~ConflictGraph()
{
    clearConflictGraph();
}

// reset Conflict Graph
void ConflictGraph::clearConflictGraph()
{
    conflictGraph_.clear();
}

void ConflictGraph::computeConflictGraph()
{
    EV << " ConflictGraph::computeConflictGraph - START "<<endl;

    // --- remove the old one --- //
    clearConflictGraph();

    // --- find the vertices of the graph by scanning the peering map --- //
    std::vector<CGVertex> vertices;
    findVertices(vertices);
    EV << " ConflictGraph::computeConflictGraph - " << vertices.size() << " vertices found" << endl;

    // --- for each CGVertex, find the interfering vertices --- //
    findEdges(vertices);

    EV << " ConflictGraph::computeConflictGraph - END "<<endl;

}

void ConflictGraph::printConflictGraph()
{
    EV << " ConflictGraph::printConflictGraph "<<endl;

    if (conflictGraph_.empty())
    {
        EV << " ConflictGraph::printConflictGraph - No reuse enabled "<<endl;
        return;
    }

    EV << "              ";
    CGMatrix::iterator it = conflictGraph_.begin(), et = conflictGraph_.end();
    for (; it != et; ++it)
    {
        if (it->first.isMulticast())
            EV << "| (" << it->first.srcId << ", *  ) ";
        else
            EV << "| (" << it->first.srcId << "," << it->first.dstId <<") ";
    }
    EV << endl;

    it = conflictGraph_.begin();
    for (; it != et; ++it)
    {
        if (it->first.isMulticast())
            EV << "| (" << it->first.srcId << ", *  ) ";
        else
            EV << "| (" << it->first.srcId << "," << it->first.dstId <<") ";
        std::map<CGVertex, bool>::iterator jt = it->second.begin();
        for (; jt != it->second.end(); ++jt)
        {
            if (it->first == jt->first)
            {
                EV << "|      -      ";
            }
            else
            {
                EV << "|      " << jt->second << "      ";
            }
        }
        EV << endl;
    }
    EV << endl;
}
