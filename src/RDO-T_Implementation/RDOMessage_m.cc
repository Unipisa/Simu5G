//
// Generated file, do not edit! Created by opp_msgtool 6.0 from rdoexperiment/src/RDOMessage.msg.
//

// Disable warnings about unused variables, empty switch stmts, etc:
#ifdef _MSC_VER
#  pragma warning(disable:4101)
#  pragma warning(disable:4065)
#endif

#if defined(__clang__)
#  pragma clang diagnostic ignored "-Wshadow"
#  pragma clang diagnostic ignored "-Wconversion"
#  pragma clang diagnostic ignored "-Wunused-parameter"
#  pragma clang diagnostic ignored "-Wc++98-compat"
#  pragma clang diagnostic ignored "-Wunreachable-code-break"
#  pragma clang diagnostic ignored "-Wold-style-cast"
#elif defined(__GNUC__)
#  pragma GCC diagnostic ignored "-Wshadow"
#  pragma GCC diagnostic ignored "-Wconversion"
#  pragma GCC diagnostic ignored "-Wunused-parameter"
#  pragma GCC diagnostic ignored "-Wold-style-cast"
#  pragma GCC diagnostic ignored "-Wsuggest-attribute=noreturn"
#  pragma GCC diagnostic ignored "-Wfloat-conversion"
#endif

#include <iostream>
#include <sstream>
#include <memory>
#include <type_traits>
#include "RDOMessage_m.h"

namespace omnetpp {

// Template pack/unpack rules. They are declared *after* a1l type-specific pack functions for multiple reasons.
// They are in the omnetpp namespace, to allow them to be found by argument-dependent lookup via the cCommBuffer argument

// Packing/unpacking an std::vector
template<typename T, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::vector<T,A>& v)
{
    int n = v.size();
    doParsimPacking(buffer, n);
    for (int i = 0; i < n; i++)
        doParsimPacking(buffer, v[i]);
}

template<typename T, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::vector<T,A>& v)
{
    int n;
    doParsimUnpacking(buffer, n);
    v.resize(n);
    for (int i = 0; i < n; i++)
        doParsimUnpacking(buffer, v[i]);
}

// Packing/unpacking an std::list
template<typename T, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::list<T,A>& l)
{
    doParsimPacking(buffer, (int)l.size());
    for (typename std::list<T,A>::const_iterator it = l.begin(); it != l.end(); ++it)
        doParsimPacking(buffer, (T&)*it);
}

template<typename T, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::list<T,A>& l)
{
    int n;
    doParsimUnpacking(buffer, n);
    for (int i = 0; i < n; i++) {
        l.push_back(T());
        doParsimUnpacking(buffer, l.back());
    }
}

// Packing/unpacking an std::set
template<typename T, typename Tr, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::set<T,Tr,A>& s)
{
    doParsimPacking(buffer, (int)s.size());
    for (typename std::set<T,Tr,A>::const_iterator it = s.begin(); it != s.end(); ++it)
        doParsimPacking(buffer, *it);
}

template<typename T, typename Tr, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::set<T,Tr,A>& s)
{
    int n;
    doParsimUnpacking(buffer, n);
    for (int i = 0; i < n; i++) {
        T x;
        doParsimUnpacking(buffer, x);
        s.insert(x);
    }
}

// Packing/unpacking an std::map
template<typename K, typename V, typename Tr, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::map<K,V,Tr,A>& m)
{
    doParsimPacking(buffer, (int)m.size());
    for (typename std::map<K,V,Tr,A>::const_iterator it = m.begin(); it != m.end(); ++it) {
        doParsimPacking(buffer, it->first);
        doParsimPacking(buffer, it->second);
    }
}

template<typename K, typename V, typename Tr, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::map<K,V,Tr,A>& m)
{
    int n;
    doParsimUnpacking(buffer, n);
    for (int i = 0; i < n; i++) {
        K k; V v;
        doParsimUnpacking(buffer, k);
        doParsimUnpacking(buffer, v);
        m[k] = v;
    }
}

