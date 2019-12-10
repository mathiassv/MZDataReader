# MZDataReader
Helper C++ class for handling reading and writing of data easier.. 
Will throw exception ( MZDR::MZDataReaderException ) on error

## Classes

* FileDataReader<br/>
Class for reading data from file
<br/><br/>
* MemoryDataReaderLineDataWriter<br/>
Class for reading data from a memory buffer
<br/><br/>
* FileDataWriter<br/>
Class for writing to a file
<br/><br/>
* WriteLinesToFile<br/>
Class for writing a collection of lines to a file
<br/><br/>
* MemoryDataWriter<br/>
Class for writing data to memory buffert
<br/><br/>
* LineReader<br/>
Class for reading lines from a buffer or from a DataReader (see class above)
<br/><br/>
* LineDataWriter<br/>
Class for writing lines to file
* DataIdentifier<br/>
Static class that will identify what kind of dataformat it is. Binary or Text (Unicode, UTF8, Ascii)

# Example
See the MZSortLines repo for example of usage

# History
v1.0 - 2019-11-05<br/>
First public version
