#ifndef __UDSSERVER_H__
#define __UDSSERVER_H__

#include "xlet.h"

class UDSlet : public xlet::Xlet {

 protected:
        std::string     sockpath_;
        int             sockfd_;

 public:

        UDSlet(
        const std::string& sockpath,
        xlet::Direction direction,
        bool theLetListens = false
       );
        void configure();
        void start();
        virtual ~UDSlet() override{}

        bool valid() const override {  return sockfd_ >= 0; }
        std::size_t pushData(const uint64_t, const std::vector<std::byte>&) override;
        std::size_t pushData(const std::vector<std::byte>&) override;
        std::function<void()> inboundDataHandler = []()->void{};
        DAWn::Events::Signal<> inboundDataReady;

        DAWn::Events::Signal<uint64_t, std::vector<std::byte>&> letDataFromPeerReady;
};

class UDSOut : public UDSlet, public xlet::Out
{
    public:
        UDSOut(const std::string& sockpath);
        ~UDSOut(){}
};


class UDSIn : public UDSlet, public xlet::In
{
    public:
        UDSIn(const std::string& sockpath);
        ~UDSIn(){}


};

class UDSInOut : public UDSlet, public xlet::InOut
{
    public:
        UDSInOut(const std::string& sockpath, bool listen = false);
        ~UDSInOut(){}
};
#endif // __UDSSERVER_H__