// Default pack/unpack function for arrays
template<typename T>
void doParsimArrayPacking(omnetpp::cCommBuffer *b, const T *t, int n)
{
    for (int i = 0; i < n; i++)
        doParsimPacking(b, t[i]);
}

template<typename T>
void doParsimArrayUnpacking(omnetpp::cCommBuffer *b, T *t, int n)
{
    for (int i = 0; i < n; i++)
        doParsimUnpacking(b, t[i]);
}

// Default rule to prevent compiler from choosing base class' doParsimPacking() function
template<typename T>
void doParsimPacking(omnetpp::cCommBuffer *, const T& t)
{
    throw omnetpp::cRuntimeError("Parsim error: No doParsimPacking() function for type %s", omnetpp::opp_typename(typeid(t)));
}

template<typename T>
void doParsimUnpacking(omnetpp::cCommBuffer *, T& t)
{
    throw omnetpp::cRuntimeError("Parsim error: No doParsimUnpacking() function for type %s", omnetpp::opp_typename(typeid(t)));
}

}  // namespace omnetpp

namespace inet {

Register_Class(RDOPacket)

RDOPacket::RDOPacket(const char *name, short kind) : ::omnetpp::cPacket(name, kind)
{
}

RDOPacket::RDOPacket(const RDOPacket& other) : ::omnetpp::cPacket(other)
{
    copy(other);
}

RDOPacket::~RDOPacket()
{
}

RDOPacket& RDOPacket::operator=(const RDOPacket& other)
{
    if (this == &other) return *this;
    ::omnetpp::cPacket::operator=(other);
    copy(other);
    return *this;
}

void RDOPacket::copy(const RDOPacket& other)
{
    this->sourceId = other.sourceId;
    this->destinationId = other.destinationId;
    this->sequenceNumber = other.sequenceNumber;
    this->timestamp = other.timestamp;
}

void RDOPacket::parsimPack(omnetpp::cCommBuffer *b) const
{
    ::omnetpp::cPacket::parsimPack(b);
    doParsimPacking(b,this->sourceId);
    doParsimPacking(b,this->destinationId);
    doParsimPacking(b,this->sequenceNumber);
    doParsimPacking(b,this->timestamp);
}

void RDOPacket::parsimUnpack(omnetpp::cCommBuffer *b)
{
    ::omnetpp::cPacket::parsimUnpack(b);
    doParsimUnpacking(b,this->sourceId);
    doParsimUnpacking(b,this->destinationId);
    doParsimUnpacking(b,this->sequenceNumber);
    doParsimUnpacking(b,this->timestamp);
}

int RDOPacket::getSourceId() const
{
    return this->sourceId;
}

void RDOPacket::setSourceId(int sourceId)
{
    this->sourceId = sourceId;
}

int RDOPacket::getDestinationId() const
{
    return this->destinationId;
}

void RDOPacket::setDestinationId(int destinationId)
{
    this->destinationId = destinationId;
}

int RDOPacket::getSequenceNumber() const
{
    return this->sequenceNumber;
}

void RDOPacket::setSequenceNumber(int sequenceNumber)
{
    this->sequenceNumber = sequenceNumber;
}

::omnetpp::simtime_t RDOPacket::getTimestamp() const
{
    return this->timestamp;
}

void RDOPacket::setTimestamp(::omnetpp::simtime_t timestamp)
{
    this->timestamp = timestamp;
}

class RDOPacketDescriptor : public omnetpp::cClassDescriptor
{
  private:
    mutable const char **propertyNames;
    enum FieldConstants {
        FIELD_sourceId,
        FIELD_destinationId,
        FIELD_sequenceNumber,
        FIELD_timestamp,
    };
  public:
    RDOPacketDescriptor();
    virtual ~RDOPacketDescriptor();

    virtual bool doesSupport(omnetpp::cObject *obj) const override;
    virtual const char **getPropertyNames() const override;
    virtual const char *getProperty(const char *propertyName) const override;
    virtual int getFieldCount() const override;
    virtual const char *getFieldName(int field) const override;
    virtual int findField(const char *fieldName) const override;
    virtual unsigned int getFieldTypeFlags(int field) const override;
    virtual const char *getFieldTypeString(int field) const override;
    virtual const char **getFieldPropertyNames(int field) const override;
    virtual const char *getFieldProperty(int field, const char *propertyName) const override;
    virtual int getFieldArraySize(omnetpp::any_ptr object, int field) const override;
    virtual void setFieldArraySize(omnetpp::any_ptr object, int field, int size) const override;

