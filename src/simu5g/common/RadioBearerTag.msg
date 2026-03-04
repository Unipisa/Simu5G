//                  Simu5G
//
// Authors: Andras Varga
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//
import inet.common.INETDefs;
import inet.common.TagBase;

cplusplus {{
   using namespace inet;
}};

namespace simu5g;

class RadioBearerTagBase extends inet::TagBase {
	    int drbIndex = -1;
}

class RadioBearerReq extends RadioBearerTagBase
{
}

class RadioBearerInd extends RadioBearerTagBase
{
}