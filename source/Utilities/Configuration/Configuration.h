//
// Created by Julian Guarin on 19/02/24.
//

#ifndef AUDIOSTREAMPLUGIN_CONFIGURATION_H
#define AUDIOSTREAMPLUGIN_CONFIGURATION_H

#include <cstdint>
#include <string>


namespace DAWn::Utilities
{

    class Configuration
    {
    public:
        std::string mFilename;
        struct {
            uint64_t bsize = 480;
            uint64_t srate = 48000;
            uint64_t channels = 2;
            bool mono = false;

        } audio;

        struct {
            std::string key{};
            std::string authEndpoint{"https://r8831cvez5.execute-api.us-east-1.amazonaws.com"};
            std::string wsEndpoint{"65qepm0kbf.execute-api.us-east-1.amazonaws.com"};
        }auth;

        struct {
            std::string rtptype{"udpRTP"};
            int port{8899};
            std::string ip{""};
            std::string role{"none"};
        }transport;

        struct {
            bool manifest = false;
        }version;


        struct {
            bool opuscache = false;

            uint32_t prebuffersize { 500 }; // 5 seconds
            bool prebufferenabled { false };

            int mgmport {8899};
            std::string mgmip {"0.0.0.0"};
            bool cli{false};

            bool wscommands {true};

        }options;

        struct {
            bool overridermssilence = false;
        }debug;

        void dump();
        explicit Configuration(std::string const& );


    };

} // DAWn

#endif //AUDIOSTREAMPLUGIN_CONFIGURATION_H