    virtual const char *getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const override;
    virtual std::string getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const override;
    virtual omnetpp::cValue getFieldValue(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const override;

    virtual const char *getFieldStructName(int field) const override;
    virtual omnetpp::any_ptr getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const override;
};

Register_ClassDescriptor(RDOPacketDescriptor)

RDOPacketDescriptor::RDOPacketDescriptor() : omnetpp::cClassDescriptor(omnetpp::opp_typename(typeid(inet::RDOPacket)), "omnetpp::cPacket")
{
    propertyNames = nullptr;
}

RDOPacketDescriptor::~RDOPacketDescriptor()
{
    delete[] propertyNames;
}

bool RDOPacketDescriptor::doesSupport(omnetpp::cObject *obj) const
{
    return dynamic_cast<RDOPacket *>(obj)!=nullptr;
}

const char **RDOPacketDescriptor::getPropertyNames() const
{
    if (!propertyNames) {
        static const char *names[] = {  nullptr };
        omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
        const char **baseNames = base ? base->getPropertyNames() : nullptr;
        propertyNames = mergeLists(baseNames, names);
    }
    return propertyNames;
}

const char *RDOPacketDescriptor::getProperty(const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? base->getProperty(propertyName) : nullptr;
}

int RDOPacketDescriptor::getFieldCount() const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? 4+base->getFieldCount() : 4;
}

unsigned int RDOPacketDescriptor::getFieldTypeFlags(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeFlags(field);
        field -= base->getFieldCount();
    }
    static unsigned int fieldTypeFlags[] = {
        FD_ISEDITABLE,    // FIELD_sourceId
        FD_ISEDITABLE,    // FIELD_destinationId
        FD_ISEDITABLE,    // FIELD_sequenceNumber
        FD_ISEDITABLE,    // FIELD_timestamp
    };
    return (field >= 0 && field < 4) ? fieldTypeFlags[field] : 0;
}

const char *RDOPacketDescriptor::getFieldName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldName(field);
        field -= base->getFieldCount();
    }
    static const char *fieldNames[] = {
        "sourceId",
        "destinationId",
        "sequenceNumber",
        "timestamp",
    };
    return (field >= 0 && field < 4) ? fieldNames[field] : nullptr;
}

int RDOPacketDescriptor::findField(const char *fieldName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    int baseIndex = base ? base->getFieldCount() : 0;
    if (strcmp(fieldName, "sourceId") == 0) return baseIndex + 0;
    if (strcmp(fieldName, "destinationId") == 0) return baseIndex + 1;
    if (strcmp(fieldName, "sequenceNumber") == 0) return baseIndex + 2;
    if (strcmp(fieldName, "timestamp") == 0) return baseIndex + 3;
    return base ? base->findField(fieldName) : -1;
}

const char *RDOPacketDescriptor::getFieldTypeString(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeString(field);
        field -= base->getFieldCount();
    }
    static const char *fieldTypeStrings[] = {
        "int",    // FIELD_sourceId
        "int",    // FIELD_destinationId
        "int",    // FIELD_sequenceNumber
        "omnetpp::simtime_t",    // FIELD_timestamp
    };
    return (field >= 0 && field < 4) ? fieldTypeStrings[field] : nullptr;
}

const char **RDOPacketDescriptor::getFieldPropertyNames(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldPropertyNames(field);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    }
}

const char *RDOPacketDescriptor::getFieldProperty(int field, const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldProperty(field, propertyName);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    }
}

int RDOPacketDescriptor::getFieldArraySize(omnetpp::any_ptr object, int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldArraySize(object, field);
        field -= base->getFieldCount();
    }
    RDOPacket *pp = omnetpp::fromAnyPtr<RDOPacket>(object); (void)pp;
    switch (field) {
        default: return 0;
    }
}

