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

#ifndef _LTE_ENOBSTATSCOLLECTOR_H_
#define _LTE_ENOBSTATSCOLLECTOR_H_

#include <omnetpp.h>

#include <inet/common/ModuleRefByPar.h>

#include "common/LteCommon.h"
#include "nodes/mec/utils/MecCommon.h"
#include <map>
#include "corenetwork/statsCollector/L2Measures/L2MeasBase.h"
#include "common/cellInfo/CellInfo.h"
#include "stack/mac/LteMacEnb.h"
#include "stack/pdcp_rrc/LtePdcpRrc.h"
#include "stack/rlc/um/LteRlcUm.h"
#include "stack/packetFlowManager/PacketFlowManagerEnb.h"

namespace simu5g {
using namespace inet;

/**
 * This is the statistic collector of an eNodeB/gNodeB. It stores all the attributes of the RNI
 * service Layer2Measurements resource. The RNI service will call its methods in order to
 * respond to requests.
 * It holds a map structure with all the UeCollectors of the UEs connected to the
 * eNodeB/gNodeB
 */

class UeStatsCollector;

typedef std::map<MacNodeId, UeStatsCollector *> UeStatsCollectorMap;

class BaseStationStatsCollector : public cSimpleModule
{
  private:
    std::string collectorType_;
    RanNodeType nodeType_; // ENODEB or GNODEB

    // used by the RNI service
    mec::Ecgi ecgi_;

    // LTE NIC layers
    inet::ModuleRefByPar<LtePdcpRrcEnb> pdcp_;
    inet::ModuleRefByPar<LteMacEnb> mac_;
    inet::ModuleRefByPar<LteRlcUm> rlc_;
    inet::ModuleRefByPar<PacketFlowManagerEnb> packetFlowManager_;

    inet::ModuleRefByPar<CellInfo> cellInfo_;

    UeStatsCollectorMap ueCollectors_;

    // L2 Measures per EnodeB
    L2MeasBase dl_total_prb_usage_cell;
    L2MeasBase ul_total_prb_usage_cell;
    L2MeasBase number_of_active_ue_dl_nongbr_cell;
    L2MeasBase number_of_active_ue_ul_nongbr_cell;
    L2MeasBase dl_nongbr_pdr_cell;
    L2MeasBase ul_nongbr_pdr_cell;

    // TODO insert signals for OMNeT++ statistics

    /*
     * timers:
     * - PRB usage (TTI)
     * - # active UEs
     * - Discard rate
     * - Packet delay
     * - PDCP bytes
     * - scheduled throughput
     */

    cMessage *prbUsage_ = nullptr;
    cMessage *activeUsers_ = nullptr;
    cMessage *discardRate_ = nullptr;
    cMessage *packetDelay_ = nullptr;
    cMessage *pdcpBytes_ = nullptr;
    cMessage *tPut_ = nullptr;

    double prbUsagePeriod_;
    double activeUsersPeriod_;
    double discardRatePeriod_;
    double delayPacketPeriod_;
    double dataVolumePeriod_;
    double tPutPeriod_;

  public:
    ~BaseStationStatsCollector() override;

    const mec::Ecgi& getEcgi() const;
    MacCellId getCellId() const;
    RanNodeType getCellNodeType() const
    {
        return nodeType_;
    }

    void setCellNodeType(RanNodeType nodeType)
    {
        nodeType_ = nodeType;
    }

    // UeStatsCollector management methods

    /*
     * addUeCollector adds the UE collector to the list;
     * Called during UE initialization and after handover
     * where the UE moves to the eNodeB/gNodeB of this BaseStationStatsCollector
     * @param id of the UE to add
     * @param ueCollector pointer to the ueStatsCollector
     */
    void addUeCollector(MacNodeId id, UeStatsCollector *ueCollector);

    /*
     * removeUeCollector removes the UE collector from the list;
     * Called during UE termination and after handover
     * where the UE moves to another eNodeB/gNodeB
     * @param id of the UE to remove
     */
    void removeUeCollector(MacNodeId id);

    /*
     * getUeCollector returns:
     *  - the UeStatsCollector of a UE;
     *  - nullptr if nodeId is not present
     *
     * @param id relative to a UeStatsCollector
     */
    UeStatsCollector *getUeCollector(MacNodeId id);
    UeStatsCollectorMap *getCollectorMap();

    /*
     * hasUeCollector returns true if the eNodeB
     * is associated with this BaseStationStatsCollector
     *
     * @param id relative to a UeStatsCollector
     */
    bool hasUeCollector(MacNodeId id);

    // L2 measurements methods per cell

    /*
     * It indicates (in percentage) the PRB usage for UL/DL
     * non-GBR traffic, as defined in ETSI TS 136 314
     */
    void add_dl_total_prb_usage_cell();
    void add_ul_total_prb_usage_cell();

    /*
     * It indicates the number of active UEs with UL/DL non-GBR
     * traffic, as defined in ETSI TS 136 314
     */
    void add_number_of_active_ue_dl_nongbr_cell();
    void add_number_of_active_ue_ul_nongbr_cell();

    /*
     * It indicates the packet discard rate in percentage of the
     * UL/DL non-GBR traffic in a cell, as defined in ETSI
     * TS 136 314
     */
    void add_dl_nongbr_pdr_cell();
    void add_ul_nongbr_pdr_cell();

    /*
     * It indicates (in percentage) the PRB usage for total UL/DL
     * traffic, as defined in ETSI TS 136 314
     */
    int get_dl_total_prb_usage_cell();
    int get_ul_total_prb_usage_cell();

    // getters
    int get_dl_nongbr_prb_usage_cell();
    int get_ul_nongbr_prb_usage_cell();

    int get_number_of_active_ue_dl_nongbr_cell();
    int get_number_of_active_ue_ul_nongbr_cell();

    int get_dl_nongbr_pdr_cell();
    int get_ul_nongbr_pdr_cell();

    // save stats into UeCollectors
    void add_dl_nongbr_delay_perUser();
    void add_ul_nongbr_delay_perUser();
    void add_dl_nongbr_pdr_cell_perUser();
    void add_ul_nongbr_pdr_cell_perUser();
    void add_ul_nongbr_data_volume_ue_perUser();
    void add_dl_nongbr_data_volume_ue_perUser();
    void add_dl_nongbr_throughput_ue_perUser();
    void add_ul_nongbr_throughput_ue_perUser();

    /* getters for GBR (Guaranteed Bit Rate) L2 measures.
     * currently not implemented since the simulator does not
     * handle them
     */
    int get_dl_gbr_prb_usage_cell() { return -1; }
    int get_ul_gbr_prb_usage_cell() { return -1; }
    int get_received_dedicated_preambles_cell() { return -1; }
    int get_received_randomly_selected_preambles_low_range_cell() { return -1; }
    int get_received_randomly_selected_preambles_high_range_cell() { return -1; }
    int get_number_of_active_ue_dl_gbr_cell() { return -1; }
    int get_number_of_active_ue_ul_gbr_cell() { return -1; }
    int get_dl_gbr_pdr_cell() { return -1; }
    int get_ul_gbr_pdr_cell() { return -1; }

    // reset structures to calculate the measure
    void resetDiscardCounterPerUe();
    void resetDelayCounterPerUe();
    void resetBytesCountersPerUe();
    void resetThroughputCountersPerUe();

    void resetStats(MacNodeId nodeId);

  protected:
    void initialize(int stages) override;

    int numInitStages() const override { return INITSTAGE_LAST; }

    void handleMessage(cMessage *msg) override;

};

} //namespace

#endif //_LTE_ENOBSTATSCOLLECTOR_H_

