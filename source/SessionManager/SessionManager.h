//
// Created by Julian Guarin on 9/12/23.
//
#ifndef AUDIOSTREAMPLUGIN_SESSIONMANAGER_H
#define AUDIOSTREAMPLUGIN_SESSIONMANAGER_H
#include <string>
#include "AudioMixerBlock.h"

class SessionManager
{
    SessionManager();
    ~SessionManager() = default;

    std::string __authk{};
    std::vector<Mixer::AudioMixerBlock>&__mixer;

public:

    void Initialize();
    void CreateSession();
    void CreateStream();
    void DestroyStream();
    void DestroySession();
    void Shutdown();
    void GetSession();
    void GetStream();
    void GetSP();

    static SessionManager& GetInstance();
};

#endif //AUDIOSTREAMPLUGIN_SESSIONMANAGER_H