void RDOPacketDescriptor::setFieldArraySize(omnetpp::any_ptr object, int field, int size) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldArraySize(object, field, size);
            return;
        }
        field -= base->getFieldCount();
    }
    RDOPacket *pp = omnetpp::fromAnyPtr<RDOPacket>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set array size of field %d of class 'RDOPacket'", field);
    }
}

const char *RDOPacketDescriptor::getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldDynamicTypeString(object,field,i);
        field -= base->getFieldCount();
    }
    RDOPacket *pp = omnetpp::fromAnyPtr<RDOPacket>(object); (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

std::string RDOPacketDescriptor::getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValueAsString(object,field,i);
        field -= base->getFieldCount();
    }
    RDOPacket *pp = omnetpp::fromAnyPtr<RDOPacket>(object); (void)pp;
    switch (field) {
        case FIELD_sourceId: return long2string(pp->getSourceId());
        case FIELD_destinationId: return long2string(pp->getDestinationId());
        case FIELD_sequenceNumber: return long2string(pp->getSequenceNumber());
        case FIELD_timestamp: return simtime2string(pp->getTimestamp());
        default: return "";
    }
}

void RDOPacketDescriptor::setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValueAsString(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    RDOPacket *pp = omnetpp::fromAnyPtr<RDOPacket>(object); (void)pp;
    switch (field) {
        case FIELD_sourceId: pp->setSourceId(string2long(value)); break;
        case FIELD_destinationId: pp->setDestinationId(string2long(value)); break;
        case FIELD_sequenceNumber: pp->setSequenceNumber(string2long(value)); break;
        case FIELD_timestamp: pp->setTimestamp(string2simtime(value)); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'RDOPacket'", field);
    }
}

omnetpp::cValue RDOPacketDescriptor::getFieldValue(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValue(object,field,i);
        field -= base->getFieldCount();
    }
    RDOPacket *pp = omnetpp::fromAnyPtr<RDOPacket>(object); (void)pp;
    switch (field) {
        case FIELD_sourceId: return pp->getSourceId();
        case FIELD_destinationId: return pp->getDestinationId();
        case FIELD_sequenceNumber: return pp->getSequenceNumber();
        case FIELD_timestamp: return pp->getTimestamp().dbl();
        default: throw omnetpp::cRuntimeError("Cannot return field %d of class 'RDOPacket' as cValue -- field index out of range?", field);
    }
}

void RDOPacketDescriptor::setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValue(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    RDOPacket *pp = omnetpp::fromAnyPtr<RDOPacket>(object); (void)pp;
    switch (field) {
        case FIELD_sourceId: pp->setSourceId(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_destinationId: pp->setDestinationId(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_sequenceNumber: pp->setSequenceNumber(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_timestamp: pp->setTimestamp(value.doubleValue()); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'RDOPacket'", field);
    }
}

const char *RDOPacketDescriptor::getFieldStructName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructName(field);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    };
}

omnetpp::any_ptr RDOPacketDescriptor::getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructValuePointer(object, field, i);
        field -= base->getFieldCount();
    }
    RDOPacket *pp = omnetpp::fromAnyPtr<RDOPacket>(object); (void)pp;
    switch (field) {
        default: return omnetpp::any_ptr(nullptr);
    }
}

void RDOPacketDescriptor::setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldStructValuePointer(object, field, i, ptr);
            return;
        }
        field -= base->getFieldCount();
    }
    RDOPacket *pp = omnetpp::fromAnyPtr<RDOPacket>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'RDOPacket'", field);
    }
}

Register_Class(RDOStatePacket)

RDOStatePacket::RDOStatePacket(const char *name, short kind) : ::inet::RDOPacket(name, kind)
{
}

RDOStatePacket::RDOStatePacket(const RDOStatePacket& other) : ::inet::RDOPacket(other)
{
    copy(other);
}

RDOStatePacket::~RDOStatePacket()
{
}

RDOStatePacket& RDOStatePacket::operator=(const RDOStatePacket& other)
{
    if (this == &other) return *this;
    ::inet::RDOPacket::operator=(other);
    copy(other);
    return *this;
}

