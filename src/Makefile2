#
# OMNeT++/OMNEST Makefile for $(LIB_PREFIX)simu5g
#
# This file was generated with the command:
#  opp_makemake --make-so -f --deep -o simu5g -O out -KINET_PROJ=../../inet -KVEINS_PROJ=../../veins -KVEINS_INET_PROJ=../../veins_inet -DINET_IMPORT -DVEINS_IMPORT -DVEINS_INET_IMPORT -I. -I$$\(INET_PROJ\)/src -I$$\(VEINS_PROJ\)/src -I. -I$$\(VEINS_INET_PROJ\)/src -L$$\(INET_PROJ\)/src -L$$\(VEINS_PROJ\)/src -L$$\(VEINS_INET_PROJ\)/src -lINET$$\(D\) -lveins$$\(D\) -lveins_inet$$\(D\)
#

# Name of target to be created (-o option)
TARGET_DIR = .
TARGET_NAME = $(LIB_PREFIX)simu5g$(D)
TARGET = $(TARGET_NAME)$(SHARED_LIB_SUFFIX)
TARGET_IMPLIB = $(TARGET_NAME)$(IMPLIB_SUFFIX)
TARGET_IMPDEF = $(TARGET_NAME)$(IMPDEF_SUFFIX)
TARGET_FILES = $(TARGET_DIR)/$(TARGET)

# C++ include paths (with -I)
INCLUDE_PATH = -I. -I$(INET_PROJ)/src -I$(VEINS_PROJ)/src -I. -I$(VEINS_INET_PROJ)/src

# Additional object and library files to link with
EXTRA_OBJS =

# Additional libraries (-L, -l options)
LIBS = $(LDFLAG_LIBPATH)$(INET_PROJ)/src $(LDFLAG_LIBPATH)$(VEINS_PROJ)/src $(LDFLAG_LIBPATH)$(VEINS_INET_PROJ)/src  -lINET$(D) -lveins$(D) -lveins_inet$(D)

# Output directory
PROJECT_OUTPUT_DIR = ../out
PROJECTRELATIVE_PATH = src
O = $(PROJECT_OUTPUT_DIR)/$(CONFIGNAME)/$(PROJECTRELATIVE_PATH)

