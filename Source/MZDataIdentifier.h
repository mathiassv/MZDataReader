#pragma once

#include <WinBase.h>
#include <memory>
#include "MZUtilsExceptions.h"
#include "../../MZMisc/Source/AutoHandle.h"

namespace MZDR
{
  enum ContentFormat
  {
    ContentUnknown = 0,
    ContentAscii = 1,
    ContentUnicode,
    ContentUTF8,
    ContentBinary,
  };

  class DataIdentifier
  {
  public:
    static ContentFormat GetContentFormat(const STLString& filename)
    {
      const DWORD dataLen = 1024;
      DWORD len = 0;
      auto pData = GetSampleData(filename, dataLen, &len);

      if (HasUnicodeFileHeader(pData.get(), len) || IsUnicodeFile(pData.get(), len))
        return ContentUnicode;

      if (HasUTF8FileHeader(pData.get(), len))
        return ContentUTF8; // UTF8 not supported. But as lines goes. it is ascii compatible.. (sorting might be wrong)

      if (IsBinary(pData.get(), len))
        return ContentBinary;

      return ContentAscii;
    }


    static  std::unique_ptr<BYTE []> GetSampleData(const STLString& filename, DWORD sampleSize, DWORD* pDataRead = nullptr)
    {
      if (::GetFileAttributes(filename.c_str()) == INVALID_FILE_ATTRIBUTES)
        throw MZDR::MZDataReaderException(ERROR_FILE_NOT_FOUND, "File not found");

      AutoHandle hFile(::CreateFile(filename.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0));
      if (hFile.isValid() == false)
      {
        throw MZDR::MZDataReaderException(::GetLastError(), "Unable to open file");
      }

      auto pBuffer = std::make_unique<BYTE []>(sampleSize);
      DWORD dwBytesRead = 0;
      if (::ReadFile(hFile, pBuffer.get(), sampleSize, &dwBytesRead, nullptr) == FALSE)
        throw MZDR::MZDataReaderException(::GetLastError(), "Unable to read file content");

      if (pDataRead)
        *pDataRead = dwBytesRead;

      return pBuffer;
    }


    static bool HasUnicodeFileHeader(const BYTE* pData, DWORD nLen)
    {
      if (nLen < 2)
        return false;

      if (*pData == 0xFF && *(pData + 1) == 0xFE)
        return true;

      return false;
    }

    static  bool HasUTF8FileHeader(const BYTE* pData, DWORD nLen)
    {
      if (nLen < 3)
        return false;

      if (*pData == 0xEF && *(pData + 1) == 0xBB && *(pData + 2) == 0xBF)
        return true;

      if (HasXMLUTF8Header(pData, nLen))
        return true;

      return false;
    }

    static  bool HasXMLUTF8Header(const BYTE* pData, DWORD nLen)
    {
      if (nLen < 6)
        return false;

      const char* pxmlData = reinterpret_cast<const char*>(pData);
      if (strncmp(pxmlData, "<?xml ", 6) == 0)
      {
        // find line end
        const char* pEndOfData = reinterpret_cast<const char*>(pData+nLen);
        const char* pLineEnd = (const char*) pData;
        while (*pLineEnd != '\n' && *pLineEnd != '\0')
        {
          pLineEnd++;
          // makesure pLineEnd do not go past endofdata
          if (pLineEnd > pEndOfData)
            return false;
        }

        // if lineend is null then end line was not found
        if (*pLineEnd != '\0')
        {
          char szLine[1024] = { 0 };
          strncpy_s(szLine, _countof(szLine), (const char*) pData, (DWORD) (pLineEnd - (const char*) pData));
          if (strstr(szLine, "UTF-8"))
          {
            return true;
          }
        }
      }

      return false;
    }

    static bool IsUnicodeFile(const BYTE* pData, DWORD nLen)
    {
      if (nLen < 10)
        return false;

      // every uneven letter us NULL. then it is unicode. However. It can be unicode even if this fail. depending on language of the file
      if (pData[1] == '\0' && pData[3] == '\0' && pData[5] == '\0' && pData[7] == '\0' && pData[9] == '\0')
      {
        if (pData[0] != '\0' && pData[2] != '\0' && pData[4] != '\0' && pData[6] != '\0' && pData[8] != '\0')
          return true;
      }
      return false;
    }


    static bool IsValidTextCharacter(unsigned char ch)
    {
      // Normal ASCII
      if (ch >= 0x20 && ch <= 0x7E)
        return true;

      // Selected extended ASCII char , like for ASCII Drawing..
      if (ch >= 0xb0 && ch <= 0xDF)
        return true;

      // TAB		,   CR          ,   LF
      if (ch == 0x09 || ch == 0x0D || ch == 0x0A)
        return true;

      return false;
    }

    static bool IsBinary(const BYTE* pData, DWORD nMaxLen)
    {
      DWORD nCount_ValidTextCharacters = 0;  // 
      DWORD nCount_NotValidTextCharacters = 0;
      CHAR* pStart = (CHAR*) pData;
      for (DWORD i = 0; i < nMaxLen; i++, pStart++)
      {
        if (IsValidTextCharacter((UCHAR) (*pStart)))
          nCount_ValidTextCharacters++;
        else
        {
          nCount_NotValidTextCharacters++;
        }

      }
      // if more the 20% is NoneAscii then its binary
      return (nCount_NotValidTextCharacters > (DWORD) (nMaxLen * 0.20) ? true : false);
    }

  };



}

