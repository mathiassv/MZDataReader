#pragma once

#include "MZLinesData.h"

namespace MZDR
{
  enum NewLine;
    // T MUST be char or wchar_t
  struct ParseLineResult
  {
    void Clear()
    {
      pLine = nullptr;
      pNextLine = nullptr;
      length = 0;
      nCharsForNewLine = 0;
      bEndOfDataReached = false;
    }

    const BYTE* pLine;
    const BYTE* pNextLine;
    DWORD length;
    BYTE nCharsForNewLine; // 0,1,2  - 2 if newline was two characters. (CRLF)
    NewLine newLineChars;
    bool bEndOfDataReached;

  };
  
  template<class T>
  class LineParserT
  {
  public:
    static ParseLineResult ParseLine(const T* pBegin, const T* pEndOfData)
      {
        ParseLineResult result;

//        const T*  pLineBegin = pBegin;
        const T*  pLineEnd = nullptr;
//        DWORD   dwLineLength = 0;
//        short   dwLineFlags = 0;

        T* pData = (T*) pBegin;

        if (pData >= pEndOfData)
        {
          result.Clear();
          result.bEndOfDataReached = true;
          return result;
        }

        // Find CR or LF character
        while (pData < pEndOfData && *pData != 0x0a && *pData != 0x0d)
        {
          pData++;
        }

        // if pData == pDataEnd. then we reach the end of the buffert. 
        //  find out if we at the EOD. if we are then it is okey. else we need to stop
        if (pData == pEndOfData)
          result.bEndOfDataReached = true;
        else
          result.bEndOfDataReached = false;

        pLineEnd = pData;

        if (pData < pEndOfData && pData <= pEndOfData)
        {
          if (*pData == 0x0d && *(pData + 1) == 0x0a)
          {
            pData += 2;
            result.nCharsForNewLine = 2;
            result.newLineChars = MZDR::CRLF;
          }
          else if (*pData == 0x0a)
          {
            pData++;
            result.nCharsForNewLine = 1;
            result.newLineChars = LF;
          }
          else
          {
            result.nCharsForNewLine = 0;
            result.newLineChars = NoNewLine;
          }
        }
        else
        {
          result.nCharsForNewLine = 0;
          result.newLineChars = NoNewLine;
        }

        // pData is still pointing at same char. can't be right..
        if (pLineEnd == pData && pData < pEndOfData)
          pData++;

        result.pLine = reinterpret_cast<const BYTE*>(pBegin);
        result.pNextLine = reinterpret_cast<const BYTE*>(pData);
        result.length = (DWORD) ((pLineEnd - pBegin)*sizeof(T));
        return result;
      }

  };

  class LineParser
  {
    public:
      ParseLineResult ParseLine(const char* pBegin, const char* pEndOfData)
      {
        return LineParserT<char>::ParseLine(pBegin, pEndOfData);
      }
      ParseLineResult ParseLine(const wchar_t* pBegin, const wchar_t* pEndOfData)
      {
        return LineParserT<wchar_t>::ParseLine(pBegin, pEndOfData);
      }
  };

}
