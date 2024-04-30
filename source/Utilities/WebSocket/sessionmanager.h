#ifndef UNTITLED_SESSIONMANAGER_H
#define UNTITLED_SESSIONMANAGER_H

#include "Events.h"
#include "nlohmann/json.hpp"

#include <string>

namespace DAWn {

    //DOOB = DAWn Out Of Band control protocol. Not a mary jane reference.
    class SessionManager {
        bool mAuthenticated = false;
        std::string mApiKey;

     public:
        /*!
         * @brief Initialize the session manager. This will create the web socket connection to the server.
         */
        virtual void Init() = 0;

        /*!
         * @brief Authenticate the session manager with the API key. Internally this will use the key to get the plugin Token.\n
         * The plugin token is used to connect to the web socket server on a persistent connection (NOT REST).
         *
         * @param apiKey. The key from the FrontEnd application.
         * @return return true if the session manager was initialized correctly.
         */
        bool Authenticate(
        const std::string& apiKey,
        const std::string& authUrl = "https://r8831cvez5.execute-api.us-east-1.amazonaws.com/auth/plugin/authorize?APIKey=");

        /*!
         * @brief Authenticate to the web socket server using the plugin token.
         * @param wshost. The host name of the web socket server.
         * @param path. The path to the web socket server.
         * @param secure. If the connection is secure or not.
         * */
        virtual void Run(
            const std::string wshost,
            bool secure = true) = 0;

        DAWn::Events::Signal<> OnConnected;
        DAWn::Events::Signal<const char*/*, std::string&, bool&*/> OnMessageReceived;
        DAWn::Events::Signal<> OnKeepAliveLapsed;
     protected:
        std::string mPluginToken{};

    };

    class WSManager : public SessionManager {

     public:
        void Init() override;
        void Run(std::string wshost, bool secure = true) override;
        void Send(const nlohmann::json& message, bool timestampit = false);
        virtual ~WSManager();
    };

    using SMSPtr = std::shared_ptr<SessionManager>;

}



#endif //UNTITLED_SESSIONMANAGER_H
