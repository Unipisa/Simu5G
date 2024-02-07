#ifndef _LTE_NRCOMMON_H_
#define _LTE_NRCOMMON_H_

#define _NO_W32_PSEUDO_MODIFIERS

#include <iostream>
#include <omnetpp.h>
#include <string>
#include <fstream>
#include <vector>
#include <bitset>
#include <set>
#include <queue>
#include <map>
#include <list>
#include <algorithm>

enum ResourceType {
    GBR, NGBR, DCGBR, UNKNOWN_RES
//GBR -> Guaranteed Bitrate, NGBR -> NonGBR, DCGBR -> Delay-CriticalGBR
};
// QosCharacteristics to which a 5QI refers to
class QosCharacteristic {
public:
    QosCharacteristic() {
    }
    QosCharacteristic(ResourceType resType, unsigned short priorityLevel, double PDB, double PER, uint16_t DMDBV, uint16_t defAveragingWindow) :
            resType(resType), priorityLevel(priorityLevel), PDB(PDB), PER(PER), DMDBV(DMDBV), defAveragingWindow(defAveragingWindow) {
        ASSERT(priorityLevel >= 0 && priorityLevel <= 127);
        ASSERT(DMDBV >= 0 && DMDBV <= 4095);
        ASSERT(defAveragingWindow >= 0 && defAveragingWindow <= 4095);
    }

    ResourceType resType; //GBR,nonGBR,DC-GBR
    unsigned short priorityLevel; // TS 38.413, 9.3.1.84, 1...127
    double PDB; //Packet Delay Budget, in milliseconds,
    double PER; //Packet Error Rate
    uint16_t DMDBV; //Default Maximum Data Burst Volume, in Bytes, 0...4095, 38.413, 9.3.1.83
    uint16_t defAveragingWindow; // in milliseconds, 38.413, 9.3.1.82, 0...4095

    unsigned short getPriorityLevel(){
        return priorityLevel;
    }

    double getPdb(){
        return PDB;
    }

    double getPer(){
        return PER;
    }
};

ResourceType convertStringToResourceType(std::string type);

//QFI --> defined in 23.501, Chapter 5.7
typedef unsigned short QFI;

//Value for 5QI --> TS 23.501, Chapter 5.7
typedef unsigned short QI;

//TS 38.413, 9.3.1.4, unit --> bit/s
typedef unsigned int bitrate;

// see in TS 23.501 ->  Chapter 5.7, Table 5.7.4-1
class NRQosCharacteristics {
public:
    static NRQosCharacteristics* getNRQosCharacteristics() {
        if (instance == nullptr) {
            instance = new NRQosCharacteristics();
        }
        return instance;
    }
    ~NRQosCharacteristics() {
        values.clear();
        instance = nullptr;
    }
    std::map<QI, QosCharacteristic>& getValues() {
        return values;
    }
    QosCharacteristic& getQosCharacteristic(unsigned short value5qi) {
        return values[value5qi];
    }
private:
    static NRQosCharacteristics *instance;
    NRQosCharacteristics() {
    }

    //QosCharacteristics from 23.501 V16.0.2, Table 5.7.4-1, map is filled with values in initialization method in NRBinder
    std::map<QI, QosCharacteristic> values;
};
//QosParameters
class NRQosParameters {
public:
    NRQosParameters() {
    }
    NRQosParameters(QI qi, unsigned short arp, bool reflectiveQosAttribute, bool notificationControl, bitrate MFBR_UL, bitrate MFBR_DL, bitrate GFBR_DL, bitrate GFBR_UL, bitrate sessionAMBR,
            bitrate ueAMBR, unsigned int MPLR_DL, unsigned int MPLR_UL) {
        qosCharacteristics = NRQosCharacteristics::getNRQosCharacteristics()->getQosCharacteristic(qi);
        this->arp = arp;
        this->reflectiveQosAttribute = reflectiveQosAttribute;
        this->notificationControl = notificationControl;
        this->MFBR_UL = MFBR_UL;
        this->GFBR_DL = GFBR_DL;
        this->MFBR_DL = MFBR_DL;
        this->GFBR_UL = GFBR_UL;
        this->MPLR_DL = MPLR_DL;
        this->MPLR_UL = MPLR_UL;
        this->sessionAMBR = sessionAMBR;
        this->ueAMBR = ueAMBR;
        ASSERT(arp >= 1 && arp <= 15);
    }
    QosCharacteristic qosCharacteristics;
    uint16_t arp; //1...15
    bool reflectiveQosAttribute; //optional, non-GBR
    bool notificationControl; //GBR
    bitrate MFBR_UL; //maximum flow bit rate, GBR / DC-GBR only
    bitrate MFBR_DL;
    bitrate GFBR_UL; // guaranteed flow bit rate
    bitrate GFBR_DL;
    bitrate sessionAMBR;
    bitrate ueAMBR;
    uint32_t MPLR_DL; //Maximum Packet Loss Rate
    uint32_t MPLR_UL;
    // maximum packet loss rate
};

//QosProfile --> TS 23.501
class NRQosProfile {
public:
    NRQosProfile(QI qi, unsigned short arp, bool reflectiveQosAttribute, bool notificationControl, bitrate MFBR_UL, bitrate MFBR_DL, bitrate GFBR_DL, bitrate GFBR_UL, bitrate sessionAMBR,
            bitrate ueAMBR, unsigned int MPLR_DL, unsigned int MPLR_UL) {
        this->qosParam = new NRQosParameters(qi, arp, reflectiveQosAttribute, notificationControl, MFBR_UL, MFBR_DL, GFBR_DL, GFBR_UL, sessionAMBR, ueAMBR, MPLR_DL, MPLR_UL);
    }

    virtual ~NRQosProfile() {
    }

    NRQosParameters* getQosParameters() {
        return qosParam;
    }
protected:
    NRQosParameters *qosParam;
};

#endif
