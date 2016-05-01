#include <stdio.h>

#include <iostream>

#include "BufferManager.h"


#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <algorithm>
#include <iostream>
#include <unistd.h>



const int pageSize = 800;

using namespace std;



BufferFrame::BufferFrame(uint64_t id) {
	//cout << "create new bufferframe with i "<< id << endl;
	//cout << "in constr "<<inUse<<"\n";
	data = malloc(pageSize);
	pageId = id;
}
void BufferFrame::show() {
	cout << "BF: " << pageId << " inUse :" << inUse << " isDirty: " <<isDirty << " exc "<<exclusive<<  endl;
	for (int i = 0; i < pageSize; i++) {
		cout << ((int *)data)[i] << " ";
	}

	cout << endl;
}

void* BufferFrame::getData() {
	return data;
}

BufferManager::BufferManager(unsigned pageCount_) {
	cout << "create BufferManager with pages " << pageCount_ << endl;
	pageCount = pageCount_;	// max pages in memory
	loadedPages = 0;	// how many pages are curently loaded
}
BufferFrame::~BufferFrame() {
	free(data);
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
			loadedPages++;
			
			
			BufferFrame* bf = new BufferFrame(pageId);
			bf->exclusive = true;	// avoid modifications of others
			hashTable[pageId] = bf;
			table_mutex.unlock();
			readFile(pageId, bf->data);	// read without an lock
			table_mutex.lock();
			bf->exclusive = false;
			
		}
		if (hashTable[pageId]->exclusive || (exclusive && hashTable[pageId]->inUse > 0)) {
			// cerr << "cannot grant this page now" << endl;
			table_mutex.unlock();
			usleep(10);
		} else {
			hashTable[pageId]->exclusive = exclusive;
			hashTable[pageId]->inUse++;

			BufferFrame& bf = *hashTable[pageId];
			table_mutex.unlock();
			return  bf;
		}
	}
}

void BufferManager::freePage() {
	// cout << "need to free page " << hashTable.size() << endl;
	int pick = rand() % hashTable.size();
	// pick randomly one element (0(n) complexity)
	auto it = hashTable.begin();
	for (int i = 0; i < pick; i++) {
		it++;
	}
	for (int i = 0; i < hashTable.size(); i++) {
		if (it == hashTable.end()) {
			it = hashTable.begin();
		}
		if (!it->second->exclusive && it->second->inUse == 0) {
			// need to write
			BufferFrame* bf = it->second;

			
			if (bf->isDirty) {
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
	// cout << "unfix " << frame.pageId << endl;
	
	// force strategy
	if (isDirty) {
		frame.isDirty = true;
	}
	if (frame.exclusive) {
		frame.exclusive = false;
	}

	frame.inUse--;
	table_mutex.unlock();
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
		
		// cout <<"callindg to deconstruct "<<endl;
		// it->second->show();
		BufferFrame bf = *(it->second);
		if (bf.isDirty) {
			// cout << "need to write to disk"<<bf.pageId << endl;
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