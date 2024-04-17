//
// Created by Julian Guarin on 9/12/23.
//
#ifndef AUDIOSTREAMPLUGIN_SESSIONMANAGER_H
#define AUDIOSTREAMPLUGIN_SESSIONMANAGER_H
#include <string>
#include <random>
#include <fstream>
#include "AudioMixerBlock.h"

namespace DAWn::Session
{
    enum class Role
    {
        None,
        NonMixer,
        Mixer,
        Rogue //Every one mixes and sends their own audio
    };
    static std::unordered_map<Role, std::string> sRoleMap {
        {Role::None, "none"},
        {Role::NonMixer, "peer"},
        {Role::Mixer, "mixer"},
        {Role::Rogue, "rogue"}
    };


    template <typename T>
    class UserID
    {
        bool valid {false};
        T mId { 0xdeadbee0 };
        Role mRole { Role::None };
    public:
        DAWn::Events::Signal <std::string> sgnRoleAssigned;
        UserID()
        {
        }

        friend std::ostream& operator << (std::ostream& os, const UserID<T>& o)
        {
            os << "(" << sRoleMap[o.mRole] << ") " << o.mId;
            return os;
        }

        void SetRole(Role role)
        {
            if (role == Role::None) return;
            mRole = role;
            mId = 0xdeadbee0;
            while(mId >= 0xdeadbee0 && mId <= 0xdeadbeef)
            {
                static std::mt19937 generator(std::random_device{}()); // Initialize once with a random seed
                std::uniform_int_distribution<T> distribution;
                mId = distribution(generator);
            }
            mId = ((mId >> 1) << 1) | (role == Role::Mixer ? 0 : 1);
            sgnRoleAssigned.Emit(sRoleMap[mRole]);
        }
        inline const Role& GetRole() const
        {
            return mRole;
        }
        inline std::string GetRoleString() const
        {
            return sRoleMap[mRole];
        }

        inline T operator()(){ return mId; }
        inline bool IsValid() { return (mId >> 4) != 0xdeadbee; }

        inline bool IsRoleSet() const
        {
            return mRole != Role::None;
        }
    };
}




class SessionManager
{
    SessionManager();
    ~SessionManager() = default;

    std::string __authk{};
    //std::vector<Mixer::AudioMixerBlock>&__mixer;

    bool __isInitialized{false};
public:

    void Initialize();
    const bool operator()() const;
    static SessionManager& GetInstance();
};

#endif //AUDIOSTREAMPLUGIN_SESSIONMANAGER_H

