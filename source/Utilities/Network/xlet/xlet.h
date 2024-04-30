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

    struct Data
    {
        std::vector<std::byte> second;
        uint64_t first;
    };

    using Queue = std::vector<Data>;

    enum Transport   {
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



        DAWn::Events::Signal<int32_t, std::thread::id>  letIsListening;
        DAWn::Events::Signal<int32_t, std::string>      letOperationalError;
        DAWn::Events::Signal<>                          letInvalidSocketError;
        DAWn::Events::Signal<>                          letIsIINBOnly;

        DAWn::Events::Signal<int32_t, int32_t>          letAcceptedANewConnection;
        DAWn::Events::Signal<int32_t>                   letWillCloseConnection;


     protected:
        Transport transport;
        Direction direction;

        std::function<void(std::vector<std::byte>&)> incomingDataCallBack{[](auto&){}};
        std::vector<struct pollfd> pollfds_;



    };

    class In  {
    public:
        inline void push_back(const xlet::Data& d)
        {
            std::lock_guard<std::mutex> lock(mtxin_);
            qin_.push_back(d);
        }
        inline const bool empty() const
        {
            return qin_.empty();
        }

    protected:
        Queue qin_;
        std::mutex mtxin_;
    };

    class Out  {
    public:
        inline void push_back(const xlet::Data& d)
        {
            std::lock_guard<std::mutex> lock(mtxout_);
            qout_.push_back(d);
        }
        inline const bool emptyt() const
        {
            return qout_.empty();
        }

    protected:
        Queue qout_;
        std::mutex mtxout_;
    };

    class InOut  {
    public:
        inline void push_back(const xlet::Data& d, const xlet::Direction dir)
        {
            std::lock_guard<std::mutex> lock(dir == xlet::Direction::INB ? mtxin_ : mtxout_);
            if (dir == INB) qin_.push_back(d);
            else if (dir == OUTB) qout_.push_back(d);
            else std::cout << "push: Invalid direction" << std::endl;
        }
        inline const bool empty(const xlet::Direction dir) const
        {
            if (dir == INOUTB) std::cout << "empty: Invalid direction" << std::endl;
            return dir == INB ? qin_.empty() : ( dir  == OUTB ? qout_.empty() : false);
        }
    protected:
        Queue qin_;
        Queue qout_;
        std::mutex mtxin_;
        std::mutex mtxout_;
    };



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