void RDOStatePacket::copy(const RDOStatePacket& other)
{
    this->stateValue = other.stateValue;
}

void RDOStatePacket::parsimPack(omnetpp::cCommBuffer *b) const
{
    ::inet::RDOPacket::parsimPack(b);
    doParsimPacking(b,this->stateValue);
}

void RDOStatePacket::parsimUnpack(omnetpp::cCommBuffer *b)
{
    ::inet::RDOPacket::parsimUnpack(b);
    doParsimUnpacking(b,this->stateValue);
}

double RDOStatePacket::getStateValue() const
{
    return this->stateValue;
}

void RDOStatePacket::setStateValue(double stateValue)
{
    this->stateValue = stateValue;
}

class RDOStatePacketDescriptor : public omnetpp::cClassDescriptor
{
  private:
    mutable const char **propertyNames;
    enum FieldConstants {
        FIELD_stateValue,
    };
  public:
    RDOStatePacketDescriptor();
    virtual ~RDOStatePacketDescriptor();

    virtual bool doesSupport(omnetpp::cObject *obj) const override;
    virtual const char **getPropertyNames() const override;
    virtual const char *getProperty(const char *propertyName) const override;
    virtual int getFieldCount() const override;
    virtual const char *getFieldName(int field) const override;
    virtual int findField(const char *fieldName) const override;
    virtual unsigned int getFieldTypeFlags(int field) const override;
    virtual const char *getFieldTypeString(int field) const override;
    virtual const char **getFieldPropertyNames(int field) const override;
    virtual const char *getFieldProperty(int field, const char *propertyName) const override;
    virtual int getFieldArraySize(omnetpp::any_ptr object, int field) const override;
    virtual void setFieldArraySize(omnetpp::any_ptr object, int field, int size) const override;

    virtual const char *getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const override;
    virtual std::string getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const override;
    virtual omnetpp::cValue getFieldValue(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const override;

    virtual const char *getFieldStructName(int field) const override;
    virtual omnetpp::any_ptr getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const override;
};

Register_ClassDescriptor(RDOStatePacketDescriptor)

RDOStatePacketDescriptor::RDOStatePacketDescriptor() : omnetpp::cClassDescriptor(omnetpp::opp_typename(typeid(inet::RDOStatePacket)), "inet::RDOPacket")
{
    propertyNames = nullptr;
}

RDOStatePacketDescriptor::~RDOStatePacketDescriptor()
{
    delete[] propertyNames;
}

bool RDOStatePacketDescriptor::doesSupport(omnetpp::cObject *obj) const
{
    return dynamic_cast<RDOStatePacket *>(obj)!=nullptr;
}

const char **RDOStatePacketDescriptor::getPropertyNames() const
{
    if (!propertyNames) {
        static const char *names[] = {  nullptr };
        omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
        const char **baseNames = base ? base->getPropertyNames() : nullptr;
        propertyNames = mergeLists(baseNames, names);
    }
    return propertyNames;
}

const char *RDOStatePacketDescriptor::getProperty(const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? base->getProperty(propertyName) : nullptr;
}

int RDOStatePacketDescriptor::getFieldCount() const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? 1+base->getFieldCount() : 1;
}

unsigned int RDOStatePacketDescriptor::getFieldTypeFlags(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeFlags(field);
        field -= base->getFieldCount();
    }
    static unsigned int fieldTypeFlags[] = {
        FD_ISEDITABLE,    // FIELD_stateValue
    };
    return (field >= 0 && field < 1) ? fieldTypeFlags[field] : 0;
}

const char *RDOStatePacketDescriptor::getFieldName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldName(field);
        field -= base->getFieldCount();
    }
    static const char *fieldNames[] = {
        "stateValue",
    };
    return (field >= 0 && field < 1) ? fieldNames[field] : nullptr;
}

int RDOStatePacketDescriptor::findField(const char *fieldName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    int baseIndex = base ? base->getFieldCount() : 0;
    if (strcmp(fieldName, "stateValue") == 0) return baseIndex + 0;
    return base ? base->findField(fieldName) : -1;
}

