#include <iostream>
#include <vector>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <pthread.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <algorithm>
#include <iostream>

#include "BufferManager.h"

/*
g++ main.cpp BufferManager.cpp BufferFrame.cpp -std=c++11 && ./a.out 

*/
using namespace std;


class TID {
public:
	uint64_t pageId, slotId;
};

class Record {
public:
	uint size;
	char* data;

};


class SPSegment {
public:
	BufferManager* bm;
	int lastTochedFrame = 1;	// zero is reserved for the segment
	int curUsedPages = 0;	// how many pages are (partially) filled with user data
	int globalSidCounter = 0;
	SPSegment() {
		cout << "create spsegment";
		bm = new BufferManager(10);
		// the first page is reserverd for the segement descriptor
		BufferFrame& frame = bm->fixPage(0, false);
		int* pointer = (int*)frame.getData();
		curUsedPages = pointer[0];
		globalSidCounter = pointer[0];
		if (curUsedPages < 1) curUsedPages = 1;	// on creating the data ther is a zero, but zero page is reserved for internal usage
		cout << "this segment uses " << curUsedPages << endl;

		bm->unfixPage(frame, false);
		printAllRecords();

	}

	~SPSegment() {
		// save the segements page
		BufferFrame& frame = bm->fixPage(0, true);
		int* pointer = (int*)frame.getData();
		pointer[0] = curUsedPages;
		pointer[1] = globalSidCounter;
		bm->unfixPage(frame, true);

		printAllRecords();

		bm->~BufferManager();
	}

	void printHex(unsigned char c) {
		if (c < 16) {
			cout  << " " << (int)c;
		} else {
			cout << (int)c;
		}
	}
	void printFrame(BufferFrame& frame) {
		cout << std::hex;
		for (int i = 0; i < pageSize; i++) {
			if (i % 32 == 0) cout << endl;
			printHex(((unsigned char*)frame.getData())[i]);

		}

		cout << dec << endl;
	}

	// for debugging purposes, prints all the currently inserted records
	void printAllRecords() {
		for (int i = 1; i <= curUsedPages; i++) {
			BufferFrame& frame = bm->fixPage(i, false);
			printFrame(frame);
			int* pointer = (int*)frame.getData();
			int count = pointer[0];
			for (int j = 0; j < count; j++) {
				int size = (j > 0) ? pointer[j * 2 + 1] - pointer[j * 2 - 1] : pointer[j * 2 + 1];

				cout << "P: " << i << " S: " << pointer[j * 2 + 2] << " size " << size << " = ";
				for (int k = 0; k < size; k++) {
					printHex(((unsigned char*)frame.getData())[pageSize - pointer[j * 2 + 1] + k]);
				}
				cout << endl;
			}

			bm->unfixPage(frame, false);
		}
	}

	// searchs and returns a frame with at least size + pointer empty space
	BufferFrame& getEmptyFrame(int size) {

		while (true) {
			BufferFrame& frame = bm->fixPage(lastTochedFrame, false);
			int* pointer = (int*)frame.getData();

			// the whichth element this had been insereted (fist element is elemnt 1), is equal to the position wherr the pointer has been stored

			int numEntries = pointer[0];

			int offset = (numEntries > 0) ? pointer[numEntries * 2 - 1] + size: size;	// offset where the record will be stored 
			
			if (pageSize - offset < (numEntries * 2 + 3) * (int)(sizeof(int))) {
				cout << endl << "im full " <<endl;
				bm->unfixPage(frame, false);

				lastTochedFrame++;
				curUsedPages++;
				cout << "switch to page " << lastTochedFrame << endl;
			} else {

				cout << "return frame ";				
				cout << "Ps"<<pageSize << "off"<<offset<<"numEntries"<< numEntries << endl;
				

				printFrame(frame);
				return frame;
			}


		}
	}

	TID insert(const Record& r) {
		if (r.size <= 0) {
			cerr << "Record without data is not allowed. abort";
			exit(-1);
		}

		BufferFrame& frame = getEmptyFrame(r.size);
		int* pointer = (int*)frame.getData();
		pointer[0]++;	// counter is being incremented
		int numEntries = pointer[0];
		int offset = (numEntries > 0) ? pointer[numEntries * 2 - 3] : 0;	// pointer to the last element
		offset += r.size;	// pointer where i will store the current record 
			
		
		pointer[numEntries * 2 - 1] = offset;
		pointer[numEntries * 2] = globalSidCounter;

		cout << endl << "remaining: " << (pageSize - offset - (numEntries * 2 + 1) * sizeof(int)) << endl;

		cout << "go record with "<<r.size << endl;
//		cout << "holds value "<< hex(r.data[0])<<endl;
		cout << "numEntries "<<numEntries<<"at offst "<<offset<< " = position" << (pageSize - offset) << endl;
		memcpy((char*)frame.getData() + pageSize - offset, r.data, r.size);
		printFrame(frame);

		bm->unfixPage(frame, true);
		TID tid = *new TID();
		tid.pageId = frame.pageId;
		tid.slotId = globalSidCounter;
		globalSidCounter++;

		return tid;
	}

