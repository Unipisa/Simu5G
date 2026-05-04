#ifndef _LTE_LTEULFEEDBACKGENERATOR_H_
#define _LTE_LTEULFEEDBACKGENERATOR_H_

#include <inet/common/ModuleRefByPar.h>

#include "simu5g/common/cellInfo/CellInfo.h"
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

    LteFeedbackDoubleVector computeUlFeedback(const FeedbackRequest& req, const std::vector<double>& snr, MacNodeId sourceNodeId);
    LteFeedbackDoubleVector computeD2DFeedback(const FeedbackRequest& req, const std::vector<double>& snr, MacNodeId sourceNodeId);
};

} // namespace simu5g

#endif
