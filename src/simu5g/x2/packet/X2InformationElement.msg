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

namespace simu5g;

// add here new IE types
enum X2InformationElementType
{
    COMP_REQUEST_IE = 0;         // CoMP slave -> master
    COMP_REPLY_IE = 1;           // CoMP master -> slave
    COMP_PROP_REQUEST_IE = 2;    // CoMP slave -> master (for compManagerProportional, with RB value)
    COMP_PROP_REPLY_IE = 3;      // CoMP master -> slave (for compManagerProportional, with map)
    X2_HANDOVER_CMD_IE = 4;      // HO command source eNB -> target eNB
};

//
// X2 Information Element
//
// Extend this class to define new Information Elements
//
class X2InformationElement extends cObject
{
    X2InformationElementType type;
    int64_t length;  // Length in bytes
};

cplusplus(X2InformationElement) {{
  public:
    X2InformationElement(X2InformationElementType type) : type(type) {}
}}
