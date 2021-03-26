//
// Copyright (C) 2004 Andras Varga
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#ifndef _RNISERVICE_H
#define _RNISERVICE_H

#include "nodes/mec/MEPlatform/MeServices/MeServiceBase/MeServiceBase.h"
#include "nodes/mec/MEPlatform/MeServices/RNIService/resources/L2Meas.h"


/**
 *
 *
 *
 */


class RNIService: public MeServiceBase
{
  private:

    L2Meas L2MeasResource_;

    bool scheduledSubscription;

  public:
    RNIService();
  protected:

    virtual void initialize(int stage) override;
    virtual void finish() override;

    virtual void handleGETRequest(const std::string& uri, inet::TcpSocket* socket) override;
    virtual void handlePOSTRequest(const std::string& uri, const std::string& body, inet::TcpSocket* socket) override;
    virtual void handlePUTRequest(const std::string& uri, const std::string& body, inet::TcpSocket* socket) override;
    virtual void handleDELETERequest(const std::string& uri, inet::TcpSocket* socket) override;

    virtual ~RNIService();


};


#endif // ifndef _RNISERVICE_H

