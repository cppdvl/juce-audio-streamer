//
// Created by Julian Guarin on 19/02/24.
//

#include "Configuration.h"
#include <iostream>
#include <fstream>
#include <set>

#include "nlohmann/json.hpp"

namespace DAWn::Utilities
{
    static bool validateSchema(nlohmann::json const& j, std::string& reason)
    {
        std::set<std::string> mandatoryKeys {"key"};
        for(auto& mandatoryKey : mandatoryKeys)
        {
            if (j.find(mandatoryKey) == j.end())
            {
                reason = "key not found: " + mandatoryKey;
                return false;
            }
        }

        //TODO: Add more validation on optional + types
        std::set<std::pair<std::string, std::string>> optionalType{
            {"role",                "std::string"}, //mixer, peer, none dflt:none
            {"bsize",               "uint64_t"},    //bsize => block size for the opus codec dflt:480
            {"srate",               "uint64_t"},    //sample rate dflt:48000
            {"channels",            "uint64_t"},    //number of channels dflt:2
            {"mono",                "bool"},        //mono stream dlft:false
            {"authEndpoint",        "std::string"}, //authEndpoint dflt:......
            {"wsEndpoint",          "std::string"}, //wsEndpoint dflt:......
            {"rtptype",             "std::string"}, //rtptype dflt:udpRTP
            {"port",                "int"},         //port dflt:8899
            {"ip",                  "std::string"}, //ip dlft:""
            {"rtrx",                "bool"},        //retransmision dflt: false
            {"opuscache",           "bool"},        //opuscache dflt: false
            {"prebuffersize",       "uint32_t"},    //prebuffersize dflt: 500 blocks (4.8x10^2 samplesperblock / 4.8x10^4 samplespersecond) x 500 blocks = (0.01 seconds x 500) = 5 seconds
            {"prebufferenabled",    "bool"},        //prebufferenabled dflt: false => will try to play once there is data
            {"mgmport",             "int"},         //mgmport dflt: 13001
            {"mgmip",               "std::string"}, //mgmip dflt: 0.0.0.0
            {"cli",                 "bool"},        //if true will listen to commands in the mgmport dflt: false
            {"delayseconds",        "uint32_t"},    //delayseconds dflt: 10
            {"wscommands",          "bool"},        //enable websocket commands (use mApikey etc). dflt true
            {"wsenroll",            "bool"},        //if enabled the enrollment would take place thru websocket channel, default true
            {"overridermssilence",  "bool"},        //if enabled process (and then streaming) will not be executed on silence. dflt: false
            {"requiresrole",        "bool"},        //if enabled process (and then streaming) will be executed only if role is defined. dflt: true
            {"loopback",            "bool"}         //if enabled will loopback the audio. dflt: false
        };
        for(auto& [key, type] : optionalType)
        {
            if (j.find(key) != j.end())
            {
                if (type == "uint64_t" && !j[key].is_number_unsigned()) {reason = "optional / wrong type"; return false; }
                if (type == "uint32_t" && !j[key].is_number_unsigned()) {reason = "optional / wrong type"; return false; }
                if (type == "int" && !j[key].is_number_integer()) {reason = "optional / wrong type"; return false; }
                if (type == "bool" && !j[key].is_boolean()) {reason = "optional / wrong type"; return false; }
                if (type == "std::string" && !j[key].is_string()) {reason = "optional / wrong type"; return false; }
            }
        }
        return true;
    }

    Configuration::Configuration(std::string const& filename)
    {
        auto homepath = std::string{std::getenv("HOME")};
        if (homepath.empty())
        {
            std::cout << "CRITICAL: HOME environment variable not set" << std::endl;
            return;
        }
        mFilename = *homepath.rbegin() == '/' ? homepath + filename : homepath + "/" + filename;

        std::string buff = "{}";
        {
            std::ifstream file(mFilename);
            if (!file.is_open())
            {
                std::cout << "CRITICAL: Could not open file: " << mFilename << std::endl;
                return;
            }
            else buff = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        }

        nlohmann::json j;
        try {
            j = nlohmann::json::parse(buff);
            std::string reason{};
            if (validateSchema(j, reason) != true)
            {
                std::cout << "CRITICAL: Invalid configuration: " << reason << std::endl;
                return;
            }
        } catch (nlohmann::json::parse_error& e) {
            std::cout << "CRITICAL: Could not parse file: " << mFilename << std::endl;
            return;
        }

        if (j.find("key")                   != j.end()) auth.key = j["key"];
        if (j.find("authEndpoint")          != j.end()) auth.authEndpoint = j["authEndpoint"];
        if (j.find("wsEndpoint")            != j.end()) auth.wsEndpoint = j["wsEndpoint"];

        if (j.find("bsize")                 != j.end()) audio.bsize = j["bsize"];
        if (j.find("srate")                 != j.end()) audio.srate = j["srate"];
        if (j.find("channels")              != j.end()) audio.channels = j["channels"];
        if (j.find("mono")                  != j.end()) audio.mono = j["mono"];

        if (j.find("rtptype")               != j.end()) transport.rtptype = j["rtptype"];
        if (j.find("port")                  != j.end()) transport.port = j["port"];
        if (j.find("ip")                    != j.end()) transport.ip = j["ip"];
        if (j.find("role")                  != j.end()) transport.role = j["role"];

        if (j.find("opuscache")             != j.end()) options.opuscache = j["opuscache"];
        if (j.find("mgmport")               != j.end()) options.mgmport = j["mgmport"];
        if (j.find("mgmip")                 != j.end()) options.mgmip = j["mgmip"];
        if (j.find("cli")                   != j.end()) options.cli = j["cli"];
        if (j.find("wscommands")            != j.end()) options.wscommands = j["wscommands"];
        if (j.find("wsenroll")              != j.end()) options.wsenroll = j["wsenroll"];
        if (j.find("delayseconds")          != j.end()) options.delayseconds = j["delayseconds"];

        if (j.find("overridermssilence")    != j.end()) debug.overridermssilence = j["overridermssilence"];
        if (j.find("requiresrole")          != j.end()) debug.requiresrole = j["requiresrole"];



        dump();

    }
    void Configuration::dump()
    {
        auto j = nlohmann::json{
            {"key", auth.key},
            {"authEndpoint", auth.authEndpoint},
            {"wsEndpoint", auth.wsEndpoint},

            {"bsize", audio.bsize},
            {"srate", audio.srate},
            {"channels", audio.channels},
            {"mono", audio.mono},

            {"rtptype", transport.rtptype},
            {"port", transport.port},
            {"ip", transport.ip},
            {"role", transport.role},

            {"opuscache", options.opuscache},
            {"mgmport", options.mgmport},
            {"mgmip", options.mgmip},
            {"cli", options.cli},
            {"wscommands", options.wscommands},
            {"wsenroll", options.wsenroll},
            {"delayseconds", options.delayseconds},

            {"overridermssilence", debug.overridermssilence},
            {"requiresrole", debug.requiresrole},

        };
        std::cout << j.dump(4) << std::endl;
    }
} // DAWn
