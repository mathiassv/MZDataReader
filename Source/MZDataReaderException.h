#pragma once

namespace MZDR
{
  struct MZDataReaderException : std::exception
  {
    MZDataReaderException(DWORD dwError, const char* szText)
      : std::exception(szText)
    {
      errorCode = dwError;
    }
    //char const* what() const throw();
    DWORD errorCode = 0;
  };

}