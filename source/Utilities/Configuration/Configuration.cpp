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
    bool validateSchema(nlohmann::json const& j, std::string& reason)
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
            {"role", "std::string"},
            {"bsize", "uint64_t"},
            {"srate", "uint64_t"},
            {"channels", "uint64_t"},
            {"mono", "bool"},
            {"authEndpoint", "std::string"},
            {"wsEndpoint", "std::string"},
            {"rtptype", "std::string"},
            {"port", "int"},
            {"ip", "std::string"},
            {"rtrx", "bool"},
            {"opuscache", "bool"},
            {"prebuffersize", "uint32_t"},
            {"prebufferenabled", "bool"},
            {"mgmport", "int"},
            {"mgmip", "std::string"},
            {"cli", "bool"},
            {"wscommands", "bool"}
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
            dump();
            if (validateSchema(j, reason) != true)
            {
                std::cout << "CRITICAL: Invalid configuration: " << reason << std::endl;
                return;
            }
        } catch (nlohmann::json::parse_error& e) {
            std::cout << "CRITICAL: Could not parse file: " << mFilename << std::endl;
            return;
        }



        if (j.find("key") != j.end()) auth.key = j["key"];
        if (j.find("authEndpoint") != j.end()) auth.authEndpoint = j["authEndpoint"];
        if (j.find("wsEndpoint") != j.end()) auth.wsEndpoint = j["wsEndpoint"];
        if (j.find("bsize") != j.end()) audio.bsize = j["bsize"];
        if (j.find("srate") != j.end()) audio.srate = j["srate"];
        if (j.find("channels") != j.end()) audio.channels = j["channels"];
        if (j.find("mono") != j.end()) audio.mono = j["mono"];
        if (j.find("rtptype") != j.end()) transport.rtptype = j["rtptype"];
        if (j.find("port") != j.end()) transport.port = j["port"];
        if (j.find("ip") != j.end()) transport.ip = j["ip"];
        if (j.find("opuscache") != j.end()) options.opuscache = j["opuscache"];
        if (j.find("prebuffersize") != j.end()) options.prebuffersize = j["prebuffersize"];
        if (j.find("prebufferenabled") != j.end()) options.prebufferenabled = j["prebufferenabled"];
        if (j.find("mgmport") != j.end()) options.mgmport = j["mgmport"];
        if (j.find("mgmip") != j.end()) options.mgmip = j["mgmip"];
        if (j.find("cli") != j.end()) options.cli = j["cli"];
        if (j.find("wscommands") != j.end()) options.wscommands = j["wscommands"];
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
            {"opuscache", options.opuscache},
            {"prebuffersize", options.prebuffersize},
            {"prebufferenabled", options.prebufferenabled},
            {"mgmport", options.mgmport},
            {"mgmip", options.mgmip},
            {"cli", options.cli},
            {"wscommands", options.wscommands}
        };
        std::cout << j.dump(4) << std::endl;
    }
} // DAWn
