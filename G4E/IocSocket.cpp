// SteSocket.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "stddef.h"

#include "IocSocket.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
template <class T>
CircularBuffer<T>::CircularBuffer()
{
    Reset();
}

//--------------------------------------------------------------------------------------------------
template <class T>
void CircularBuffer<T>::Reset()
{
    for (int i=0; i < eSize; ++i)
    {
        memset( (void*)&buffer[i], 0, sizeof(T));
    }
    readCount = 0;
    readFrom = 0;
    writeCount = 0;
    writeTo = 0;
    maxDepth = 0;
    writeActive = false;
    readActive = false;
    name = "IOC-G4E";
}

//--------------------------------------------------------------------------------------------------
template <class T>
void CircularBuffer<T>::Write( const T& data)
{
    ASSERT(!writeActive);
    writeActive = true;

    int depth = writeCount - readCount;
    //ASSERT(depth < eSize);
    if (depth > maxDepth)
    {
        maxDepth = depth;
        if (maxDepth > 5)
        {
            //TRACE("MaxDepth(%s)%d: %d\n", (LPCTSTR)name, clock(), maxDepth);
        }
    }
    else if (depth == 0)
    {
        maxDepth = 0;
    }

    if (depth < eSize)
    {
        buffer[writeTo] = data;
        ++writeCount;
        if (++writeTo >= eSize)
        {
            writeTo = 0;
        }
    }
    else
    {
        TRACE("Write overflow: %s at %d\n", (LPCTSTR)name, clock());
    }

    writeActive = false;
}

//--------------------------------------------------------------------------------------------------
template <class T>
bool CircularBuffer<T>::Read(T& data)
{
    bool status = false;

    ASSERT(!readActive);
    readActive = true;

    if (writeCount > readCount)
    {
        data = buffer[readFrom];
        status = true;

        ++readCount;
        if ( ++readFrom >= eSize)
        {
            readFrom = 0;
        }
    }
    readActive = false;

    return status;
}

//--------------------------------------------------------------------------------------------------
IocSocket::IocSocket(SocketEnum type)
    : CAsyncSocket()
    , m_connected(false)
    , m_port( IOC_SOCKET_PORT)
    , m_client( NULL)
    , m_clientServer( NULL)
{
    m_socketType = type;
}

//--------------------------------------------------------------------------------------------------
void IocSocket::GetConnection()
{
    if ( m_socketType == eServer)
    {
        Server();
    }
    else if ( m_socketType == eClient)
    {
        Client();
    }
    Reset();
}

//--------------------------------------------------------------------------------------------------
void IocSocket::Reset(void)
{
    m_g4i.Reset();
    m_g4o.Reset();
}

//--------------------------------------------------------------------------------------------------
bool IocSocket::Server()
{
    BOOL result;

    // ensure its closed
    CAsyncSocket::Close();

    result = Create(m_port);

    result = Listen( 5);

    if (result == 0)
    {
        int err = GetLastError();
    }

    return m_connected;
}

// close the connection to the current client if you are the server
void IocSocket::KillClient()
{
    if ( m_socketType == eServer)
    {
        TRACE("KILL Client %d %d\n", m_socketType, clock());
        delete m_client;
        m_client = NULL;
        m_connected = false;
    }
}

//--------------------------------------------------------------------------------------------------
bool IocSocket::Client()
{
    BOOL result;

    // ensure its closed
    CAsyncSocket::Close();

    result = Create();

    result = Connect( _T("localhost"), m_port);

    if (result == 0)
    {
        int err = GetLastError();
    }

    return m_connected;
}

//--------------------------------------------------------------------------------------------------
void IocSocket::RxData()
{
    int rxed;
    //TRACE("Ioc Rx Data ");
    if ( m_socketType == eServerSide)
    {
        G4O_Payload g4o(eHeader);
        UINT32 size = sizeof(g4o);
        // receive G4O
        rxed = Receive( &g4o, size);
        if (rxed != SOCKET_ERROR)
        {
            if ( g4o.header == eHeader && Checksum( &g4o, size) == g4o.checksum)
            {
                m_g4o.Write( g4o.payload);
                //TRACE("%2d %d %4d\n", m_g4o.cmdRequest, m_g4o.cmdRqstCount, m_g4o.cmdRqstHandled);
            }
        }
        else
        {
            int errNo = GetLastError();
            //TRACE("Errno: %d %d\n", errNo, clock());
            m_clientServer->KillClient();
        }
    }
    else if ( m_socketType == eClient)
    {
        G4I_Payload g4i(eHeader);
        UINT32 size = sizeof( g4i);
        // receive G4I
        rxed = Receive( &g4i, size);
        if (rxed != SOCKET_ERROR)
        {
            if ( g4i.header == eHeader && Checksum( &g4i, size) == g4i.checksum)
            {
                m_g4i.Write( g4i.payload);
                //TRACE("%2d %4d %4d\n", m_g4i.cmdRequest, m_g4i.cmdRqstCount, m_g4i.cmdRqstHandled);
            }
        }
        else
        {
            int errNo = GetLastError();
            //TRACE( "RxData Err(%d)\n", errNo);
        }
    }
    else
    {
        ASSERT(0);
    }
}

//--------------------------------------------------------------------------------------------------
void IocSocket::TxData()
{
    //TRACE("Ioc Tx Data ");
    if ( m_socketType == eServerSide)
    {
        G4I_Payload g4i(eHeader);
        int size = sizeof( g4i);
        if (m_g4i.Read(g4i.payload))
        {
            g4i.checksum = 0;
            g4i.checksum = Checksum( &g4i, size);
            int sent = 0;
            while (sent < size)
            {
                int bytes = Send( &g4i, size);
                if (bytes != SOCKET_ERROR)
                {
                    sent += bytes;
                }
                else
                {
                    int err = GetLastError();
                    //TRACE("TxSend Err(%d)\n", err);
                    if ( err == 10053 || err == 10054)
                    {
                        // exit connection lost
                        sent = size;
                    }
                }
            }
        }
        //TRACE("%2d %4d %4d\n", m_g4i.cmdRequest, m_g4i.cmdRqstCount, m_g4i.cmdRqstHandled);
    }
    else if ( m_socketType == eClient)
    {
        G4O_Payload g4o(eHeader);
        int size = sizeof( g4o);
        if ( m_g4o.Read(g4o.payload))
        {
            g4o.checksum = 0;
            g4o.checksum = Checksum( &g4o, size);
            int sent = Send( &g4o, size);
            //ASSERT( sent == size);
        }
        //TRACE("%2d %4d %4d\n", m_g4o.cmdRequest, m_g4o.cmdRqstCount, m_g4o.cmdRqstHandled);
    }
    else
    {
        ASSERT(0);
    }
}

//--------------------------------------------------------------------------------------------------
UINT32 IocSocket::Checksum( void* ptr, int size)
{
    UINT32 checksum = 0;
    unsigned char* buffer = (unsigned char*)ptr;

    // exclude the checksum
    for ( int i = 0; i < size-4; ++i)
    {
        checksum += buffer[i];
    }
    return checksum;
}