# Object files for local .cc, .msg and .sm files
OBJS = \
    $O/apps/PingPong/MECPingPongApp.o \
    $O/apps/PingPong/UEPingPongApp.o \
    $O/apps/PingPong/utility.o \
    $O/apps/PingPong/packets/Serializers/PingPongPacketSerializer.o \
    $O/apps/alert/AlertReceiver.o \
    $O/apps/alert/AlertSender.o \
    $O/apps/burst/BurstReceiver.o \
    $O/apps/burst/BurstSender.o \
    $O/apps/cbr/CbrReceiver.o \
    $O/apps/cbr/CbrSender.o \
    $O/apps/d2dMultihop/MultihopD2D.o \
    $O/apps/d2dMultihop/eventGenerator/EventGenerator.o \
    $O/apps/d2dMultihop/statistics/MultihopD2DStatistics.o \
    $O/apps/mec/DeviceApp/DeviceApp.o \
    $O/apps/mec/DeviceApp/DeviceAppMessages/Serializers/DeviceAppPacketSerializer.o \
    $O/apps/mec/MecApps/MecAppBase.o \
    $O/apps/mec/MecApps/MecRequestBackgroundApp/MecRequestBackgroundApp.o \
    $O/apps/mec/MecApps/MecRequestBackgroundGeneratorApp/MecRequestBackgroundGeneratorApp.o \
    $O/apps/mec/MecApps/MecRequestForegroundApp/MecRequestForegroundApp.o \
    $O/apps/mec/MecRequestResponseApp/MECResponseApp.o \
    $O/apps/mec/MecRequestResponseApp/UERequestApp.o \
    $O/apps/mec/RealTimeVideoStreamingApp/MecRTVideoStreamingReceiver.o \
    $O/apps/mec/RealTimeVideoStreamingApp/RTVideoStreamingSender.o \
    $O/apps/mec/WarningAlert/MECWarningAlertApp.o \
    $O/apps/mec/WarningAlert/UEWarningAlertApp.o \
    $O/apps/mec/WarningAlert/packets/Serializers/WarningAlertPacketSerializer.o \
    $O/apps/mecRequestResponseApp/MecRequestApp.o \
    $O/apps/mecRequestResponseApp/MecResponseApp.o \
    $O/apps/vod/VoDUDPClient.o \
    $O/apps/vod/VoDUDPServer.o \
    $O/apps/voip/VoipPacketSerializer.o \
    $O/apps/voip/VoIPReceiver.o \
    $O/apps/voip/VoIPSender.o \
    $O/common/LteCommon.o \
    $O/common/LteControlInfo.o \
    $O/common/binder/Binder.o \
    $O/common/blerCurves/PhyPisaData.o \
    $O/common/carrierAggregation/ComponentCarrier.o \
    $O/common/cellInfo/CellInfo.o \
    $O/common/timer/TTimer.o \
    $O/common/utils/utlis.o \
    $O/corenetwork/gtp/GtpUser.o \
    $O/corenetwork/gtp/GtpUserMsgSerializer.o \
    $O/corenetwork/gtp/GtpUserX2.o \
    $O/corenetwork/statsCollector/BaseStationStatsCollector.o \
    $O/corenetwork/statsCollector/UeStatsCollector.o \
    $O/corenetwork/statsCollector/L2Measures/L2MeasBase.o \
    $O/corenetwork/trafficFlowFilter/TrafficFlowFilter.o \
    $O/mobility/trafficLightMobility/TrafficLightController.o \
    $O/mobility/trafficLightMobility/TrafficLightMobility.o \
    $O/nodes/ExtCell.o \
    $O/nodes/backgroundCell/BackgroundCellAmc.o \
    $O/nodes/backgroundCell/BackgroundCellAmcNr.o \
    $O/nodes/backgroundCell/BackgroundCellChannelModel.o \
    $O/nodes/backgroundCell/BackgroundCellTrafficManager.o \
    $O/nodes/backgroundCell/BackgroundScheduler.o \
    $O/nodes/mec/MECOrchestrator/MecOrchestrator.o \
    $O/nodes/mec/MECOrchestrator/ApplicationDescriptor/ApplicationDescriptor.o \
    $O/nodes/mec/MECOrchestrator/mecHostSelectionPolicies/AvailableResourcesSelectionBased.o \
    $O/nodes/mec/MECOrchestrator/mecHostSelectionPolicies/MecHostSelectionBased.o \
    $O/nodes/mec/MECOrchestrator/mecHostSelectionPolicies/MecServiceSelectionBased.o \
    $O/nodes/mec/MECPlatform/EventNotification/CircleNotificationEvent.o \
    $O/nodes/mec/MECPlatform/EventNotification/EventNotification.o \
    $O/nodes/mec/MECPlatform/MECServices/LocationService/LocationService.o \
    $O/nodes/mec/MECPlatform/MECServices/LocationService/resources/CircleNotificationSubscription.o \
    $O/nodes/mec/MECPlatform/MECServices/LocationService/resources/CurrentLocation.o \
    $O/nodes/mec/MECPlatform/MECServices/LocationService/resources/LocationApiDefs.o \
    $O/nodes/mec/MECPlatform/MECServices/LocationService/resources/LocationInfo.o \
    $O/nodes/mec/MECPlatform/MECServices/LocationService/resources/LocationResource.o \
    $O/nodes/mec/MECPlatform/MECServices/LocationService/resources/TerminalLocation.o \
    $O/nodes/mec/MECPlatform/MECServices/LocationService/resources/User.o \
    $O/nodes/mec/MECPlatform/MECServices/LocationService/resources/UserInfo.o \
    $O/nodes/mec/MECPlatform/MECServices/MECServiceBase/MecServiceBase.o \
    $O/nodes/mec/MECPlatform/MECServices/MECServiceBase/SocketManager.o \
    $O/nodes/mec/MECPlatform/MECServices/RNIService/RNIService.o \
    $O/nodes/mec/MECPlatform/MECServices/RNIService/resources/AssociateId.o \
    $O/nodes/mec/MECPlatform/MECServices/RNIService/resources/CellUEInfo.o \
    $O/nodes/mec/MECPlatform/MECServices/RNIService/resources/Ecgi.o \
    $O/nodes/mec/MECPlatform/MECServices/RNIService/resources/L2Meas.o \
    $O/nodes/mec/MECPlatform/MECServices/RNIService/resources/L2MeasSubscription.o \
    $O/nodes/mec/MECPlatform/MECServices/RNIService/resources/MeasRepUeSubscription.o \
    $O/nodes/mec/MECPlatform/MECServices/RNIService/resources/Plmn.o \
    $O/nodes/mec/MECPlatform/MECServices/RNIService/resources/RNICellInfo.o \
    $O/nodes/mec/MECPlatform/MECServices/Resources/AttributeBase.o \
    $O/nodes/mec/MECPlatform/MECServices/Resources/SubscriptionBase.o \
    $O/nodes/mec/MECPlatform/MECServices/Resources/TimeStamp.o \
    $O/nodes/mec/MECPlatform/MECServices/packets/AperiodicSubscriptionTimer.o \
    $O/nodes/mec/MECPlatform/MECServices/packets/HttpRequestMessage/HttpRequestMessage.o \
    $O/nodes/mec/MECPlatform/MECServices/packets/HttpRequestMessage/Serializers/HttpRequestMessageSerializer.o \
    $O/nodes/mec/MECPlatform/MECServices/packets/HttpResponseMessage/HttpResponseMessage.o \
    $O/nodes/mec/MECPlatform/MECServices/packets/HttpResponseMessage/Serializers/HttpResponseMessageSerializer.o \
    $O/nodes/mec/MECPlatform/ServiceRegistry/ServiceRegistry.o \
    $O/nodes/mec/MECPlatform/ServiceRegistry/resources/CategoryRef.o \
    $O/nodes/mec/MECPlatform/ServiceRegistry/resources/EndPointInfo.o \
    $O/nodes/mec/MECPlatform/ServiceRegistry/resources/ServiceInfo.o \
    $O/nodes/mec/MECPlatform/ServiceRegistry/resources/TransportInfo.o \
    $O/nodes/mec/MECPlatformManager/MecPlatformManager.o \
    $O/nodes/mec/UALCMP/UALCMPApp.o \
    $O/nodes/mec/UALCMP/UALCMPMessages/CreateContextAppAckMessage.o \
    $O/nodes/mec/UALCMP/UALCMPMessages/CreateContextAppMessage.o \
    $O/nodes/mec/VirtualisationInfrastructureManager/VirtualisationInfrastructureManager.o \
    $O/nodes/mec/utils/httpUtils/httpUtils.o \
    $O/stack/backgroundTrafficGenerator/BackgroundTrafficManager.o \
    $O/stack/backgroundTrafficGenerator/generators/TrafficGeneratorBase.o \
    $O/stack/backgroundTrafficGenerator/generators/TrafficGeneratorCbr.o \
    $O/stack/compManager/LteCompManagerBase.o \
    $O/stack/compManager/X2CompMsg.o \
    $O/stack/compManager/compManagerProportional/LteCompManagerProportional.o \
    $O/stack/d2dModeSelection/D2DModeSelectionBase.o \
    $O/stack/d2dModeSelection/d2dModeSelectionBestCqi/D2DModeSelectionBestCqi.o \
    $O/stack/dualConnectivityManager/DualConnectivityManager.o \
    $O/stack/dualConnectivityManager/X2DualConnectivityDataMsg.o \
    $O/stack/handoverManager/LteHandoverManager.o \
    $O/stack/handoverManager/X2HandoverControlMsg.o \
    $O/stack/ip2nic/IP2Nic.o \
    $O/stack/ip2nic/SplitBearersTable.o \
    $O/stack/mac/allocator/LteAllocationModule.o \
    $O/stack/mac/allocator/LteAllocationModuleFrequencyReuse.o \
    $O/stack/mac/amc/AmcPilotAuto.o \
    $O/stack/mac/amc/AmcPilotD2D.o \
    $O/stack/mac/amc/LteAmc.o \
    $O/stack/mac/amc/LteMcs.o \
    $O/stack/mac/amc/NRAmc.o \
    $O/stack/mac/amc/NRMcs.o \
    $O/stack/mac/amc/UserTxParams.o \
    $O/stack/mac/buffer/LteMacBuffer.o \
    $O/stack/mac/buffer/LteMacQueue.o \
    $O/stack/mac/buffer/harq/LteHarqBufferRx.o \
    $O/stack/mac/buffer/harq/LteHarqBufferTx.o \
    $O/stack/mac/buffer/harq/LteHarqProcessRx.o \
    $O/stack/mac/buffer/harq/LteHarqProcessTx.o \
    $O/stack/mac/buffer/harq/LteHarqUnitTx.o \
    $O/stack/mac/buffer/harq_d2d/LteHarqBufferMirrorD2D.o \
    $O/stack/mac/buffer/harq_d2d/LteHarqBufferRxD2D.o \
    $O/stack/mac/buffer/harq_d2d/LteHarqBufferTxD2D.o \
    $O/stack/mac/buffer/harq_d2d/LteHarqProcessMirrorD2D.o \
    $O/stack/mac/buffer/harq_d2d/LteHarqProcessRxD2D.o \
    $O/stack/mac/buffer/harq_d2d/LteHarqProcessTxD2D.o \
    $O/stack/mac/buffer/harq_d2d/LteHarqUnitTxD2D.o \
    $O/stack/mac/conflict_graph/ConflictGraph.o \
    $O/stack/mac/conflict_graph/DistanceBasedConflictGraph.o \
    $O/stack/mac/layer/LteMacBase.o \
    $O/stack/mac/layer/LteMacEnb.o \
    $O/stack/mac/layer/LteMacEnbD2D.o \
    $O/stack/mac/layer/LteMacUe.o \
    $O/stack/mac/layer/LteMacUeD2D.o \
    $O/stack/mac/layer/NRMacGnb.o \
    $O/stack/mac/layer/NRMacUe.o \
    $O/stack/mac/packet/LteMacPdu.o \
    $O/stack/mac/scheduler/LcgScheduler.o \
    $O/stack/mac/scheduler/LteScheduler.o \
    $O/stack/mac/scheduler/LteSchedulerEnb.o \
    $O/stack/mac/scheduler/LteSchedulerEnbDl.o \
    $O/stack/mac/scheduler/LteSchedulerEnbUl.o \
    $O/stack/mac/scheduler/LteSchedulerUeUl.o \
    $O/stack/mac/scheduler/NRSchedulerGnbUl.o \
    $O/stack/mac/scheduling_modules/LteAllocatorBestFit.o \
    $O/stack/mac/scheduling_modules/LteDrr.o \
    $O/stack/mac/scheduling_modules/LteMaxCi.o \
    $O/stack/mac/scheduling_modules/LteMaxCiComp.o \
    $O/stack/mac/scheduling_modules/LteMaxCiMultiband.o \
    $O/stack/mac/scheduling_modules/LteMaxCiOptMB.o \
    $O/stack/mac/scheduling_modules/LtePf.o \
    $O/stack/packetFlowManager/PacketFlowManagerBase.o \
    $O/stack/packetFlowManager/PacketFlowManagerEnb.o \
    $O/stack/packetFlowManager/PacketFlowManagerUe.o \
    $O/stack/pdcp_rrc/ConnectionsTable.o \
    $O/stack/pdcp_rrc/layer/LtePdcpRrc.o \
    $O/stack/pdcp_rrc/layer/LtePdcpRrcEnbD2D.o \
    $O/stack/pdcp_rrc/layer/LtePdcpRrcUeD2D.o \
    $O/stack/pdcp_rrc/layer/NRPdcpRrcEnb.o \
    $O/stack/pdcp_rrc/layer/NRPdcpRrcUe.o \
    $O/stack/pdcp_rrc/layer/entity/LteRxPdcpEntity.o \
    $O/stack/pdcp_rrc/layer/entity/LteTxPdcpEntity.o \
    $O/stack/pdcp_rrc/layer/entity/NRRxPdcpEntity.o \
    $O/stack/pdcp_rrc/layer/entity/NRTxPdcpEntity.o \
    $O/stack/phy/ChannelModel/LteChannelModel.o \
    $O/stack/phy/ChannelModel/LteDummyChannelModel.o \
    $O/stack/phy/ChannelModel/LteRealisticChannelModel.o \
    $O/stack/phy/ChannelModel/NRChannelModel.o \
    $O/stack/phy/ChannelModel/NRChannelModel_3GPP38_901.o \
    $O/stack/phy/das/DasFilter.o \
    $O/stack/phy/das/RemoteAntennaSet.o \
    $O/stack/phy/feedback/LteDlFeedbackGenerator.o \
    $O/stack/phy/feedback/LteFeedback.o \
    $O/stack/phy/feedback/LteFeedbackComputation.o \
    $O/stack/phy/feedback/LteFeedbackComputationRealistic.o \
    $O/stack/phy/feedback/LteSummaryBuffer.o \
    $O/stack/phy/feedback/LteSummaryFeedback.o \
    $O/stack/phy/feedback/testFeedback/FeedbackTester.o \
    $O/stack/phy/layer/LtePhyBase.o \
    $O/stack/phy/layer/LtePhyEnb.o \
    $O/stack/phy/layer/LtePhyEnbD2D.o \
    $O/stack/phy/layer/LtePhyUe.o \
    $O/stack/phy/layer/LtePhyUeD2D.o \
    $O/stack/phy/layer/NRPhyUe.o \
    $O/stack/phy/packet/LteAirFrame.o \
    $O/stack/phy/packet/LteFeedbackPkt.o \
    $O/stack/rlc/LteRlcMux.o \
    $O/stack/rlc/am/LteRlcAm.o \
    $O/stack/rlc/am/buffer/AmRxQueue.o \
    $O/stack/rlc/am/buffer/AmTxQueue.o \
    $O/stack/rlc/am/packet/LteRlcAmPdu.o \
    $O/stack/rlc/packet/LteRlcDataPdu.o \
    $O/stack/rlc/tm/LteRlcTm.o \
    $O/stack/rlc/um/LteRlcUm.o \
    $O/stack/rlc/um/LteRlcUmD2D.o \
    $O/stack/rlc/um/entity/UmRxEntity.o \
    $O/stack/rlc/um/entity/UmTxEntity.o \
    $O/world/radio/ChannelAccess.o \
    $O/world/radio/ChannelControl.o \
    $O/world/radio/LteChannelControl.o \
    $O/x2/LteX2Manager.o \
    $O/x2/X2AppClient.o \
    $O/x2/X2AppServer.o \
    $O/x2/packet/LteX2MsgSerializer.o \
    $O/apps/PingPong/packets/PingPongPacket_m.o \
    $O/apps/alert/AlertPacket_m.o \
    $O/apps/burst/BurstPacket_m.o \
    $O/apps/cbr/CbrPacket_m.o \
    $O/apps/d2dMultihop/MultihopD2DPacket_m.o \
    $O/apps/d2dMultihop/TrickleTimerMsg_m.o \
    $O/apps/mec/DeviceApp/DeviceAppMessages/DeviceAppPacket_m.o \
    $O/apps/mec/MecApps/packets/ProcessingTimeMessage_m.o \
    $O/apps/mec/MecRequestResponseApp/packets/RequestResponsePacket_m.o \
    $O/apps/mec/RealTimeVideoStreamingApp/packets/RTVideoStreamingPackets_m.o \
    $O/apps/mec/WarningAlert/packets/WarningAlertPacket_m.o \
    $O/apps/mecRequestResponseApp/packets/MecRequestResponsePacket_m.o \
    $O/apps/mecRequestResponseApp/packets/MigrationTimer_m.o \
    $O/apps/vod/M1Message_m.o \
    $O/apps/vod/VoDPacket_m.o \
    $O/apps/voip/VoipPacket_m.o \
    $O/common/LteCommon_m.o \
    $O/common/LteCommonEnum_m.o \
    $O/common/LteControlInfo_m.o \
    $O/common/timer/TMultiTimerMsg_m.o \
    $O/common/timer/TTimerMsg_m.o \
    $O/corenetwork/gtp/GtpUserMsg_m.o \
    $O/corenetwork/trafficFlowFilter/TftControlInfo_m.o \
    $O/nodes/mec/MECOrchestrator/MECOMessages/MECOrchestratorMessages_m.o \
    $O/nodes/mec/MECPlatform/MEAppPacket_m.o \
    $O/nodes/mec/MECPlatform/MECPackets_m.o \
    $O/nodes/mec/MECPlatform/MECServices/packets/AperiodicSubscriptionTimer_m.o \
    $O/nodes/mec/MECPlatform/MECServices/packets/HttpMessages_m.o \
    $O/nodes/mec/UALCMP/UALCMPMessages/UALCMPMessages_m.o \
    $O/stack/backgroundTrafficGenerator/ActiveUeNotification_m.o \
    $O/stack/backgroundTrafficGenerator/generators/RtxNotification_m.o \
    $O/stack/d2dModeSelection/D2DModeSwitchNotification_m.o \
    $O/stack/mac/packet/LteHarqFeedback_m.o \
    $O/stack/mac/packet/LteMacPdu_m.o \
    $O/stack/mac/packet/LteMacSduRequest_m.o \
    $O/stack/mac/packet/LteRac_m.o \
    $O/stack/mac/packet/LteSchedulingGrant_m.o \
    $O/stack/pdcp_rrc/packet/LtePdcpPdu_m.o \
    $O/stack/pdcp_rrc/packet/LteRohcPdu_m.o \
    $O/stack/phy/packet/AirFrame_m.o \
    $O/stack/phy/packet/LteAirFrame_m.o \
    $O/stack/phy/packet/LteFeedbackPkt_m.o \
    $O/stack/rlc/LteRlcDefs_m.o \
    $O/stack/rlc/am/packet/LteRlcAmPdu_m.o \
    $O/stack/rlc/am/packet/LteRlcAmSdu_m.o \
    $O/stack/rlc/packet/LteRlcDataPdu_m.o \
    $O/stack/rlc/packet/LteRlcPdu_m.o \
    $O/stack/rlc/packet/LteRlcSdu_m.o \
    $O/x2/packet/LteX2Message_m.o \
    $O/x2/packet/X2ControlInfo_m.o

