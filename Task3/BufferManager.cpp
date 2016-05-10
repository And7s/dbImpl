#include "BufferManager.h"
#include "BufferFrame.h"

#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <iostream>

using namespace std;


BufferManager::BufferManager(unsigned pageCount_) {
	cout << "create BufferManager with pages " << pageCount_ << endl;
	pageCount = pageCount_;	// max pages in memory
	loadedPages = 0;		// how many pages are curently loaded
}

BufferFrame& BufferManager::fixPage(uint64_t pageId, bool exclusive) {
	// this page can 1. be in memory, 2. be on disk

	while(true) {
		table_mutex.lock();
		
		auto it = hashTable.find(pageId);
		if (it == hashTable.end()) {	// data is not in memory
			if (loadedPages == pageCount) {
				freePage();
			}
			loadedPages++;	// declare once more page loaded
			
			BufferFrame* bf = new BufferFrame(pageId);
			bf->exclusive = true;	// avoid modifications of others
			hashTable[pageId] = bf;
			table_mutex.unlock();
			readFile(pageId, bf->data);	// read without an lock
			table_mutex.lock();
			bf->exclusive = false;
			
		}
		if (hashTable[pageId]->exclusive || (exclusive && hashTable[pageId]->inUse > 0)) {
			// cannot grant this page now 
			table_mutex.unlock();
			usleep(10);
		} else {
			// as i can grant you the page, it is not exclusive, but could be set to exclusive
			hashTable[pageId]->exclusive = exclusive;
			hashTable[pageId]->inUse++;

			BufferFrame& bf = *hashTable[pageId];
			table_mutex.unlock();
			return bf;
		}
	}
}

void BufferManager::freePage() {
	// pick a random offset from where an empty page is searched
	int pick = rand() % hashTable.size();
	auto it = hashTable.begin();
	for (int i = 0; i < pick; i++) {
		it++;
	}
	for (int i = 0; i < hashTable.size(); i++) {
		if (it == hashTable.end()) {	// loop once, can go over borders
			it = hashTable.begin();
		}
		if (!it->second->exclusive && it->second->inUse == 0) {
			
			BufferFrame* bf = it->second;

			if (bf->isDirty) { // need to write
				writeFile(bf->pageId, bf->data);
			}		
			hashTable.erase(it);	
			loadedPages--;
			return;
		}
		it++;
	}
	
	// wasnt able to free memory
	cerr << "no more memory "<<endl;
	throw -3;
}
void BufferManager::unfixPage(BufferFrame& frame, bool isDirty) {
	table_mutex.lock();
	// no force
	if (isDirty) {
		frame.isDirty = true;
	}
	if (frame.exclusive) {
		frame.exclusive = false;
	}

	frame.inUse--; // memorize that one less has this page referenced
	table_mutex.unlock();
}

void BufferManager::readFile(uint64_t pageId, void* buff) {
	// segment is first 32bit, page is last 32 bit
	uint64_t segment = (pageId  & 0xFFFFFFFF00000000) >> 32;
	uint64_t page = (pageId & 0x00000000FFFFFFFF);
	
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
	// cout << "write page "<<pageId<<endl;
	uint64_t segment = (pageId  & 0xFFFFFFFF00000000) >> 32;
	uint64_t page = (pageId & 0x00000000FFFFFFFF);
	
	int fd;
	if ((fd = open(to_string(segment).c_str(), O_RDWR)) < 0) {
		std::cerr << "cannot open file" << strerror(errno) << std::endl;
	}
	if (lseek(fd, page * pageSize	, SEEK_SET) < 0) {
		cerr << "error lseek  " << strerror(errno) << std::endl;
	}
	if (write(fd, buff, pageSize) < 0) {
		cerr << "error write  " << strerror(errno) << std::endl;
	}
	close(fd);
}

BufferManager::~BufferManager() {
	cout << "deconstuct buffer manager" << endl;
	for (auto it = hashTable.begin(); it != hashTable.end(); ) {
		BufferFrame bf = *(it->second);
		if (bf.isDirty) {
			writeFile(bf.pageId, bf.data);
		}
		it = hashTable.erase(it);	// will implicitly call the deconstructor
	}
}
