// IocSocket.h : header file
// Implements the socket connection for the low level driver code

#pragma once

#include "HW_Emul.h"

template <class T>
struct Payload
{
    UINT32 header;
    UINT32 size;
    T      payload;
    UINT32 checksum;

    Payload( UINT32 header)
        : header( header)
        , checksum(0)
    {
        size = sizeof( T);
        Reset();
    }

    void Reset() {
        memset( &payload, 0, size);
    }
};

typedef Payload<G4O_Data> G4O_Payload;
typedef Payload<G4I_Data> G4I_Payload;

////////////////////////////////////////////////////////////////////////////////////////////////////
class IocSocket;

class IocSocket : public CAsyncSocket
{
public:
    enum IocSocketConstants { 
        eHeader         = 0x12345678,
        IOC_SOCKET_PORT = 50005,
    };

    enum SocketEnum {
        eServer = 0,
        eClient = 1,
        eServerSide = 2,
    };

    /// C-tor
    IocSocket(SocketEnum type);

    ~IocSocket() { Close(); }

    /// Server - setup as a server
    void GetConnection();

    /// Server - setup as a server
    bool Server();
    void KillClient();

    /// Client - try to connect as a client
    bool Client();

    /// Receive Data
    void RxData();

    /// Transmit Data
    void TxData();

    UINT32 Checksum( void* ptr, int size);

    // Respond to Connect callback
    virtual void OnAccept(int nErrorCode) {
        //TRACE("Accept %d %d\n", nErrorCode, m_socketType);
        if ( m_client != NULL)
        {
            delete m_client;
            Sleep(1000);
        }

        m_client = new IocSocket( eServerSide);

        if ( Accept( *m_client))
        {
            m_connected = (nErrorCode == 0);
            m_client->m_connected = m_connected;
            m_client->m_clientServer = this;
        }
    }

    // Respond to Connect callback
    virtual void OnConnect(int nErrorCode) { 
        //TRACE("Connect %d %d\n", nErrorCode, m_socketType);
        if (nErrorCode == 0) {
            m_connected = true;
        }
        else {
            m_connected = false;
        }
    }

    // Respond to data received callback
    virtual void OnReceive(int nErrorCode) { 
        RxData();
    }

    // respond to socket closed callback
    virtual void OnClose(int nErrorCode) { 
        //TRACE("Close %d %d %d\n", nErrorCode, m_socketType, clock());
        m_connected = (nErrorCode == 0);
        Reset();
        ShutDown();
    }

    // specialized shut down
    BOOL ShutDown()
    {
        int errNo = 0;
        BOOL sdr = CAsyncSocket::ShutDown(2);
        if (sdr == 0)
        {
            errNo = GetLastError();
            CAsyncSocket::ShutDown(2);
        }
        //TRACE("ShutDown %d Err: %d %d\n", m_socketType, errNo, clock());
        m_connected = false;
        return sdr;
    }

    bool IsConnected()
    {
        if (m_socketType == eServer)
        {
            if ( m_client != NULL)
            {
                m_connected = m_client->m_connected;
            }
            else
            {
                m_connected = false;
            }
        }
        return m_connected;
    }

public:
    SocketEnum m_socketType;
    bool m_connected;
    UINT m_port;
    IocSocket* m_client;
    IocSocket* m_clientServer;

    BUF_G4I m_g4i;
    BUF_G4O m_g4o;

private:
    void Reset();
};
