
#pragma once

#include "../../MZMisc/Source/AutoHandle.h"
#include "MZDataReaderException.h"
#include "MZLinesData.h"

namespace MZDR
{
  struct RangePos
  {
    DWORD nStart;
    DWORD nEnd;
  };


  // move ot MCStringutils.h
  class MCExtra
  {
  public:
    static std::wstring Format(const TCHAR* strFormat, ...)
    {
      TCHAR str[2048];
      str[0] = '\0';

      if (strFormat == NULL)
        return str;

      va_list vaList;
      va_start(vaList, strFormat);

      int nRetVal = _vsntprintf_s(str, _countof(str), _TRUNCATE, strFormat, vaList);

      va_end(vaList);

      if (nRetVal == -1)
      {
        str[0] = '\0';
        return str;
      }

      return str;
    }

    static bool GetUniqueFilename(STLString& filename, const STLString& strTail)
    {
      // no need to modify filename
      if (GetFileAttributes(filename.c_str()) == INVALID_FILE_ATTRIBUTES)
        return true;

      STLString strFileRoot = filename;

      STLString strPath = _T("");
      STLString strExt = _T("");
      for (int i = 1; i <= 9999; i++)
      {
        if (i > 1)
        {
          strExt = Format(_T("_(%04d)"), i);
        }

        strExt += strTail;

        strPath = strFileRoot;
        strPath += strExt;
        if (GetFileAttributes(strPath.c_str()) == INVALID_FILE_ATTRIBUTES)
        {
          filename = strPath;
          return true;
        }
      }

      return false;
    }

    static bool BackupFileEx(const TCHAR* filename, TCHAR* szNewName, DWORD len)
    {
      STLString strPath = filename;
      if (GetUniqueFilename(strPath, _T(".bak")))
      {
        if (szNewName)
          _tcsncpy_s(szNewName, len, strPath.c_str(), strPath.length());

        if (MoveFile(filename, strPath.c_str()))
          return true;
      }

      return false;
    }
  };

  class DataWriter
  {
  public:
    DataWriter() {};
    virtual ~DataWriter() {}

    virtual void Prepare(size_t /*dwExpectedDataSize*/) {};
    virtual void Close() {}

    void SetNewLineData(std::unique_ptr<BYTE []>& pData, DWORD lenData)
    {
      m_pNewLineData = std::move(pData);
      m_lenNewLineData = lenData;
    }

    virtual DWORD WriteNewLine()
    {
      DWORD dwBytesWritten = 0;
      if (m_lenNewLineData > 0)
      {
        WriteData(m_pNewLineData.get(), m_lenNewLineData, &dwBytesWritten);
      }
      return dwBytesWritten;
    }

    virtual DWORD WriteData(const BYTE* pData, DWORD lenData)
    {
      DWORD dwBytesWritten = 0;
      if (lenData > 0)
      {
        WriteData(pData, lenData, &dwBytesWritten);
      }
      return dwBytesWritten;
    }
  protected:
    std::unique_ptr<BYTE []> m_pNewLineData;
    DWORD m_lenNewLineData = 0;

    virtual void WriteData(const BYTE* pBuffer, DWORD dwBytesToWrite, DWORD* dwBytesWritten) = 0;
  };

  class FileDataWriter : public DataWriter
  {
  public:
    FileDataWriter()
    {

    }

    void OpenForWriting(const STLString& filename, bool bOverwrite)
    {
      
      DWORD fileOpenMode = bOverwrite ? CREATE_ALWAYS : CREATE_NEW;

      m_hFile = AutoHandle(::CreateFile(filename.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, fileOpenMode, 0, 0));
      if (m_hFile.isValid() == false)
      {
        USES_CONVERSION;
        STL_string str = "Unable to open file for writing : ";
        str += W2CA(filename.c_str());

        throw MZDataReaderException(::GetLastError(), str.c_str());
      }
    }
    
  protected:
    AutoHandle m_hFile;
    void WriteData(const BYTE* pBuffer, DWORD dwBytesToWrite, DWORD* dwBytesWritten)
    {
      if (WriteFile(m_hFile, pBuffer, dwBytesToWrite, dwBytesWritten, nullptr) == FALSE)
      {
        throw MZDataReaderException(::GetLastError(), "Failed to write data to file");
      }
    }
  };

