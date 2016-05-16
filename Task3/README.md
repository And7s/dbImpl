# DataImpl
by Marius Guggenmos and Andreas Schmelz 

developed and tested under ubuntu 16.04

compile with make, this will generate a binary called test

usage example
./test

the input of pageSize has been removed. Its hardcoded (BufferFrame.h) You'll have to recompile in order to change the pageSize

The tests run in this configuration, but may fail with different pageSize/ random seed. I do not clearly understand the purpose of some of the tests, especially the one with the not exceeding the pageNumber.
Aditionally update will fail if the current page cannot hold the updated data.

It would be nice to get more precise instructions in the Assignment sheet.
