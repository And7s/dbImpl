# DataImpl
by Marius Guggenmos and Andreas Schmelz 

developed and tested under ubuntu 15.10

compile with make, this will generate a binary called test

usage example
./test 150 10 2

the inputfile must exist and hold at least as many pages as accessed.
The code currently defines a pageSize to be 1024 bytes (defined in BufferFrame.h). Here: 150 * 1024 
bytes = 120kb would be necessary to be in a file called "0". The 64bit pageId
is split at the middle into segmentId (first 32bit) and page last 32 bit.
Eg. pageId 0x00000003000000C8 is segment 3, page 200. This would expect a file named "3" with at least 201kb.