const char *RDOStatePacketDescriptor::getFieldTypeString(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeString(field);
        field -= base->getFieldCount();
    }
    static const char *fieldTypeStrings[] = {
        "double",    // FIELD_stateValue
    };
    return (field >= 0 && field < 1) ? fieldTypeStrings[field] : nullptr;
}

const char **RDOStatePacketDescriptor::getFieldPropertyNames(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldPropertyNames(field);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    }
}

const char *RDOStatePacketDescriptor::getFieldProperty(int field, const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldProperty(field, propertyName);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    }
}

int RDOStatePacketDescriptor::getFieldArraySize(omnetpp::any_ptr object, int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldArraySize(object, field);
        field -= base->getFieldCount();
    }
    RDOStatePacket *pp = omnetpp::fromAnyPtr<RDOStatePacket>(object); (void)pp;
    switch (field) {
        default: return 0;
    }
}

void RDOStatePacketDescriptor::setFieldArraySize(omnetpp::any_ptr object, int field, int size) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldArraySize(object, field, size);
            return;
        }
        field -= base->getFieldCount();
    }
    RDOStatePacket *pp = omnetpp::fromAnyPtr<RDOStatePacket>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set array size of field %d of class 'RDOStatePacket'", field);
    }
}

const char *RDOStatePacketDescriptor::getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldDynamicTypeString(object,field,i);
        field -= base->getFieldCount();
    }
    RDOStatePacket *pp = omnetpp::fromAnyPtr<RDOStatePacket>(object); (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

std::string RDOStatePacketDescriptor::getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValueAsString(object,field,i);
        field -= base->getFieldCount();
    }
    RDOStatePacket *pp = omnetpp::fromAnyPtr<RDOStatePacket>(object); (void)pp;
    switch (field) {
        case FIELD_stateValue: return double2string(pp->getStateValue());
        default: return "";
    }
}

void RDOStatePacketDescriptor::setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValueAsString(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    RDOStatePacket *pp = omnetpp::fromAnyPtr<RDOStatePacket>(object); (void)pp;
    switch (field) {
        case FIELD_stateValue: pp->setStateValue(string2double(value)); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'RDOStatePacket'", field);
    }
}

omnetpp::cValue RDOStatePacketDescriptor::getFieldValue(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValue(object,field,i);
        field -= base->getFieldCount();
    }
    RDOStatePacket *pp = omnetpp::fromAnyPtr<RDOStatePacket>(object); (void)pp;
    switch (field) {
        case FIELD_stateValue: return pp->getStateValue();
        default: throw omnetpp::cRuntimeError("Cannot return field %d of class 'RDOStatePacket' as cValue -- field index out of range?", field);
    }
}

void RDOStatePacketDescriptor::setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValue(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    RDOStatePacket *pp = omnetpp::fromAnyPtr<RDOStatePacket>(object); (void)pp;
    switch (field) {
        case FIELD_stateValue: pp->setStateValue(value.doubleValue()); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'RDOStatePacket'", field);
    }
}

const char *RDOStatePacketDescriptor::getFieldStructName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructName(field);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    };
}

omnetpp::any_ptr RDOStatePacketDescriptor::getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructValuePointer(object, field, i);
        field -= base->getFieldCount();
    }
    RDOStatePacket *pp = omnetpp::fromAnyPtr<RDOStatePacket>(object); (void)pp;
    switch (field) {
        default: return omnetpp::any_ptr(nullptr);
    }
}

void RDOStatePacketDescriptor::setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldStructValuePointer(object, field, i, ptr);
            return;
        }
        field -= base->getFieldCount();
    }
    RDOStatePacket *pp = omnetpp::fromAnyPtr<RDOStatePacket>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'RDOStatePacket'", field);
    }
}

Register_Class(RDOGradientPacket)

RDOGradientPacket::RDOGradientPacket(const char *name, short kind) : ::inet::RDOPacket(name, kind)
{
}

RDOGradientPacket::RDOGradientPacket(const RDOGradientPacket& other) : ::inet::RDOPacket(other)
{
    copy(other);
}

RDOGradientPacket::~RDOGradientPacket()
{
}

