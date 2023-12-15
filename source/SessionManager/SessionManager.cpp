//
// Created by Julian Guarin on 9/12/23.
//

#include "SessionManager.h"

SessionManager& SessionManager::GetInstance()
{
    static SessionManager instance;
    return instance;
}

SessionManager::SessionManager()
{

}

void SessionManager::Initialize()
{

}

void SessionManager::Shutdown()
{
    //Close All Connections. Destroy Signaling Session.
}