# Message files
MSGFILES = \
    apps/PingPong/packets/PingPongPacket.msg \
    apps/alert/AlertPacket.msg \
    apps/burst/BurstPacket.msg \
    apps/cbr/CbrPacket.msg \
    apps/d2dMultihop/MultihopD2DPacket.msg \
    apps/d2dMultihop/TrickleTimerMsg.msg \
    apps/mec/DeviceApp/DeviceAppMessages/DeviceAppPacket.msg \
    apps/mec/MecApps/packets/ProcessingTimeMessage.msg \
    apps/mec/MecRequestResponseApp/packets/RequestResponsePacket.msg \
    apps/mec/RealTimeVideoStreamingApp/packets/RTVideoStreamingPackets.msg \
    apps/mec/WarningAlert/packets/WarningAlertPacket.msg \
    apps/mecRequestResponseApp/packets/MecRequestResponsePacket.msg \
    apps/mecRequestResponseApp/packets/MigrationTimer.msg \
    apps/vod/M1Message.msg \
    apps/vod/VoDPacket.msg \
    apps/voip/VoipPacket.msg \
    common/LteCommon.msg \
    common/LteCommonEnum.msg \
    common/LteControlInfo.msg \
    common/timer/TMultiTimerMsg.msg \
    common/timer/TTimerMsg.msg \
    corenetwork/gtp/GtpUserMsg.msg \
    corenetwork/trafficFlowFilter/TftControlInfo.msg \
    nodes/mec/MECOrchestrator/MECOMessages/MECOrchestratorMessages.msg \
    nodes/mec/MECPlatform/MEAppPacket.msg \
    nodes/mec/MECPlatform/MECPackets.msg \
    nodes/mec/MECPlatform/MECServices/packets/AperiodicSubscriptionTimer.msg \
    nodes/mec/MECPlatform/MECServices/packets/HttpMessages.msg \
    nodes/mec/UALCMP/UALCMPMessages/UALCMPMessages.msg \
    stack/backgroundTrafficGenerator/ActiveUeNotification.msg \
    stack/backgroundTrafficGenerator/generators/RtxNotification.msg \
    stack/d2dModeSelection/D2DModeSwitchNotification.msg \
    stack/mac/packet/LteHarqFeedback.msg \
    stack/mac/packet/LteMacPdu.msg \
    stack/mac/packet/LteMacSduRequest.msg \
    stack/mac/packet/LteRac.msg \
    stack/mac/packet/LteSchedulingGrant.msg \
    stack/pdcp_rrc/packet/LtePdcpPdu.msg \
    stack/pdcp_rrc/packet/LteRohcPdu.msg \
    stack/phy/packet/AirFrame.msg \
    stack/phy/packet/LteAirFrame.msg \
    stack/phy/packet/LteFeedbackPkt.msg \
    stack/rlc/LteRlcDefs.msg \
    stack/rlc/am/packet/LteRlcAmPdu.msg \
    stack/rlc/am/packet/LteRlcAmSdu.msg \
    stack/rlc/packet/LteRlcDataPdu.msg \
    stack/rlc/packet/LteRlcPdu.msg \
    stack/rlc/packet/LteRlcSdu.msg \
    x2/packet/LteX2Message.msg \
    x2/packet/X2ControlInfo.msg

