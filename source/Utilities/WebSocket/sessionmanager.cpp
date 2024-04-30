#include "websocketpp/config/asio_client.hpp"
#include "websocketpp/client.hpp"
#include "nlohmann/json.hpp"

#include "sessionmanager.h"
#include "sessionmanagermessages.h"

#include <chrono>
#include <string>
#include <iostream>

#include <curl/curl.h>


namespace DAWn {
    using ContextPtr = websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context>;
    using WSClient = websocketpp::client<websocketpp::config::asio_tls_client>;
    using ThrdSPtr = websocketpp::lib::shared_ptr<websocketpp::lib::thread>;
    using ConnectionHndlr = websocketpp::connection_hdl;
    using WSError = websocketpp::lib::error_code;
    ContextPtr notls(const char *hostname, websocketpp::connection_hdl ) {
        ContextPtr ctx = websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23);

        //get a connection out of connection_hdl


        try {
            ctx->set_options(boost::asio::ssl::context::default_workarounds |
                             boost::asio::ssl::context::no_sslv2 |
                             boost::asio::ssl::context::no_sslv3 |
                             boost::asio::ssl::context::single_dh_use);
            ctx->set_verify_mode(boost::asio::ssl::verify_none);
        } catch (::std::exception &e) {
            std::cout << e.what() << std::endl;
        }
        return ctx;
    }

    struct WebSocketEndPoint {
        WSManager& mWSManager;
        WSClient mClient;
        ThrdSPtr mThread {nullptr};
        ConnectionHndlr h;

        void Send(std::string msg)
        {
            WSError ec;
            mClient.send(h, msg, websocketpp::frame::opcode::text, ec);
            if (ec)
            {
                std::cout << "Could not send message because: " << ec.message() << std::endl;
            }
        }
        void Close()
        {
            std::cout << "Websocket: Connection Closed." << std::endl;
            mClient.stop_perpetual();
            mClient.close(h, websocketpp::close::status::going_away, "Shutting down websocket client");
            mClient.get_io_service().stop();
            if (mThread->joinable())
            {
                mThread->join();
            }
        }

        WebSocketEndPoint(WSManager& wsmanager, std::string mPluginToken, std::string wshost, bool secure) : mWSManager(wsmanager)
        {
            auto uri = "ws" + std::string{secure ? "s" : ""} + "://" + std::string{wshost} ;
            std::cout << "***************************************************************************************" << std::endl;
            std::cout << "URI: " << uri << std::endl;
            std::cout << "+ ----------------------------------------------------------------------------------- *" << std::endl;
            std::cout << "  HEADER" << std::endl;
            std::cout << "+ ----------------------------------------------------------------------------------- *" << std::endl;
            std::cout << "  Authorization :" << std::endl;
            std::cout << "  " << mPluginToken << std::endl;
            std::cout << "***************************************************************************************" << std::endl;

            mClient.clear_access_channels(websocketpp::log::alevel::all);
            mClient.set_access_channels(websocketpp::log::alevel::connect | websocketpp::log::alevel::disconnect);
            mClient.init_asio();
            mClient.start_perpetual();
            mClient.set_tls_init_handler(std::bind(&notls, wshost.c_str(), std::placeholders::_1));

            mClient.set_message_handler([this](websocketpp::connection_hdl hdl, WSClient::message_ptr msg) {
                mWSManager.OnMessageReceived.Emit(msg->get_payload().c_str());
            });

            mClient.set_open_handler([this](websocketpp::connection_hdl hdl) {
                h = hdl;
                mWSManager.OnConnected.Emit();
                nlohmann::json msgj = DAWn::Messages::AudioSettingsChanged(48000, 480, 32);
                std::string msg = msgj.dump();
                Send(msg);
        });

            websocketpp::lib::error_code ec;
            WSClient::connection_ptr connectionPtr = mClient.get_connection(uri, ec);
            if (ec) {
                std::cout << "Could not create connection because: " << ec.message() << std::endl;
                return;
            }
            connectionPtr->append_header("Authorization", "Bearer " + mPluginToken);
            //connectionPtr->set_proxy("http://127.0.0.1:8080");
            mClient.connect(connectionPtr);
            mThread = websocketpp::lib::make_shared<websocketpp::lib::thread>(&WSClient::run, &mClient);

            std::cout << "No blocking!!" << std::endl;
        }
        ~WebSocketEndPoint()
        {

        }
    };
}
namespace DAWn {
    static std::shared_ptr<WebSocketEndPoint> spPep{nullptr};
    static size_t WriteCallback (char* ptr, size_t size, size_t nmemb, std::string* data)
    {
        data->append(ptr, size * nmemb);
        return size * nmemb;
    }

    bool SessionManager::Authenticate(const std::string& apiKey, const std::string& authUrl)
    {
        auto authUrl_= authUrl+"/auth/plugin/authorize?APIKey="+apiKey;
        CURL* curl;
        CURLcode res;
        ::std::string readBuffer{};
        curl_global_init(CURL_GLOBAL_DEFAULT);
        curl = curl_easy_init();

        if(curl)
        {
            curl_easy_setopt(curl, CURLOPT_URL, authUrl_.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            res = curl_easy_perform(curl);

            if (res != CURLE_OK){
                std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            }
            curl_easy_cleanup(curl);
        }
        curl_easy_cleanup(curl);

        mPluginToken = readBuffer;
        return res == CURLE_OK;
    }

    void WSManager::Init()
    {
        std::string timestamp = std::to_string(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
        std::cout << "" << std::endl;
    }

    void WSManager::Run(std::string wshost, bool secure)
    {
        std::cout << "Run with object @0x" << std::hex << (uint64_t)spPep.get() << std::endl;
        std::cout << "Creating Websocket Object" << std::endl;
        if (spPep)
        {
            std::cout << "Refreshing runs....." << std::endl;
            spPep.reset(new WebSocketEndPoint(*this, mPluginToken, wshost, secure));
        }
        else
        {
            spPep = std::make_shared<WebSocketEndPoint>(*this, mPluginToken, wshost, secure);
        }

    }

    void WSManager::Send(const nlohmann::json& kMessage, bool timestampit)
    {

        if (!spPep) {
            std::cerr << "No connection to the server" << std::endl;
            return;
        }
        auto nMessage = kMessage;

        if (timestampit)
        {
            //Maybe this is violating the separation of concerns.
            nMessage["data"]["TimeStamp"] = std::to_string(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
        }

        std::string _j = nMessage.dump();
        std::cout << "json: " << std::endl;
        std::cout << std::setw(4) << kMessage << std::endl;
        std::cout << "json string: " << std::endl;
        std::cout << _j << std::endl;
        spPep->Send(_j);
    }

    WSManager::~WSManager()
    {
        static bool justOnce = true;
        if (spPep && justOnce)
        {
            justOnce = false;
            std::cout << "Closing websocket......" << std::endl;
            spPep->Close();
            std::cout << "Websocket is gone..... resetting the holding shared pointer" << std::endl;
            spPep.reset();
            std::cout << "Shared pointer restted..." << std::endl;
        }
    }
}