  template<typename T>
  class MemoryDataWriter : public DataWriter
  { 
  public:
    MemoryDataWriter(size_t memSize)
    {
      m_pData = std::make_unique<T[]>(memSize/sizeof(T));
      ZeroMemory(m_pData.get(), memSize);
      m_pCurPos = reinterpret_cast<BYTE*>(m_pData.get());
      m_pEndPos = reinterpret_cast<BYTE*>(m_pCurPos + memSize);
    }

    size_t Size() 
    { 
      if (m_pData == nullptr || m_pCurPos == nullptr)
        return 0;

      return m_pCurPos - m_pData.get();
    }
    std::unique_ptr<T[]> StealData() { return std::move(m_pData); }


  protected:
    std::unique_ptr<T[]> m_pData;
    BYTE* m_pEndPos;
    BYTE* m_pCurPos;

    DWORD m_nCurrentLine = 0;
    DWORD m_nCurLinePos = 0;

    DWORD WriteNewLine() override
    {
      auto r = __super::WriteNewLine();
      ++m_nCurrentLine;
      m_nCurLinePos = 0;
      return r;
    }

    void WriteData(const BYTE* pBuffer, DWORD dwBytesToWrite, DWORD* dwBytesWritten) override
    {
      if (m_pCurPos + dwBytesToWrite > m_pEndPos)
      {
        dwBytesToWrite = static_cast<DWORD>(m_pEndPos - m_pCurPos); // makesure we do not write pass the end
      }

      CopyMemory(m_pCurPos, pBuffer, dwBytesToWrite);
      if (dwBytesWritten)
        *dwBytesWritten = dwBytesToWrite;

      m_pCurPos += dwBytesToWrite;
    }
  
  };


  class LineDataWriter
  {
  public:
    
    template<class LineData>
    static void WriteLinesToFile(const STLString& filename, LineData& pData, bool bOverwrite, const BYTE* pNewLine = nullptr, DWORD dwNewLineLen = 0)
    {
      DWORD fileOpenMode = bOverwrite ? CREATE_ALWAYS : CREATE_NEW;

      AutoHandle hFile(::CreateFile(filename.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, fileOpenMode, 0, 0));
      if (hFile.isValid() == false)
      {
        throw MZDataReaderException(::GetLastError(), "Unable to open file for writing");
      }

      const DWORD buffersize = 32 * 1024;

      auto spBuffer = std::make_unique<BYTE []>(buffersize);
      DWORD availBufferSize = buffersize;
      BYTE* pBufferPos = spBuffer.get();

      auto& vLines = pData->GetLines();

      for (auto&& line : vLines)
      {
        if (line.GetLineData() == nullptr)
          continue;

        DWORD len = line.GetLineDataLength();

        if (len > availBufferSize)
        {
          // Write current buffer and continue
          WriteFileThrow(hFile, spBuffer.get(), buffersize - availBufferSize);
          availBufferSize = buffersize;
          pBufferPos = spBuffer.get();
        }

        CopyMemory(pBufferPos, line.GetLineData(), len);
        pBufferPos += len;

        assert(availBufferSize >= len);
        availBufferSize -= len;

        if (pNewLine)
        {
          if (line.nBytesForNewLine == 0 && dwNewLineLen < availBufferSize)
          {
            if (dwNewLineLen > availBufferSize)
            {
              // Write current buffer and continue
              WriteFileThrow(hFile, spBuffer.get(), buffersize - availBufferSize);
              availBufferSize = buffersize;
              pBufferPos = spBuffer.get();
            }

            CopyMemory(pBufferPos, pNewLine, dwNewLineLen);
            pBufferPos += dwNewLineLen;
            availBufferSize -= dwNewLineLen;
          }
        }
      }

      if (availBufferSize != buffersize)
      {
        WriteFileThrow(hFile, spBuffer.get(), buffersize - availBufferSize);
      }
    }

  protected:
    static void WriteFileThrow(HANDLE hFile, const BYTE* pBuffer, DWORD dwBytesToWrite)
    {
      DWORD dwBytesWritten;
      if (WriteFile(hFile, pBuffer, dwBytesToWrite, &dwBytesWritten, nullptr) == FALSE)
      {
        throw MZDataReaderException(::GetLastError(), "Failed to write data to file");
      }
    }   
  };

}