# SM files
SMFILES =

# Other makefile variables (-K)
INET_PROJ=../../inet
VEINS_PROJ=../../veins
VEINS_INET_PROJ=../../veins_inet

#------------------------------------------------------------------------------

# Pull in OMNeT++ configuration (Makefile.inc)

ifneq ("$(OMNETPP_CONFIGFILE)","")
CONFIGFILE = $(OMNETPP_CONFIGFILE)
else
CONFIGFILE = $(shell opp_configfilepath)
endif

ifeq ("$(wildcard $(CONFIGFILE))","")
$(error Config file '$(CONFIGFILE)' does not exist -- add the OMNeT++ bin directory to the path so that opp_configfilepath can be found, or set the OMNETPP_CONFIGFILE variable to point to Makefile.inc)
endif

include $(CONFIGFILE)

# Simulation kernel and user interface libraries
OMNETPP_LIBS = -loppenvir$D $(KERNEL_LIBS) $(SYS_LIBS)
ifneq ($(PLATFORM),win32)
LIBS += -Wl,-rpath,$(abspath $(INET_PROJ)/src) -Wl,-rpath,$(abspath $(VEINS_PROJ)/src) -Wl,-rpath,$(abspath $(VEINS_INET_PROJ)/src)
endif

COPTS = $(CFLAGS) $(IMPORT_DEFINES) -DINET_IMPORT -DVEINS_IMPORT -DVEINS_INET_IMPORT $(INCLUDE_PATH) -I$(OMNETPP_INCL_DIR)
MSGCOPTS = $(INCLUDE_PATH)
SMCOPTS =