	Record lookup(TID tid) {
		Record re;
		BufferFrame& frame = bm->fixPage(tid.pageId, false);
		int* pointer = (int*)frame.getData();
		int numEntries = pointer[0];	// the offset position

		for (int i = 0; i < numEntries; i++) {
			if (pointer[i * 2] == tid.slotId) {	// found the element
				int offset = pointer[i * 2 - 1];
				re.size = (i > 0) ? offset - pointer[i * 2 - 3] : offset;
				cout << "size is " << re.size << endl;

				re.data = (char*)malloc(re.size);
				memcpy(re.data, (char*)frame.getData() + pageSize - offset, re.size);
				bm->unfixPage(frame, false);
				return re;
			}
		}
		bm->unfixPage(frame, false);
		cout << "didnt find the Record";
		return re;
		

	}

	void remove(TID tid) {
		
		cout << "will remove "<<tid.pageId << " at slot "<<tid.slotId<<endl;
		BufferFrame& frame = bm->fixPage(tid.pageId, false);
		printFrame(frame);
		int* pointer = (int*)frame.getData();
		int numEntries = pointer[0];	// the offset position

		for (int i = 0; i < numEntries; i++) {
			if (pointer[i * 2 + 2] == tid.slotId) {	// found the element
				cout << "fount at "<<i << "of "<<numEntries<<endl;
				int elAfter = numEntries - i - 1;
				int offset = pointer[i * 2 + 1];
				int size = (i > 0) ? offset - pointer[i * 2 - 1] : offset;

				int lastOffset = pointer[numEntries * 2 - 1];

				cout << "elAfter "<<elAfter<<"offset"<<offset<<" size "<<size << "lastOffset " << lastOffset << endl;
				char* dataPointer = (char*)frame.getData();
				// shift the data after the removed element by size
				memmove(dataPointer + pageSize - lastOffset + size, dataPointer + pageSize - lastOffset, lastOffset - offset);	 

				// reaign poiners
				// TODO
				memmove(dataPointer + (i * 2 + 1) * sizeof(int), dataPointer + (i * 2 + 3)* sizeof(int), elAfter * 2 * sizeof(int));	 
				// reduce offsets
				for (int j = 0; j < elAfter; j++) {
					pointer[(i +j) * 2 + 1] -= size;
				}

				pointer[0]--;
				cout << "el after "<<elAfter<<endl;	
				cout << "offset "<<offset<<" size "<<size <<endl;
				printFrame(frame);
				bm->unfixPage(frame, true);
				printAllRecords();
				

			}
		}
	}
};

int main(int argc, char** argv) {
	// create an extensive big file
	int maxPagesOnDisk = 10;
	int fd, ret;
	if ((fd = open("0", O_CREAT|O_TRUNC|O_WRONLY, S_IRUSR|S_IWUSR)) < 0) {
		std::cerr << "cannot open file '" << "0" << "': " << strerror(errno) << std::endl;
		return -1;
	}
	if ((ret = posix_fallocate(fd, 0, maxPagesOnDisk * pageSize * sizeof(uint64_t))) != 0)
		std::cerr << "warning: could not allocate file space: " << strerror(ret) << std::endl;
	close(fd);

	SPSegment* sps = new SPSegment();

/*
	
	TID tid;
	tid.pageId = 1;
	tid.slotId = 5;
	sps->remove(tid);*/

	Record* r = new Record();
	r->size = 1;
	char* data = (char*)malloc(1);
	data[0] = 6;
	r->data = data;

	sps->insert(*r);




	r = new Record();
	r->size = 5;
	data = (char*)malloc(5);
	data[0] = 56;
	r->data = data;

	sps->insert(*r);

	/*for (int i = 0; i < 10; i++) {
		r = new Record();
		r->size = 5;
		data = (char*)malloc(5);
		data[0] = 65;
		r->data = data;

		sps->insert(*r);
	}*/

	r = new Record();
	r->size = 1;
	data = (char*)malloc(1);
	data[0] = 10;
	r->data = data;
		r = new Record();
	r->size = 1;
	data = (char*)malloc(1);
	data[0] = 11;
	r->data = data;


	TID tid = sps->insert(*r);
	tid.slotId = 1;
	sps->remove(tid);



	r = new Record();
	r->size = 2;
	data = (char*)malloc(2);
	data[0] = 66;
	data[1] = 1;
	r->data = data;

	sps->insert(*r);


/*
	r = new Record();
	r->size = 2;
	data = (char*)malloc(2);
	data[0] = 11;
	r->data = data;

	sps->insert(*r);

	
	sps->printAllRecords();*/
	//sps->~SPSegment();

}