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

#ifndef LTE_LTECOMPMANAGERPROPORTIONAL_H_
#define LTE_LTECOMPMANAGERPROPORTIONAL_H_

#include "stack/compManager/LteCompManagerBase.h"
#include "stack/compManager/compManagerProportional/X2CompProportionalRequestIE.h"
#include "stack/compManager/compManagerProportional/X2CompProportionalReplyIE.h"

class LteCompManagerProportional : public LteCompManagerBase {

protected:

    /*
     * Client info
     */
    // amount of requested blocks after provisioning
    unsigned int provisionedBlocks_;

    /*
     * Coordinator info
     */
    // requests from clients
    std::map<X2NodeId, unsigned int> reqBlocksMap_;
    // frame partitioning
    std::vector<unsigned int> partitioning_;
    std::vector<unsigned int> offset_;

    // utility function: convert a vector of double to a vector of integer, preserving the sum of the elements
    std::vector<unsigned int> roundVector(std::vector<double>& vec, int sum);

    virtual void provisionalSchedule();  // run the provisional scheduling algorithm (client side)
    virtual void doCoordination();       // run the coordination algorithm (coordinator side)

    virtual X2CompProportionalRequestIE* buildClientRequest();
    virtual void handleClientRequest(inet::Ptr<X2CompMsg> compMsg);

    virtual X2CompProportionalReplyIE* buildCoordinatorReply(X2NodeId clientId);
    virtual void handleCoordinatorReply(inet::Ptr<X2CompMsg> compMsg);

    UsableBands parseAllowedBlocksMap(std::vector<CompRbStatus>& allowedBlocksMap);

public:
    LteCompManagerProportional() {}
    virtual ~LteCompManagerProportional() {}

    virtual void initialize();
};

#endif /* LTE_LTECOMPMANAGERPROPORTIONAL_H_ */