RDOGradientPacket& RDOGradientPacket::operator=(const RDOGradientPacket& other)
{
    if (this == &other) return *this;
    ::inet::RDOPacket::operator=(other);
    copy(other);
    return *this;
}

void RDOGradientPacket::copy(const RDOGradientPacket& other)
{
    this->gradientValue = other.gradientValue;
}

void RDOGradientPacket::parsimPack(omnetpp::cCommBuffer *b) const
{
    ::inet::RDOPacket::parsimPack(b);
    doParsimPacking(b,this->gradientValue);
}

void RDOGradientPacket::parsimUnpack(omnetpp::cCommBuffer *b)
{
    ::inet::RDOPacket::parsimUnpack(b);
    doParsimUnpacking(b,this->gradientValue);
}

double RDOGradientPacket::getGradientValue() const
{
    return this->gradientValue;
}

void RDOGradientPacket::setGradientValue(double gradientValue)
{
    this->gradientValue = gradientValue;
}

class RDOGradientPacketDescriptor : public omnetpp::cClassDescriptor
{
  private:
    mutable const char **propertyNames;
    enum FieldConstants {
        FIELD_gradientValue,
    };
  public:
    RDOGradientPacketDescriptor();
    virtual ~RDOGradientPacketDescriptor();

    virtual bool doesSupport(omnetpp::cObject *obj) const override;
    virtual const char **getPropertyNames() const override;
    virtual const char *getProperty(const char *propertyName) const override;
    virtual int getFieldCount() const override;
    virtual const char *getFieldName(int field) const override;
    virtual int findField(const char *fieldName) const override;
    virtual unsigned int getFieldTypeFlags(int field) const override;
    virtual const char *getFieldTypeString(int field) const override;
    virtual const char **getFieldPropertyNames(int field) const override;
    virtual const char *getFieldProperty(int field, const char *propertyName) const override;
    virtual int getFieldArraySize(omnetpp::any_ptr object, int field) const override;
    virtual void setFieldArraySize(omnetpp::any_ptr object, int field, int size) const override;

    virtual const char *getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const override;
    virtual std::string getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const override;
    virtual omnetpp::cValue getFieldValue(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const override;

    virtual const char *getFieldStructName(int field) const override;
    virtual omnetpp::any_ptr getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const override;
};

Register_ClassDescriptor(RDOGradientPacketDescriptor)

RDOGradientPacketDescriptor::RDOGradientPacketDescriptor() : omnetpp::cClassDescriptor(omnetpp::opp_typename(typeid(inet::RDOGradientPacket)), "inet::RDOPacket")
{
    propertyNames = nullptr;
}

RDOGradientPacketDescriptor::~RDOGradientPacketDescriptor()
{
    delete[] propertyNames;
}

bool RDOGradientPacketDescriptor::doesSupport(omnetpp::cObject *obj) const
{
    return dynamic_cast<RDOGradientPacket *>(obj)!=nullptr;
}

const char **RDOGradientPacketDescriptor::getPropertyNames() const
{
    if (!propertyNames) {
        static const char *names[] = {  nullptr };
        omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
        const char **baseNames = base ? base->getPropertyNames() : nullptr;
        propertyNames = mergeLists(baseNames, names);
    }
    return propertyNames;
}

const char *RDOGradientPacketDescriptor::getProperty(const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? base->getProperty(propertyName) : nullptr;
}

int RDOGradientPacketDescriptor::getFieldCount() const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? 1+base->getFieldCount() : 1;
}

unsigned int RDOGradientPacketDescriptor::getFieldTypeFlags(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeFlags(field);
        field -= base->getFieldCount();
    }
    static unsigned int fieldTypeFlags[] = {
        FD_ISEDITABLE,    // FIELD_gradientValue
    };
    return (field >= 0 && field < 1) ? fieldTypeFlags[field] : 0;
}

const char *RDOGradientPacketDescriptor::getFieldName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldName(field);
        field -= base->getFieldCount();
    }
    static const char *fieldNames[] = {
        "gradientValue",
    };
    return (field >= 0 && field < 1) ? fieldNames[field] : nullptr;
}