# we want to recompile everything if COPTS changes,
# so we store COPTS into $COPTS_FILE (if COPTS has changed since last build)
# and make the object files depend on it
COPTS_FILE = $O/.last-copts
ifneq ("$(COPTS)","$(shell cat $(COPTS_FILE) 2>/dev/null || echo '')")
  $(shell $(MKPATH) "$O")
  $(file >$(COPTS_FILE),$(COPTS))
endif

# On Windows, the target has additional file(s). An import lib and an optional debug symbol file is created too.
ifeq ($(PLATFORM),win32)
  TARGET_FILES+= $(TARGET_DIR)/$(TARGET_IMPLIB)
  LDFLAGS+= $(LDFLAG_IMPLIB)$O/$(TARGET_IMPLIB)
  ifeq ($(TOOLCHAIN_NAME),clang-msabi)
    ifeq ($(MODE),debug)
      TARGET_FILES+=$(TARGET_DIR)/$(TARGET_NAME).pdb
    endif
  else
    TARGET_FILES+= $(TARGET_DIR)/$(TARGET_IMPDEF)
    LDFLAGS+= $(LDFLAG_IMPDEF)$O/$(TARGET_IMPDEF)
  endif
endif

#------------------------------------------------------------------------------
# User-supplied makefile fragment(s)
-include makefrag

