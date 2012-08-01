#pragma once

class PersistFile : public CFile
{
public:
    PersistFile( int size);
    virtual ~PersistFile();

    void Init( const CString& name, BYTE resetValue = 0, bool appendDat=true);
    void Reset( BYTE resetValue = 0);
    bool IsOpen() {return m_hFile != CFile::hFileNull;}

    UINT32 m_size;
    CString m_filename;
};