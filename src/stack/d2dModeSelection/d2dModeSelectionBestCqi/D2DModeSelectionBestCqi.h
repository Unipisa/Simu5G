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

#ifndef LTE_D2DMODESELECTIONBESTCQI_H_
#define LTE_D2DMODESELECTIONBESTCQI_H_

#include "stack/d2dModeSelection/D2DModeSelectionBase.h"

//
// D2DModeSelectionBestCqi
//
// For each D2D-capable flow, select the mode having the best CQI
//
class D2DModeSelectionBestCqi : public D2DModeSelectionBase
{

protected:

    // run the mode selection algorithm
    virtual void doModeSelection();

public:
    D2DModeSelectionBestCqi() {}
    virtual ~D2DModeSelectionBestCqi() {}

    virtual void initialize(int stage);
    virtual void doModeSwitchAtHandover(MacNodeId nodeId, bool handoverCompleted);
};

#endif /* LTE_D2DMODESELECTIONBESTCQI_H_ */
