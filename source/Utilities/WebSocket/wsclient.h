
#include "Events.h"
#include "streammanager.h"
#include "sessionmanager.h"
#include "sessionmanagermessages.h"

#include <map>
#include <deque>
#include <mutex>
#include <memory>
#include <thread>
#include <iostream>
#include <functional>


class WebSocketApplication {

    //Signaling
    DAWn::Events::Signal<> OnKeepAliveLapsed;

 public:
    
    DAWn::SMSPtr pSm {std::make_shared<DAWn::WSManager>()};
    

    void OnConnected();

    /*!
     * @brief Initialize the WebSocketApplication
     *
     * @param apiKey The API key to authenticate with the backend
     * @param wsAuth The Url to the WebSocket authentication endpoint
     */
    void Init(const std::string& apiKey, const std::string& urlAuth, const std::string& urlWS);
    void OnMessageReceived(const char*);
    void On30SecondsSlot();

    
    DAWn::Events::Signal<> ApiKeyAuthFailed;
    DAWn::Events::Signal<> ApiKeyAuthSuccess;
    DAWn::Events::Signal<const char*> OnYouAreHost;
    DAWn::Events::Signal<const char*> OnDisconnectCommand;
    DAWn::Events::Signal<const char*> OnYouArePeer;
    DAWn::Events::Signal<const char*> ThisPeerIsGone;
    DAWn::Events::Signal<const char*> ThisPeerIsConnected;
    DAWn::Events::Signal<const char*> AudioSettingsChanged;
    DAWn::Events::Signal<const char*> OnSendAudioSettings;
    DAWn::Events::Signal<const char*> AckFromBackend;
    DAWn::Events::Signal<const char*> TransportIPSettings;
    DAWn::Events::Signal<const char*> OnCommand;
};
