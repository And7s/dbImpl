
#pragma once
#include <iostream>
#include <string>
#include <unordered_map>
#include <mutex>
#include "BufferFrame.h"

class BufferManager {
private:
	unsigned pageCount;
	unsigned loadedPages;
	std::mutex table_mutex;
	std::unordered_map<uint64_t, BufferFrame *> hashTable;
public: 

	BufferManager(unsigned pageCount_);
	BufferFrame& fixPage(uint64_t pageId, bool exclusive);
	void unfixPage(BufferFrame& frame, bool isDirty);
	~BufferManager();
	void readFile(uint64_t pageId, void* buff);
	void writeFile(uint64_t pageId, void* buff);
	void freePage();
};
