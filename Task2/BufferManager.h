
#pragma once
#include <iostream>
#include <string>
#include <unordered_map>
#include <mutex>





class BufferFrame {
public:
	void* data;
	int inUse = 0;
	bool isDirty = false;
	bool exclusive = false;
	uint64_t pageId;
	BufferFrame(uint64_t i);
	void* getData();
	void show();
	~BufferFrame();
};


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


