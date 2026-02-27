import inet.common.TagBase;

namespace simu5g;

enum PdcpOutputRoute
{
    PDCP_OUT_RLC = 0;
    PDCP_OUT_NR_RLC = 1;
    PDCP_OUT_X2 = 2;
    PDCP_OUT_UPPER = 3;
};

class PdcpOutputRoutingTag extends inet::TagBase
{
    PdcpOutputRoute route;
}