#------------------------------------------------------------------------------

# Main target
all: $(TARGET_FILES)

$(TARGET_DIR)/% :: $O/%
	@mkdir -p $(TARGET_DIR)
	$(Q)$(LN) $< $@
ifeq ($(TOOLCHAIN_NAME),clang-msabi)
	-$(Q)-$(LN) $(<:%.dll=%.lib) $(@:%.dll=%.lib) 2>/dev/null

$O/$(TARGET_NAME).pdb: $O/$(TARGET)
endif

$O/$(TARGET) $O/$(TARGET_IMPDEF) $O/$(TARGET_IMPLIB) &: $(OBJS)  $(wildcard $(EXTRA_OBJS)) Makefile $(CONFIGFILE)
	@$(MKPATH) $O
	@echo Creating shared library: $@
	$(Q)$(SHLIB_LD) -o $O/$(TARGET) $(OBJS) $(EXTRA_OBJS) $(AS_NEEDED_OFF) $(WHOLE_ARCHIVE_ON) $(LIBS) $(WHOLE_ARCHIVE_OFF) $(OMNETPP_LIBS) $(LDFLAGS)
	$(Q)$(SHLIB_POSTPROCESS) $O/$(TARGET)
ifeq ($(PLATFORM),win32)
	$(Q)llvm-ar d $O/$(TARGET_IMPLIB) $(TARGET) # WORKAROUND: throw away the first file from the archive to make the LLD generated import lib valid
