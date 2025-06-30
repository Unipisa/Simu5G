//
//                           Simu5G
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef NODES_MEC_MECORCHESTRATOR_APPLICATIONDESCRIPTOR_APPLICATIONDESCRIPTOR_H_
#define NODES_MEC_MECORCHESTRATOR_APPLICATIONDESCRIPTOR_APPLICATIONDESCRIPTOR_H_

#include <string.h>
#include <omnetpp.h>
#include "nodes/mec/utils/MecCommon.h"
#include "nodes/mec/utils/httpUtils/json.hpp"

namespace simu5g {

class ApplicationDescriptor
{
  private:
    /*
     * According to ETSI GS MEC 010-2 6.2.1.2 an Application Descriptor (AppD) is a part of the
     * application package, and describes applications requirements and rules required by the
     * application provider.
     *
     * This class uses only a subset of the entire AppD, just the necessary ones.
     *
     */
    std::string appDId_;
    std::string appName_;
    std::string appProvider_;
    std::string appInfoName_;
    std::string appDescription_;
    ResourceDescriptor virtualResourceDescritor_;
    std::vector<std::string> appServicesRequired_;
    std::vector<std::string> appServicesProduced_;

    std::string omnetppServiceRequired_;

    /*
     * emulated mecApplication variables
     */
    bool isEmulated;
    std::string externalAddress;
    int externalPort;

  public:
    ApplicationDescriptor() {}
    ApplicationDescriptor(const char *fileName); // read the AppD from file
    ApplicationDescriptor(const std::string& appDid, const std::string& appName, const std::string& appProvider, const std::string& appInfoName, const std::string& appDescription,
            const ResourceDescriptor& resources, std::vector<std::string> appServicesRequired, std::vector<std::string> appServicesProduced);

    /*
     * getters
     */
    std::string getAppDId() const { return appDId_; }
    std::string getAppName() const { return appName_; }
    std::string getAppProvider() const { return appProvider_; }
    std::string getAppInfoName() const { return appInfoName_; }
    std::string getAppDescription() const { return appDescription_; }

    std::string getOmnetppServiceRequired() const { return omnetppServiceRequired_; }

    std::string getExternalAddress() const { return externalAddress; }
    int getExternalPort() const { return externalPort; }

    ResourceDescriptor getVirtualResources() const { return virtualResourceDescritor_; }
    const std::vector<std::string>& getAppServicesRequired() const { return appServicesRequired_; }
    const std::vector<std::string>& getAppServicesProduced() const { return appServicesProduced_; }

    bool isMecAppEmulated() const { return isEmulated; }

    /*
     * setters
     */
    void setAppDId(const std::string& appdId) { appDId_ = appdId; }
    void setAppName(const std::string& appName) { appName_ = appName; }
    void setAppProvider(const std::string& appProvider) { appProvider_ = appProvider; }
    void setAppInfoName(const std::string& appInfoName) { appInfoName_ = appInfoName; }
    void setAppDescription(const std::string& appDescription) { appDescription_ = appDescription; }
    void SetVirtualResources(const ResourceDescriptor& virtualResourceDescritor) { virtualResourceDescritor_ = virtualResourceDescritor; }
    void SetAppServicesProduced(const std::vector<std::string>& appServicesRequired) { appServicesRequired_ = appServicesRequired; }
    void SetAppServicesRequired(const std::vector<std::string>& appServicesProduced) { appServicesRequired_ = appServicesProduced; }

    nlohmann::ordered_json toAppInfo() const;
    void printApplicationDescriptor() const;
};

} //namespace

#endif /* NODES_MEC_MECORCHESTRATOR_APPLICATIONDESCRIPTOR_APPLICATIONDESCRIPTOR_H_ */

