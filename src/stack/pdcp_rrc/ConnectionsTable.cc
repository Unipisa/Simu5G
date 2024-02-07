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

#include "stack/pdcp_rrc/ConnectionsTable.h"

ConnectionsTable::ConnectionsTable()
{
    // Table is resetted by putting all fields equal to 0xFF
    memset(ht_, 0xFF, sizeof(struct entry_) * TABLE_SIZE);
    entries_.clear();
}
LogicalCid ConnectionsTable::find_entry(uint32_t srcAddr, uint32_t dstAddr,
        uint16_t srcPort, uint16_t dstPort, uint16_t dir, unsigned char msgCat) {

    for (auto & var : entries_) {
        if (var.srcAddr_ == srcAddr)
            if (var.dstAddr_ == dstAddr)
                if (var.dir_ == dir)
                    if (var.srcPort_ == srcPort)
                        if (var.dstPort_ == dstPort)
                            if (var.msgCat == msgCat)
                                return var.lcid_;
    }
    return 0xFFFF;
}
void ConnectionsTable::erase_entry(uint32_t nodeAddress) {

    //std::cout << "ConnectionsTable::erase_entry start at " << simTime().dbl() << std::endl;

    int position = -1;
    while (position == -1) {

        for (unsigned int i = 0; i < entries_.size(); i++) {
            if (entries_[i].dstAddr_ == nodeAddress
                    || entries_[i].srcAddr_ == nodeAddress) {
                position = i;
                break;
            }
        }

        if (position != -1) {
            entries_.erase(entries_.begin() + position);
            position = -1;
        } else {
            break;
        }
    }
    //std::cout << "ConnectionsTable::erase_entry end at " << simTime().dbl() << std::endl;
}

void ConnectionsTable::create_entry(uint32_t srcAddr, uint32_t dstAddr,
        uint16_t srcPort, uint16_t dstPort, uint16_t dir, LogicalCid lcid, unsigned char msgCat) {

    bool alreadyCreated = (find_entry(srcAddr, dstAddr, srcPort, dstPort, dir, msgCat) != 0xFFFF);


    if (alreadyCreated) {
        return;
    } else {
        entry_ tmp;
        tmp.srcAddr_ = srcAddr;
        tmp.dstAddr_ = dstAddr;
        tmp.srcPort_ = srcPort;
        tmp.dstPort_ = dstPort;
        tmp.lcid_ = lcid;
        tmp.msgCat = msgCat;
        tmp.dir_ = dir;
        entries_.push_back(tmp);
    }
}
unsigned int ConnectionsTable::hash_func(uint32_t srcAddr, uint32_t dstAddr, uint16_t typeOfService)
{
    return (typeOfService | srcAddr | dstAddr) % TABLE_SIZE;
}

unsigned int ConnectionsTable::hash_func(uint32_t srcAddr, uint32_t dstAddr, uint16_t typeOfService, uint16_t dir)
{
    return (typeOfService | srcAddr | dstAddr | dir) % TABLE_SIZE;
}

LogicalCid ConnectionsTable::find_entry(uint32_t srcAddr, uint32_t dstAddr, uint16_t typeOfService)
{
    int hashIndex = hash_func(srcAddr, dstAddr, typeOfService);
    while (1)
    {
        if (ht_[hashIndex].lcid_ == 0xFFFF)            // Entry not found
            return 0xFFFF;
        if (ht_[hashIndex].srcAddr_ == srcAddr &&
            ht_[hashIndex].dstAddr_ == dstAddr &&
            ht_[hashIndex].typeOfService_ == typeOfService)
            return ht_[hashIndex].lcid_;                // Entry found
        hashIndex = (hashIndex + 1) % TABLE_SIZE;    // Linear scanning of the hash table
    }
}

LogicalCid ConnectionsTable::find_entry(uint32_t srcAddr, uint32_t dstAddr, uint16_t typeOfService, uint16_t dir)
{
    int hashIndex = hash_func(srcAddr, dstAddr, typeOfService, dir);
    while (1)
    {
        if (ht_[hashIndex].lcid_ == 0xFFFF)            // Entry not found
            return 0xFFFF;
        if (ht_[hashIndex].srcAddr_ == srcAddr &&
            ht_[hashIndex].dstAddr_ == dstAddr &&
            ht_[hashIndex].typeOfService_ == typeOfService &&
            ht_[hashIndex].dir_ == dir)
            return ht_[hashIndex].lcid_;                // Entry found
        hashIndex = (hashIndex + 1) % TABLE_SIZE;    // Linear scanning of the hash table
    }
}

void ConnectionsTable::create_entry(uint32_t srcAddr, uint32_t dstAddr, uint16_t typeOfService, LogicalCid lcid)
{
    int hashIndex = hash_func(srcAddr, dstAddr, typeOfService);
    while (ht_[hashIndex].lcid_ != 0xFFFF)
        hashIndex = (hashIndex + 1) % TABLE_SIZE;    // Linear scanning of the hash table
    ht_[hashIndex].srcAddr_ = srcAddr;
    ht_[hashIndex].dstAddr_ = dstAddr;
    ht_[hashIndex].typeOfService_ = typeOfService;
    ht_[hashIndex].lcid_ = lcid;
    return;
}

void ConnectionsTable::create_entry(uint32_t srcAddr, uint32_t dstAddr, uint16_t typeOfService, uint16_t dir, LogicalCid lcid)
{
    int hashIndex = hash_func(srcAddr, dstAddr, typeOfService, dir);
    while (ht_[hashIndex].lcid_ != 0xFFFF)
        hashIndex = (hashIndex + 1) % TABLE_SIZE;    // Linear scanning of the hash table
    ht_[hashIndex].srcAddr_ = srcAddr;
    ht_[hashIndex].dstAddr_ = dstAddr;
    ht_[hashIndex].typeOfService_ = typeOfService;
    ht_[hashIndex].dir_ = dir;
    ht_[hashIndex].lcid_ = lcid;
    return;
}

ConnectionsTable::~ConnectionsTable()
{
    memset(ht_, 0xFF, sizeof(struct entry_) * TABLE_SIZE);
    entries_.clear();
}
