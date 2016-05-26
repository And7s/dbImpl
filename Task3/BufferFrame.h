#pragma once
#include <iostream>
#define pageSize 16024

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