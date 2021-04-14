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

#ifndef _LTE_X2HANDOVERCOMMAND_H_
#define _LTE_X2HANDOVERCOMMAND_H_

#include "x2/packet/X2InformationElement.h"

//
// X2HandoverCommandIE
//
class X2HandoverCommandIE : public X2InformationElement
{
protected:

    bool startHandover_;
    MacNodeId ueId_;      // ID of the user performing handover

public:
  X2HandoverCommandIE()
  {
      type_ = X2_HANDOVER_CMD_IE;
      length_ = sizeof(MacNodeId) + sizeof(uint8_t);
      startHandover_ = false;
      ueId_ = 0;
  }
  X2HandoverCommandIE(const X2HandoverCommandIE& other) :
      X2InformationElement()
  {
      operator=(other);
  }

  X2HandoverCommandIE& operator=(const X2HandoverCommandIE& other)
  {
      if (&other == this)
          return *this;
      startHandover_ = other.startHandover_;
      ueId_ = other.ueId_;
      X2InformationElement::operator=(other);
      return *this;
  }
  virtual X2HandoverCommandIE *dup() const
  {
      return new X2HandoverCommandIE(*this);
  }
  virtual ~X2HandoverCommandIE() {}

  // getter/setter methods
  void setStartHandover() { startHandover_ = true; }
  bool isStartHandover() { return startHandover_; }
  void setUeId(MacNodeId ueId) { ueId_ = ueId; }
  MacNodeId getUeId() { return ueId_; }
};

#endif