int RDOGradientPacketDescriptor::findField(const char *fieldName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    int baseIndex = base ? base->getFieldCount() : 0;
    if (strcmp(fieldName, "gradientValue") == 0) return baseIndex + 0;
    return base ? base->findField(fieldName) : -1;
}

const char *RDOGradientPacketDescriptor::getFieldTypeString(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeString(field);
        field -= base->getFieldCount();
    }
    static const char *fieldTypeStrings[] = {
        "double",    // FIELD_gradientValue
    };
    return (field >= 0 && field < 1) ? fieldTypeStrings[field] : nullptr;
}

const char **RDOGradientPacketDescriptor::getFieldPropertyNames(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldPropertyNames(field);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    }
}

const char *RDOGradientPacketDescriptor::getFieldProperty(int field, const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldProperty(field, propertyName);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    }
}

int RDOGradientPacketDescriptor::getFieldArraySize(omnetpp::any_ptr object, int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldArraySize(object, field);
        field -= base->getFieldCount();
    }
    RDOGradientPacket *pp = omnetpp::fromAnyPtr<RDOGradientPacket>(object); (void)pp;
    switch (field) {
        default: return 0;
    }
}

void RDOGradientPacketDescriptor::setFieldArraySize(omnetpp::any_ptr object, int field, int size) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldArraySize(object, field, size);
            return;
        }
        field -= base->getFieldCount();
    }
    RDOGradientPacket *pp = omnetpp::fromAnyPtr<RDOGradientPacket>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set array size of field %d of class 'RDOGradientPacket'", field);
    }
}

const char *RDOGradientPacketDescriptor::getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldDynamicTypeString(object,field,i);
        field -= base->getFieldCount();
    }
    RDOGradientPacket *pp = omnetpp::fromAnyPtr<RDOGradientPacket>(object); (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

std::string RDOGradientPacketDescriptor::getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValueAsString(object,field,i);
        field -= base->getFieldCount();
    }
    RDOGradientPacket *pp = omnetpp::fromAnyPtr<RDOGradientPacket>(object); (void)pp;
    switch (field) {
        case FIELD_gradientValue: return double2string(pp->getGradientValue());
        default: return "";
    }
}

void RDOGradientPacketDescriptor::setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValueAsString(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    RDOGradientPacket *pp = omnetpp::fromAnyPtr<RDOGradientPacket>(object); (void)pp;
    switch (field) {
        case FIELD_gradientValue: pp->setGradientValue(string2double(value)); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'RDOGradientPacket'", field);
    }
}

omnetpp::cValue RDOGradientPacketDescriptor::getFieldValue(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValue(object,field,i);
        field -= base->getFieldCount();
    }
    RDOGradientPacket *pp = omnetpp::fromAnyPtr<RDOGradientPacket>(object); (void)pp;
    switch (field) {
        case FIELD_gradientValue: return pp->getGradientValue();
        default: throw omnetpp::cRuntimeError("Cannot return field %d of class 'RDOGradientPacket' as cValue -- field index out of range?", field);
    }
}

void RDOGradientPacketDescriptor::setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValue(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    RDOGradientPacket *pp = omnetpp::fromAnyPtr<RDOGradientPacket>(object); (void)pp;
    switch (field) {
        case FIELD_gradientValue: pp->setGradientValue(value.doubleValue()); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'RDOGradientPacket'", field);
    }
}

const char *RDOGradientPacketDescriptor::getFieldStructName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructName(field);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    };
}

omnetpp::any_ptr RDOGradientPacketDescriptor::getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructValuePointer(object, field, i);
        field -= base->getFieldCount();
    }
    RDOGradientPacket *pp = omnetpp::fromAnyPtr<RDOGradientPacket>(object); (void)pp;
    switch (field) {
        default: return omnetpp::any_ptr(nullptr);
    }
}

void RDOGradientPacketDescriptor::setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldStructValuePointer(object, field, i, ptr);
            return;
        }
        field -= base->getFieldCount();
    }
    RDOGradientPacket *pp = omnetpp::fromAnyPtr<RDOGradientPacket>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'RDOGradientPacket'", field);
    }
}

}  // namespace inet

namespace omnetpp {

}  // namespace omnetpp

