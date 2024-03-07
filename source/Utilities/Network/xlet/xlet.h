#ifndef XLET_H
#define XLET_H

#include <map>
#include <set>
#include <mutex>
#include <queue>
#include <memory>
#include <thread>
#include <vector>
#include <cstddef>
#include <sstream>
#include <utility>
#include <iostream>
#include <functional>

#include "signalsslots.h"

/* POSIX */
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

/* WINDOWS SHIT */

#define XLET_MAXBLOCKSIZE_SHIFTER   13
#define XLET_MAXBLOCKSIZE           (1 << XLET_MAXBLOCKSIZE_SHIFTER)



// Forward declaration of a template class Queue
namespace xlet {
    template <typename T>
    class Id
    {
        inline static uint64_t id__{0};
     protected:
        uint64_t id{id__++};
    };
    using Data = std::pair<uint64_t, std::vector<std::byte>>;

    class Queue : public std::queue<Data>{
     public:
        DAWn::Events::Signal<> dataReady;
             private:
        std::mutex mutex_;
    };


    enum Transport {
        UVGRTP, // UVG RTP
        UDS, //SOQ_SEQPACKET
        TCP, //SOCK_STREAM
        UDP  //SOCK_DGRAM
    };
    enum Direction {
        INB,
        OUTB,
        INOUTB
    };


    class Xlet {

     public:

        // Virtual destructor for proper cleanup in derived classes
        Xlet(){}
        virtual ~Xlet() = default;
        virtual bool valid () const {return false;}

        /*!
         *
         * @param data
         * @return
         */
        virtual std::size_t pushData(const uint64_t destId, const std::vector<std::byte>& data) = 0;
        virtual std::size_t pushData(const std::vector<std::byte>& data) = 0;
        void setIncomingDataCallBack(std::function<void(std::vector<std::byte>&)> callback);



        DAWn::Events::Signal<uint64_t, std::thread::id> letIsListening;
        DAWn::Events::Signal<uint64_t, std::string>     letOperationalError;
        DAWn::Events::Signal<>                          letInvalidSocketError;
        DAWn::Events::Signal<>                          letIsIINBOnly;

        DAWn::Events::Signal<uint64_t, uint64_t >   letAcceptedANewConnection;
        DAWn::Events::Signal<uint64_t>              letWillCloseConnection;


     protected:
        Transport transport;
        Direction direction;

        std::function<void(std::vector<std::byte>&)> incomingDataCallBack{[](auto&){}};
        std::vector<struct pollfd> pollfds_;



    };

    class In  {
     public:
        Queue qin_;

    };

    class Out  {
     public:
        Queue qout_;
    };

    class InOut  {
     public:
        Queue qin_;
        Queue qout_;
    };



#include "uds.h"
#include "udp.h"

    struct Configuration
    {
        std::string     address{"/tmp/tmpUDSserver"};
        Transport       transport{Transport::UDS};
        Direction       direction{Direction::INOUTB};
        std::string&    sockpath{address};
        int             port0{0};
        int             port1{0};
        int&            port{port0};
        int&            srcport{port0};
        int&            dstport{port1};

        bool            isValidConfiguration() const;

    };

    class Factory {
     public:

        static std::shared_ptr<Xlet> createXlet(const Configuration configuration);
    };

}





#endif // XLET_H

