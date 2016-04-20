#include <stdio.h>



#include "BufferManager.h"

const int pageSize = 8000;
const int tableSize = 128;


BufferManager::BufferManager(unsigned pageCount_) {
	pageCount = pageCount_;
	buffer = malloc(pageCount * pageSize);
	//hashTable = new std::unordered_map<uint64_t, BufferFrame *>();

	hashTable[123] = new BufferFrame();
}

BufferFrame& BufferManager::fixPage(uint64_t pageId, bool exclusive) {

}

void BufferManager::unfixPage(BufferFrame& frame, bool isDirty) {

}

BufferManager::~BufferManager() {

}

#include "BufferManager.h"

int main() {
	printf("hi");
	return 1;
}