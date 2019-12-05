#pragma once

#include <vector>
#include <memory>
#include "MZDataIdentifier.h"

namespace MZDR
{
  struct TextPos
  {
    TextPos() {}
    TextPos(DWORD lineIdx, DWORD lineOffset)
      : nLine(lineIdx)
      , nLineOffset(lineOffset)
    {}

    DWORD nLine;
    DWORD nLineOffset; // in characters
  };

  struct TextRange
  {
    TextRange()
    {
    }
    TextRange(DWORD nStartLineIdx, DWORD nStartLineOffset, DWORD nEndLineIdx, DWORD nEndLineOffset)
      : start(nStartLineIdx, nStartLineOffset)
      , end(nEndLineIdx, nEndLineOffset)
    {

    }
    TextPos start;
    TextPos end;
  };

  enum NewLine
  {
    NoNewLine = 0,
    CR, // \r
    LF, // \n
    CRLF,
    Unknown,
  };

  template<typename T>
  class LineHelper
  {
  public:
    static std::unique_ptr<BYTE []> GetNewLineData(OUT int& len, MZDR::NewLine newLineType)
    {
      T szNewLine[8] = { 0 };

      GetNewLine(szNewLine, _countof(szNewLine), newLineType);

      DWORD charsForNewLine = 1;
      if (newLineType == MZDR::CRLF)
        charsForNewLine = 2;

      int dwNewLineLenInBytes = static_cast<int>(charsForNewLine*sizeof(T));

      auto pData = std::make_unique<BYTE []>(dwNewLineLenInBytes + 2);
      ZeroMemory(pData.get(), dwNewLineLenInBytes + 2);
      CopyMemory(pData.get(), reinterpret_cast<const BYTE*>(szNewLine), dwNewLineLenInBytes);
      len = dwNewLineLenInBytes;
      return pData;
    }
  protected:
    static void GetNewLine(char* sz, int len, MZDR::NewLine newLineType)
    {
      if (newLineType == MZDR::CR)
        strcpy_s(sz, len, "\r");
      else if (newLineType == MZDR::LF)
        strcpy_s(sz, len, "\n");
      else if (newLineType == MZDR::CRLF)
        strcpy_s(sz, len, "\r\n");
    }

    static void GetNewLine(wchar_t* sz, int len, MZDR::NewLine newLineType)
    {
      if (newLineType == MZDR::CR)
        wcscpy_s(sz, len, L"\r");
      else if (newLineType == MZDR::LF)
        wcscpy_s(sz, len, L"\n");
      else if (newLineType == MZDR::CRLF)
        wcscpy_s(sz, len, L"\r\n");
    }


  };

  template<class L>
  class LinesData
  {
  public:
    BYTE* AllocateBuffer(DWORD nSize)
    {
      auto spBuffer = std::make_unique<BYTE[]>(nSize);
      auto pBuffer = spBuffer.get();
      m_vBuffers.push_back(std::move(spBuffer));
      return pBuffer;
    }

    void InsertLine(const BYTE* pLine, DWORD lenBytes, NewLine newLineCharacters, BYTE numBytesForNewLine)
    {
      m_vItems.push_back(L(pLine, lenBytes, newLineCharacters, numBytesForNewLine));
    }

    void ReserveLines(size_t lines)
    {
      m_vItems.reserve(lines);
    }

    void ContentFormat(MZDR::ContentFormat format)
    {
      m_ContentFormat = format;
    }
    
    MZDR::ContentFormat ContentFormat()
    {
      return m_ContentFormat;
    }

    size_t NumLines() const { return m_vItems.size();  }

    template<typename T>
    std::unique_ptr<T[]> GetLinesAsText(const T* szNewLine, DWORD len) const
    {
      size_t total = TotalLineSize(len*sizeof(T)) + 4;
      auto spBuffer = std::make_unique<T[]>(total/sizeof(T));
      ZeroMemory(spBuffer.get(), total);

      BYTE* pPos = reinterpret_cast<BYTE*>(spBuffer.get());
      BYTE* pPosBegin = reinterpret_cast<BYTE*>(spBuffer.get());
      for (auto&& line : m_vItems)
      {
        if (pPos > pPosBegin)
        {
          CopyMemory(pPos, szNewLine, len*sizeof(T));
          pPos += len*sizeof(T);
        }

        CopyMemory(pPos, line.pLine, line.lenght);
        pPos += line.lenght;
      }
      return spBuffer;
    }

    const std::vector<L>& GetLines() { return m_vItems; }

    L* GetLine(size_t nIdx)
    {
      return &(m_vItems.at(nIdx));
    }

    const L* GetLine(size_t nIdx) const
    {
      return &(m_vItems.at(nIdx));
    }

    template<typename T>
    NewLine GetNewLineStyle(T)
    {
      NewLine newLineStyle = NoNewLine;

      for (auto&& line : m_vItems)
      {
        if (line.nBytesForNewLine == 0)
          continue;

        if (line.nBytesForNewLine == 1*sizeof(T) && *((T*)(line.pLine + line.lenght)) == '\n')
        {
          newLineStyle = LF;
          break;
        }
        if (line.nBytesForNewLine == 1*sizeof(T) && *((T*)(line.pLine + line.lenght)) == '\r')
        {
          newLineStyle = CR;
          break;
        }
        if (line.nBytesForNewLine == 2*sizeof(T) && *((T*)(line.pLine + line.lenght)) == '\r' && *((T*)(line.pLine + line.lenght+sizeof(T))) == '\n')
        {
          newLineStyle = CRLF;
          break;
        }
      }

      return newLineStyle;
    }

    size_t TotalLineSize(DWORD extraPerLine) const
    {
      size_t nTotalLength = 0;
      for (auto&& l : m_vItems)
      {
        nTotalLength += l.lenght + extraPerLine;
      }

      return nTotalLength;
    }

  protected:
    std::vector< std::unique_ptr<BYTE[]>> m_vBuffers;
    std::vector<L> m_vItems;

    MZDR::ContentFormat m_ContentFormat = MZDR::ContentUnknown;
  };


}