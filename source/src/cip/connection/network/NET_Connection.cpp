//Includes

#undef WIN
#if defined(_WIN32) || defined(WIN32) || defined(__CYGWIN__) || defined(__MINGW32__)
#define WIN
#endif

#ifndef WIN
#include <unistd.h>
#endif // !WIN32

#include "../../../trace.hpp"
#include "NET_Connection.hpp"

//Static variables
fd_set NET_Connection::select_set[2];
std::map <int, NET_Connection*> NET_Connection::socket_to_conn_map;

//Methods
void NET_Connection::InitSelects()
{
    // clear the master an temp sets
    FD_ZERO(&select_set[kMasterSet]);
    FD_ZERO(&select_set[kReadSet]);
}
void NET_Connection::SelectCopy()
{
    select_set[kReadSet] = select_set[kMasterSet];
}
int NET_Connection::SelectSet(int socket_handle, int select_set_option)
{
    //if(CheckSelectSet(select_set_option) & CheckSocketHandle(socket_handle))
    {
        FD_SET(socket_handle, &select_set[select_set_option]);
        return 1;
    }
    return INVALID_INPUTS;
}

int NET_Connection::SelectIsSet(int socket_handle, int select_set_option)
{
    //if(CheckSelectSet(select_set_option) & CheckSocketHandle(socket_handle))
    {
        return FD_ISSET(socket_handle, &select_set[select_set_option]);
    }
    return INVALID_INPUTS;
}

int NET_Connection::SelectSelect(int socket_handle, int select_set_option, struct timeval * time)
{
    //if(CheckSelectSet(select_set_option) & CheckSocketHandle(socket_handle))
    {
        return select(socket_handle, &select_set[select_set_option], 0, 0, time);
    }
    return INVALID_INPUTS;
}

int NET_Connection::SelectRemove(int socket_handle, int select_set_option)
{
    //if(CheckSelectSet(select_set_option))
    {
        FD_CLR(socket_handle, &select_set[select_set_option]);
        return 0;
    }
    return INVALID_INPUTS;
}




NET_Connection::~NET_Connection ()
{
    if (sock != INVALID_SOCKET_HANDLE)
        CloseSocket();

    delete remote_address;
    delete originator_address;
}

int NET_Connection::InitSocket(CipUdint family, CipUdint type, CipUdint protocol)
{
        //Examples of parameters
        // family: AF_INET,PF_CAN
        // type: SOCK_STREAM,SOCK_RAW
        // protocol: IPPROTO_TCP,CAN_RAW

        sock = socket(family, type, protocol);
        socket_to_conn_map.emplace (sock, this);
        return sock;
}

int NET_Connection::SetSocketOpt(CipUdint type, CipUdint reuse, CipUdint val)
{
        this->type = type;
        this->reuse = reuse;
        this->val = val;
        return setsockopt ( sock, type, reuse, (char *) &val, sizeof (CipUdint));
}

int NET_Connection::BindSocket(int address_option, struct sockaddr * address)
{
    switch(address_option)
    {
        case kOriginatorAddress:
            originator_address = address;
            break;
        case kRemoteAddress:
            remote_address = address;
            break;
        default:
            return INVALID_INPUTS;
    }
    return bind( sock, address, sizeof(struct sockaddr));
}

int NET_Connection::Listen(int max_num_connections)
{
    return listen( sock, max_num_connections);
}

void NET_Connection::CloseSocket()
{
        OPENER_TRACE_INFO("networkhandler: closing socket %d\n", sock);
        if (-1 != sock)
        {
            //Check if socket is still registered
            if (socket_to_conn_map.find(sock) != socket_to_conn_map.end())
            {
                FD_CLR(sock, &select_set[kMasterSet]);
#ifdef WIN

                closesocket ( sock);
#elif __linux__
                shutdown(sock, SHUT_RDWR);
                close(sock);
#endif

                socket_to_conn_map.erase (sock);
            }
            //Set socket val to -1
            sock = -1;
        }
}

int NET_Connection::GetSocketHandle()
{
    return sock;
}

int NET_Connection::SetSocketHandle(int socket_handle)
{
    return sock = socket_handle;
}

int NET_Connection::SendData(void * data_ptr, CipUdint size)
{
    return send( sock, (char*)data_ptr, size, 0);
}

int NET_Connection::RecvData (void *data_ptr, CipUdint size)
{
    return recv( sock, (char*)data_ptr, size, 0);
}

int NET_Connection::SendDataTo(void * data_ptr, CipUdint size, struct sockaddr * destination)
{
    return sendto( sock, (char*)data_ptr, size, 0, destination, sizeof(sockaddr*));
}

int NET_Connection::RecvDataFrom (void *data_ptr, CipUdint size, struct sockaddr *source)
{
    //int size_sock = sizeof(sockaddr*);
    socklen_t socklen;
    return recvfrom( sock, (char*)data_ptr, size, 0, source, &socklen);
}

uint32_t NET_Connection::endian_htonl(uint32_t hostlong)
{
    return htonl(hostlong);
}

uint16_t NET_Connection::endian_htons(uint16_t hostshort)
{
    return htons(hostshort);
}

uint32_t NET_Connection::endian_ntohl(uint32_t netlong)
{
    return ntohl(netlong);
}

uint16_t NET_Connection::endian_ntohs(uint16_t netshort)
{
    return ntohs(netshort);
}
