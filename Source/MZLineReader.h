#pragma once
#include <vector>
#include <memory>

#include "../../MZDataReader/Source/MZLineReader.h"
#include "../../MZDataReader/Source/MZLineParser.h"


namespace MZDR
{
  class DataReader;
  
  template<class T, class TLinesData>
  class LineReaderT
  {
    public:

      std::shared_ptr<TLinesData> ReadLinesFromBuffert(const BYTE* pData, size_t buffLen, MZDR::LineParser* pLineParser)
      {
        auto pLinesData = std::make_shared<TLinesData>();
        pLinesData->ReserveLines(buffLen / 60); // Assumes 60 char average per line

        auto pBuffer = pLinesData->AllocateBuffer(m_ChunkSize);
        CopyMemory(pBuffer, pData, buffLen);


        auto result = ParseBuffert(pLinesData, pLineParser, pBuffer, pBuffer + buffLen, true);
        assert(result.bEndOfDataReached);

        return pLinesData;
      }

      std::shared_ptr<TLinesData> ReadLinesFromDataReader(MZDR::DataReader* pReader, MZDR::LineParser* pLineParser, MZDR::ContentFormat format = MZDR::ContentUnknown)
      {
        size_t nLeftToRead = pReader->TotalDataSize();

        auto pLinesData = std::make_shared<TLinesData>();
        pLinesData->ReserveLines(nLeftToRead / 60); // Assumes 60 char average per line
        pLinesData->ContentFormat(format);

        auto pBuffer = pLinesData->AllocateBuffer(m_ChunkSize);
        DWORD nOffset = 0;

        while (nLeftToRead)
        {
          DWORD dwBytesRead = 0;

          pReader->ReadDataThrow(pBuffer + nOffset, m_ChunkSize - nOffset, &dwBytesRead);

          nLeftToRead -= dwBytesRead;
          bool bLastChunk = nLeftToRead <= 0;

          auto result = ParseBuffert(pLinesData, pLineParser, pBuffer, pBuffer + nOffset + dwBytesRead, bLastChunk);
          if (result.bEndOfDataReached && bLastChunk == false)
          {
            pBuffer = pLinesData->AllocateBuffer(m_ChunkSize);
            CopyMemory(pBuffer, result.pLine, result.length);
            nOffset = result.length;
          }

        } // while read chunks

        return pLinesData;
      }

    protected:
      MZDR::ParseLineResult ParseBuffert(std::shared_ptr<TLinesData>& spLinesData, MZDR::LineParser* pLineParser, const BYTE* pBuffer, const BYTE* pEnd, bool bLastChunk)
      {
        const BYTE* pLineStart = pBuffer;
        const BYTE* pEndOfData = pEnd;

        MZDR::ParseLineResult parseResult;
        parseResult.Clear();

        while (pLineStart)
        {
          parseResult = ParseLine(pLineParser, pLineStart, pEndOfData);
          if (parseResult.pLine)
          {
            if (parseResult.bEndOfDataReached && bLastChunk == false)
            {
              pLineStart = nullptr; // no more line to parse in this buffert
              return parseResult;
            }
            else
            {
              spLinesData->InsertLine(parseResult.pLine, parseResult.length, parseResult.newLineChars, parseResult.nCharsForNewLine*sizeof(T));
              pLineStart = parseResult.pNextLine;
            }
          }
          else
          {
            pLineStart = nullptr;
          }

        } // while parse chunk
        return parseResult;

      }
     
      MZDR::ParseLineResult ParseLine(MZDR::LineParser* pLineParser, const BYTE* pLineStart, const BYTE* pEndOfData)
      {
        return pLineParser->ParseLine(reinterpret_cast<const T*>(pLineStart), reinterpret_cast<const T*>(pEndOfData));
      }

      STLString m_strFilename;
      DWORD m_ChunkSize = 32*1024; // 256kb
  };

}