endif

.PHONY: all clean cleanall depend msgheaders smheaders

# disabling all implicit rules
.SUFFIXES :

$O/%.o: %.cc $(COPTS_FILE) | msgheaders smheaders
	@$(MKPATH) $(dir $@)
	$(qecho) "$<"
	$(Q)$(CXX) -c $(CXXFLAGS) $(COPTS) -o $@ $<

%_m.cc %_m.h: %.msg
	$(qecho) MSGC: $<
	$(Q)$(MSGC) -s _m.cc -MD -MP -MF $O/$(basename $<)_m.h.d $(MSGCOPTS) $?

%_sm.cc %_sm.h: %.sm
	$(qecho) SMC: $<
	$(Q)$(SMC) -c++ -suffix cc $(SMCOPTS) $?

msgheaders: $(MSGFILES:.msg=_m.h)

smheaders: $(SMFILES:.sm=_sm.h)

clean:
	$(qecho) Cleaning $(TARGET)
	$(Q)-rm -rf $O
	$(Q)-rm -f $(TARGET_FILES)
	$(Q)-rm -f $(call opp_rwildcard, . , *_m.cc *_m.h *_sm.cc *_sm.h)

cleanall:
	$(Q)$(CLEANALL_COMMAND)
	$(Q)-rm -rf $(PROJECT_OUTPUT_DIR)

help:
	@echo "$$HELP_SYNOPSYS"
	@echo "$$HELP_TARGETS"
	@echo "$$HELP_VARIABLES"
	@echo "$$HELP_EXAMPLES"

# include all dependencies
-include $(OBJS:%=%.d) $(MSGFILES:%.msg=$O/%_m.h.d)
