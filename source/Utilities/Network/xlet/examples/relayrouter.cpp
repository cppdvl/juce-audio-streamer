//
// Created by Julian Guarin on 10/01/24.
//
#include "xlet.h"

xlet::UDPInOut* CreateRouter (std::string ip, int port, bool listen, bool synced);

int main(int argc, char**argv)
{
    auto router = CreateRouter("0.0.0.0", 8899, true, true);
    if (router)
    {
        router->run();
        router->join();

    }
    delete router;
    return 0;
}
