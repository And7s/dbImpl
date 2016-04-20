
#pragma once
#include <iostream>
#include <string>
#include <unordered_map>






class BufferFrame {
public:
	void* getData();
};


class BufferManager {
private:
	unsigned pageCount;
	void* buffer;
	std::unordered_map<uint64_t, BufferFrame *> hashTable;
public: 

	
	BufferManager(unsigned pageCount_);

	BufferFrame& fixPage(uint64_t pageId, bool exclusive);

	void unfixPage(BufferFrame& frame, bool isDirty);

	~BufferManager();

};


