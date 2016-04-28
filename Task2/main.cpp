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
	//cout << "create new bufferframe with i "<< id << endl;
	//cout << "in constr "<<inUse<<"\n";
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
	
	cout << "fix "<<pageId<< "(exl. "<< exclusive << ") loaded "<< loadedPages << "pags" << endl;

	if (hashTable[pageId] == NULL) {	// data is not in memory
		cout <<"hello "<<endl;
		if (loadedPages == pageCount) {
			cout << "out of memory"<< endl;
			freePage();
		}
		loadedPages++;
		BufferFrame* bf = new BufferFrame(pageId);
		readFile(pageId, bf->data);
		
		hashTable[pageId] = bf;
	} else {
		cout << "have already " << pageId <<endl;
	}
	if (hashTable[pageId]->exclusive) {
		cerr << "someone else uses alrady this page exclusivly" << endl;
		throw -2;
	}

	if (exclusive) {
		//cout << "try exclusive";
		if (hashTable[pageId]->exclusive || hashTable[pageId]->inUse > 0) {
			cerr << "cannot grant exclusive use " << endl;
			throw -1;	// only one exclusive allowed, TODO: wait
		}
		hashTable[pageId]->exclusive = true;
	}

	
	hashTable[pageId]->inUse++;

	// print the content
	//hashTable[pageId]->show();
	return *hashTable[pageId];

}

void BufferManager::freePage() {
	// TODO: better erase strategy
	for (auto it = hashTable.begin(); it != hashTable.end(); it++) {
		if (!it->second->exclusive && it->second->inUse == 0) {
			// as we use force, we can directly free it
			hashTable.erase(it);
			cout << "erase "<<endl;
			loadedPages--;
			return;
		}
	}
	// wasnt able to free memory
	cerr << "no more meory "<<endl;
	throw -3;
}
void BufferManager::unfixPage(BufferFrame& frame, bool isDirty) {
	cout << "unfix " << frame.pageId << endl;
	
	// force strategy
	if (isDirty) {
		writeFile(frame.pageId, frame.data);
	}
	if (frame.exclusive) {
		frame.exclusive = false;
	}
	frame.inUse--;	

	//frame.show();
}

void BufferManager::readFile(uint64_t pageId, void* buff) {
	//cout << "got page "<<pageId;
	uint64_t segment = (pageId  & 0xFFFFFFFF00000000) >> 32;
	uint64_t page = (pageId & 0x00000000FFFFFFFF);
	//cout << "segment is "<<segment << " page " << page << " "<< endl;
	
	int fd;
	if ((fd = open(to_string(segment).c_str(), O_RDWR)) < 0) {
		std::cerr << "cannot open file" << strerror(errno) << std::endl;
	}
	if (lseek(fd, page * pageSize	, SEEK_SET) < 0) {
		cerr << "error lseek  " << strerror(errno) << std::endl;
	}
	if (read(fd, buff, pageSize) < 0) {
		cerr << "error read  " << strerror(errno) << std::endl;
	}
	close(fd);
}

void BufferManager::writeFile(uint64_t pageId, void* buff) {
	cout << "write page "<<pageId<<endl;
	uint64_t segment = (pageId  & 0xFFFFFFFF00000000) >> 32;
	uint64_t page = (pageId & 0x00000000FFFFFFFF);
	//cout << "segment is "<<segment << " page " << page << " "<< endl;
	
	int fd;
	if ((fd = open(to_string(segment).c_str(), O_RDWR)) < 0) {
		std::cerr << "cannot open file" << strerror(errno) << std::endl;
	}
	cout << "open";
	if (lseek(fd, page * pageSize	, SEEK_SET) < 0) {
		cerr << "error lseek  " << strerror(errno) << std::endl;
	}
	cout << "seek";

	if (write(fd, buff, pageSize) < 0) {
		cerr << "error write  " << strerror(errno) << std::endl;
	}
	cout << "writte";
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
/*
int main() {
	BufferManager* bm = new BufferManager(5);
	
	for (int i = 0; i < 5; i++) {
		BufferFrame& b1 = bm->fixPage(i, false);
		bm->unfixPage(b1, false);
	}
	BufferFrame& b1 = bm->fixPage(0, false);
	
	

	bm->~BufferManager();
	free(bm);
	return 1;
}*/