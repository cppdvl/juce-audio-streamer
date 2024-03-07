

#include "xlet.h"


bool xlet::Configuration::isValidConfiguration() const
{
    if (transport != xlet::Transport::UDP)
    {
        if (port == 0 || address.empty())
        {
            return false;
        }
    }
    else if (transport == xlet::Transport::UDP)
    {
        if (port == 0 || address.empty())
        {
            return false;
        }
    }
    else if (transport == xlet::Transport::UDS)
    {
        if (sockpath.empty())
        {
            return false;
        }
    }
    else
    {
        return false;
    }
    return true;
}

class FactoryHelper {

    public:
        static std::shared_ptr<xlet::Xlet> SpawnUDPOut(const xlet::Configuration& config)
        {
            auto xlet = std::make_shared<xlet::UDPOut>(config.address, config.port);
            return xlet;
        }
};



std::shared_ptr<xlet::Xlet> xlet::Factory::createXlet(const xlet::Configuration cofiguration)
{
    if (!cofiguration.isValidConfiguration())
    {
        return nullptr;
    }
    
    return FactoryHelper::SpawnUDPOut(cofiguration);

}

void xlet::Xlet::setIncomingDataCallBack(std::function<void(std::vector<std::byte>&)> callback)
{
    incomingDataCallBack = callback;
}

