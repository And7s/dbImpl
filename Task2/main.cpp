#include <stdio.h>

#include <iostream>

#include "BufferManager.h"


#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <algorithm>
#include <iostream>



const int pageSize = 8;
const int tableSize = 128;



using namespace std;

struct HexCharStruct {
  unsigned char c;
  HexCharStruct(unsigned char _c) : c(_c) { }
};

inline std::ostream& operator<<(std::ostream& o, const HexCharStruct& hs) {
  return (o << std::hex << (int)hs.c);
}

inline HexCharStruct hex(unsigned char _c) {
  return HexCharStruct(_c);
}



BufferFrame::BufferFrame(uint64_t id) {
	cout << "create new bufferframe with i "<< id << endl;
	cout << "in constr "<<inUse<<"\n";
	data = malloc(tableSize);
	pageId = id;
}
void BufferFrame::show() {
	cout << "BF: " << pageId << " inUse :" << inUse << " isDirty: " <<isDirty << endl;
	for (int i = 0; i < pageSize; i++) {
		cout << hex(((char *)data)[i]) << " ";
	}

	cout << endl;
}

void* BufferFrame::getData() {
	return data;
}

BufferManager::BufferManager(unsigned pageCount_) {
	cout << "create BufferManager with pages " << pageCount << endl;
	pageCount = pageCount_;	// max pages in memory
	loadedPages = 0;	// how many pages are curently loaded
	
}
BufferFrame::~BufferFrame() {
	cout << "buffer frame deconstruct" << pageId <<endl;
	free(data);
}


BufferFrame& BufferManager::fixPage(uint64_t pageId, bool exclusive) {
	// this page can 1. be in memory, 2. be on disk
	cout << "is: "<< (hashTable[pageId] == NULL) << endl;
	cout << "loaded "<< loadedPages << "pags";

	if (hashTable[pageId] == NULL) {	// data is not in memory
		loadedPages++;
		BufferFrame* bf = new BufferFrame(pageId);
		readFile(pageId, bf->data);
		bf->show();
		hashTable[pageId] = bf;
	} else {
		cout << "have already " << pageId <<endl;
	}
	
	hashTable[pageId]->inUse++;

	// print the content
	hashTable[pageId]->show();
	return *hashTable[pageId];

}

void BufferManager::unfixPage(BufferFrame& frame, bool isDirty) {
	cout << "will unfix " << endl;
	
	// force strategy
	if (isDirty) {
		writeFile(frame.pageId, frame.data);
	}
	frame.inUse--;	

	frame.show();
}

void BufferManager::readFile(uint64_t pageId, void* buff) {
	cout << "got page "<<pageId;
	uint64_t segment = (pageId  & 0xFFFFFFFF00000000) >> 32;
	uint64_t page = (pageId & 0x00000000FFFFFFFF);
	cout << "segment is "<<segment << " page " << page << " "<< endl;
	
	int fd;
	if ((fd = open(to_string(segment).c_str(), O_RDWR)) < 0) {
		std::cerr << "cannot open file" << strerror(errno) << std::endl;
	}
	if (lseek(fd, page * pageSize	, SEEK_SET) < 0) {
		cerr << "error lseek  " << strerror(errno) << std::endl;
	}
	read(fd, buff, pageSize);
	close(fd);
}

void BufferManager::writeFile(uint64_t pageId, void* buff) {
	cout << "write page "<<pageId;
	uint64_t segment = (pageId  & 0xFFFFFFFF00000000) >> 32;
	uint64_t page = (pageId & 0x00000000FFFFFFFF);
	cout << "segment is "<<segment << " page " << page << " "<< endl;
	
	int fd;
	if ((fd = open(to_string(segment).c_str(), O_RDWR)) < 0) {
		std::cerr << "cannot open file" << strerror(errno) << std::endl;
	}
	if (lseek(fd, page * pageSize	, SEEK_SET) < 0) {
		cerr << "error lseek  " << strerror(errno) << std::endl;
	}
	write(fd, buff, pageSize);
	close(fd);
}

BufferManager::~BufferManager() {
	cout << "deconstuct buffer manager" << endl;
	for (auto it = hashTable.begin(); it != hashTable.end(); ) {
		
		cout <<"callindg to deconstruct "<<endl;
		it->second->show();
		BufferFrame bf = *(it->second);
		if (bf.isDirty) {
			cout << "need to write to disk"<<bf.pageId << endl;
			writeFile(bf.pageId, bf.data);
		}
		it = hashTable.erase(it);	// will implicitly call the deconstructor
	}

}

#include "BufferManager.h"

int main() {
	BufferManager* bm = new BufferManager(40);
	
	BufferFrame& b1 = bm->fixPage(0x0000000200000101, false);
	((char *)b1.getData())[4]++;// test
	bm->unfixPage(b1, true);/*
	for (int i = 0; i < 10; i++) {
		bm->fixPage(i, false);
	}*/

	bm->~BufferManager();
	free(bm);
	return 1;
}