#ifndef _LTE_LTEULFEEDBACKGENERATOR_H_
#define _LTE_LTEULFEEDBACKGENERATOR_H_

#include <inet/common/ModuleRefByPar.h>

#include "simu5g/common/cellInfo/CellInfo.h"
#include "simu5g/stack/phy/packet/LteAirFrame.h"
#include "simu5g/stack/phy/feedback/LteFeedback.h"
#include "simu5g/stack/phy/feedback/LteFeedbackComputation.h"

namespace simu5g {

class LtePhyEnb;

class LteUlFeedbackGenerator : public cSimpleModule
{
  private:
    inet::ModuleRefByPar<Binder> binder_;
    inet::ModuleRefByPar<LtePhyEnb> phy_;
    opp_component_ptr<CellInfo> cellInfo_;

    FeedbackType fbType_;               /// feedback type (ALLBANDS, PREFERRED, WIDEBAND)
    RbAllocationType rbAllocationType_; /// resource allocation type
    TxMode currentTxMode_;              /// transmission mode to use in feedback generation

    LteFeedbackComputation *lteFeedbackComputation_ = nullptr;
    double targetBler_ = 0.001;
    int numBands_ = 0;
    int numPreferredBands_ = 0;

    void initializeFeedbackComputation();

  protected:
    void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }

  public:
    ~LteUlFeedbackGenerator() override;

    LteFeedbackDoubleVector computeUlFeedback(UserControlInfo *lteinfo, LteAirFrame *frame);
    LteFeedbackDoubleVector computeD2DFeedback(UserControlInfo *lteinfo, LteAirFrame *frame, MacNodeId peerId, const inet::Coord& peerCoord);
};

} // namespace simu5g

#endif
