//
// Created by Julian Guarin on 10/12/23.
//
#include "wsclient.h"




void WebSocketApplication::Init(
const std::string& apiKey,
const std::string& urlAuth,
const std::string& urlWS)
{
    auto authLambda = [apiKey, urlAuth, urlWS, this]()->bool
    {
        if (urlAuth.empty()) return this->pSm->Authenticate(apiKey);
        return this->pSm->Authenticate(apiKey, urlAuth);
    };


    std::function<void()> SendKeepAlive ([this](){
        DAWn::SessionManager* pWSManager = pSm.get();
        const nlohmann::json kKeepAliveMessage = DAWn::Messages::KeepAlive();
        dynamic_cast<DAWn::WSManager*>(pWSManager)->Send(kKeepAliveMessage);
    });
    
    OnKeepAliveLapsed.Connect(SendKeepAlive); 
    /*OnKeepAliveLapsed.Connect(this, std::function<void()>{+
            [this](){
            DAWn::SessionManager* pWSManager = pSm.get();
            const auto kKeepAliveMessage = DAWn::Messages::KeepAlive();
            dynamic_cast<DAWn::WSManager*>(pWSManager)->Send(kKeepAliveMessage);
            }});
    std::thread t(KeepAliveThreadLambda);t.detach();*/
    
    pSm->OnConnected.Connect(this, &WebSocketApplication::OnConnected);
    pSm->OnMessageReceived.Connect(this, &WebSocketApplication::OnMessageReceived);

    std::cout << "Authenticating with API Key: " << apiKey << std::endl;
    if (authLambda())
    {
        ApiKeyAuthSuccess.Emit();
        pSm->Run(urlWS);
    }
    else
    {
        ApiKeyAuthFailed.Emit();
        std::cout << "Authentication failed" << std::endl;
    }
}

void WebSocketApplication::OnConnected()
{
    std::cout << "Connected" << std::endl;
    const auto kAudioSettingsMessage = DAWn::Messages::AudioSettingsChanged(48000, 480, 32);
    DAWn::SessionManager* pWSManager = pSm.get();
    dynamic_cast<DAWn::WSManager*>(pWSManager)->Send(kAudioSettingsMessage);
}

void WebSocketApplication::OnMessageReceived(const char* msg)
{
    /*!
     * @brief This is a map of the message types and the handlers for each message type.
     * @note This is a lambda function that captures the this pointer. This is done so that the lambda function can call the member functions of the class.
     * @note The lambda function is called with the message payload as the argument.
     * @note The message payload is a json object.
     *
     * @note 0: if you receive this message, you are the host.
         * @note 1: if you receive this message, you - a non host user - are being notified that the host was disconnected, in such case you should disconnect as well.
     * @note 2: if you receive this message, you are a non host user, and you are being notified about the host's transport settings to connect at.
     * @note 4: if you receive this message, you are the host, and you are being notified that a user has connected to your session. The notification payload contains the user's transport settings to connect at.
     */
    std::map<int, std::function<void(const char*)>> kMessageHandlers
    {
        {0,     [this](const char* msg){ this->OnYouAreHost.Emit                (msg);}},
        {1,     [this](const char* msg){ this->OnDisconnectCommand.Emit         (msg);}},
        {2,     [this](const char* msg){ this->OnYouArePeer.Emit                (msg);}},
        {3,     [this](const char* msg){ this->ThisPeerIsGone.Emit              (msg);}},
        {4,     [this](const char* msg){ this->ThisPeerIsConnected.Emit         (msg);}},
        {11,    [this](const char* msg){ this->AudioSettingsChanged.Emit        (msg);}},
        {9,     [this](const char* msg){ this->AckFromBackend.Emit              (msg);}},
        {12,    [this](const char* msg){ this->TransportIPSettings.Emit         (msg);}},
        {13,    [this](const char* msg){ this->OnCommand.Emit                   (msg);}}
    };

    //std::cout << "Message Received: " << msg << std::endl;
    auto j = json::parse(msg);
    if (j.find("Type") == j.end())
    {
      std::cout << "Answer to command not handled: " << j.dump() << std::endl;
      std::cout << std::setw(4) << j << std::endl;
      return;
    }
    else if (kMessageHandlers.find(j["Type"]) != kMessageHandlers.end())
    {
        kMessageHandlers[j["Type"]](j["Payload"].dump().c_str());
    }
    else
    {
        std::cout << "Message type not handled: " << j["Type"] << std::endl;
    }
}

