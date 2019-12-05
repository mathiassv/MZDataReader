#pragma once

#include "../../MZMisc/Source/AutoHandle.h"

#include "MZDataReader.h"
#include "MZDataReaderException.h"

#ifndef STL_string
#define STL_string std::string
#ifndef _UNICODE
typedef STL_string	 STLString;
#endif
#endif

#ifndef STL_wstring
#define STL_wstring std::wstring
#ifdef _UNICODE
typedef STL_wstring  STLString;
#endif
#endif

namespace MZDR
{
  //================================
  // Data reader base class
  //
  //================================
  
  class DataReader
  {
  public:
    size_t TotalDataSize() { return m_nTotalDataSize; }
    virtual void ReadDataThrow(BYTE* pBuffer, DWORD dwBytesToRead, DWORD* dwBytesRead) = 0;
    virtual void Close() {}
  protected:
    size_t m_nTotalDataSize = 0;

  };

  class FileDataReader : public DataReader
  {
  public:
    FileDataReader(const STLString& filename)
    {
      m_hFile = AutoHandle(::CreateFile(filename.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0));
      if (m_hFile.isValid() == false)
      {
        USES_CONVERSION;
        STL_string str = "Unable to open file : ";
        str += W2CA(filename.c_str());

        throw MZDR::MZDataReaderException(::GetLastError(), str.c_str());
      }

      LARGE_INTEGER fileSize = { 0 };
      if (::GetFileSizeEx(m_hFile, &fileSize) == FALSE)
      {
        USES_CONVERSION;
        STL_string str = "Failed to get filesize : ";
        str += W2CA(filename.c_str());

        throw MZDR::MZDataReaderException(::GetLastError(), str.c_str());
      }

      m_nTotalDataSize = static_cast<size_t>(fileSize.QuadPart);
    }
    
    void ReadDataThrow(BYTE* pBuffer, DWORD dwBytesToRead, DWORD* dwBytesRead) override
    {
      if (ReadFile(m_hFile, pBuffer, dwBytesToRead, dwBytesRead, nullptr) == FALSE)
      {
        throw MZDR::MZDataReaderException(::GetLastError(), "Failed to read file");
      }

      //  assert(*dwBytesRead == dwBytesToRead);
    }
    void Close() override
    {
      m_hFile.Release();
    }

  protected:
    AutoHandle m_hFile;
  };
 
  // ============================================================================

  class MemoryDataReader : public DataReader
  {
  public:
    /////////////////////////////////////////////////////////////////
    MemoryDataReader(const BYTE* pData, size_t dataLen, bool bTakeOwnership = false)
    {
      if (bTakeOwnership)
        m_bFreeMemory = true;

      m_pData = pData;
      m_nTotalDataSize = dataLen;
    }
    ~MemoryDataReader()
    {
      if (m_bFreeMemory)
      {
        delete[] m_pData;
      }
    }

    void ReadDataThrow(BYTE* pBuffer, DWORD dwBytesToRead, DWORD* dwBytesRead) override
    {
      DWORD dwBytesToCopy = 0;
      if (m_nCurPos + dwBytesToRead > m_nTotalDataSize)
        dwBytesToCopy = static_cast<DWORD>(m_nTotalDataSize) - m_nCurPos;
      else
        dwBytesToCopy = dwBytesToRead;

      CopyMemory(pBuffer, m_pData + m_nCurPos, dwBytesToCopy);

      m_nCurPos += dwBytesToCopy;
      *dwBytesRead = dwBytesToCopy;
    }

  protected:
    bool m_bFreeMemory = false;
    const BYTE* m_pData = nullptr;
    DWORD m_nCurPos = 0;
  };

}