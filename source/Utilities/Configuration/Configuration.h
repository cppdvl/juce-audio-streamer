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
            std::string ip{"127.0.0.1"};
            std::string role{"none"};
            bool rtrx{false};
        }transport;

        struct {
            bool manifest = false;
        }version;


        struct {
            bool opuscache = false;


            int mgmport {0};
            std::string mgmip {"0.0.0.0"};
            bool cli{false};

            bool wscommands {true};

            uint32_t delayseconds {0};

        }options;

        struct {
            /*!
             * @brief If true the plugin will be idle when the AudioBuffer is silent.
             */
            bool overridermssilence = true;

            /*!
             * @brief If true the plugin will require a role is set prior to stream.
             */
            bool requiresrole = true;


            /*!
             * @brief LOOPBACK if enabled NO NETWORK STREAMING WILL TAKE PLACE. Data will be pushed into the BSA then ENCODED then DECODED then pushed into the BSA again and then it will go into the audio mixer to replace whatever is in the audio playhead.
             */
            bool loopback = false; //if enabled there won't be streaming to the network, data will be pushed into the BSA then ENCODED then DECODED then pushed into the BSA again. dflt: false

            struct {
                int mgmport{13001};
                std::string mgmip{"0.0.0.0"};
                bool cli{false};
            }notimplemented;

        }debug;

        void dump();
        explicit Configuration(std::string const& );


    };

} // DAWn

#endif //AUDIOSTREAMPLUGIN_CONFIGURATION_H
