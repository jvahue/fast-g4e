#include "stdafx.h"
#include "direct.h"

#include "PersistFile.h"

PersistFile::PersistFile( int size)
    : CFile()
{
    m_size = size;
}

PersistFile::~PersistFile()
{
    if (IsOpen())
    {
        Flush();
        Close();
    }
}

void PersistFile::Init( const CString& name, BYTE resetValue, bool appendDat)
{
    UINT tryCount = 0;
    UINT flags = (CFile::modeCreate | CFile::modeNoTruncate | 
                  CFile::modeReadWrite | CFile::shareDenyWrite);

    if (appendDat)
    {
        m_filename.Format( "Persist\\%s.dat", name);
    }
    else
    {
        m_filename.Format( "Persist\\%s", name);
    }

    // make sure the subdir exists to hold this file
    _mkdir( _T("Persist"));

    while (!IsOpen() && tryCount < 3)
    {
        CFileException ex;
        if (Open( (LPCTSTR)m_filename, flags, &ex))
        {
            CFileStatus sts;
            int size = GetStatus(sts);
            if ( sts.m_size != m_size)
            {
                Reset( resetValue);
            }
            SeekToBegin();
        }
        else
        {
            tryCount++;
            Sleep(1000);
        }
    }
};

void PersistFile::Reset( BYTE resetValue)
{
    if ( IsOpen())
    {
        UINT32 written = 0;
        UINT32 workingSize = min( m_size, 128*1024);

        // fill it in with 0
        BYTE* data = new BYTE[workingSize];
        memset(data, resetValue, workingSize);

        SeekToBegin();
        while (written < m_size)
        {
            UINT32 thisWrite = min( workingSize, m_size-written);
            CFile::Write( data, thisWrite);
            written += thisWrite;
        }
        delete data;
        SeekToBegin();
    }
}