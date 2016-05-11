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
	SPSegment() {
		cout << "create spsegment";
		bm = new BufferManager(10);
		// the first page is reserverd for the segement descriptor
		BufferFrame& frame = bm->fixPage(0, false);
		int* pointer = (int*)frame.getData();
		curUsedPages = pointer[0];
		curUsedPages = 1;
		cout << "this segment uses " << curUsedPages << endl;

		bm->unfixPage(frame, false);
		printAllRecords();

	}

	~SPSegment() {
		// save the segements page
		BufferFrame& frame = bm->fixPage(0, true);
		int* pointer = (int*)frame.getData();
		pointer[0] = curUsedPages;
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
			for (int j = 1; j <= count; j++) {
				int size = (j > 1) ? pointer[j] - pointer[j - 1] : pointer[j];
				cout << "P: " << i << " S: " << j << " size " << size << "j: " << pointer[j] << " j-1: " << pointer[j - 1] <<endl;
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
			int slotId = pointer[0] + 1;	
			int offset = pointer[slotId - 1] + size;	// offset where the record will be stored 
			
			if (pageSize - offset < (slotId + 1) * (int)(sizeof(int))) {
				cout << endl << "im full " <<endl;
				bm->unfixPage(frame, false);

				lastTochedFrame++;
				curUsedPages++;
				cout << "switch to page " << lastTochedFrame << endl;
			} else {

				cout << "return frame with " << (pageSize - offset - slotId * sizeof(int))<< endl;
				cout << "Ps"<<pageSize << "off"<<offset<<"num"<<slotId<<endl;
				cout << "int "<<(int)pageSize<< " off int "<<(int)offset<<endl;
				cout << "div"<< (pageSize - offset)<< "<= " << (slotId * sizeof(int)) << endl;
				cout << "bool"<<(pageSize - offset <= slotId * 4)<<endl;
				cout << "inverse"<<((pageSize - offset) > slotId * sizeof(int))<<endl;
				cout << "pointer zero "<< pointer[0] << endl;

				uint u = 64;
				int i = 1-u;
				cout << "1-u "<<i<< "compare "<<(-u < u)<<endl;
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
		int numPos = pointer[0] + 1;	
		int offset = pointer[numPos - 1]  + r.size;	// offset where the record will be stored 
			
		pointer[0]++;	// counter is being incremented
		pointer[numPos] = offset;

		cout << endl << "remaining: " << (pageSize - offset - (numPos + 1) * sizeof(int)) << endl;

		cout << "go record with "<<r.size << endl;
//		cout << "holds value "<< hex(r.data[0])<<endl;
		cout << "nth "<<numPos<<"at offst "<<offset<< " = position" << (pageSize - offset) << endl;
		memcpy((char*)frame.getData() + pageSize - offset, r.data, r.size);
		printFrame(frame);

		bm->unfixPage(frame, true);
		TID tid = *new TID();
		tid.pageId = frame.pageId;
		tid.slotId = numPos;

		return tid;
	}

	Record lookup(TID tid) {
		Record re;
		BufferFrame& frame = bm->fixPage(tid.pageId, false);
		int* pointer = (int*)frame.getData();
		int offset = pointer[tid.slotId];	// the offset position
		// if it has precessing items, the size is the difference of the offsets
		re.size = (tid.slotId > 1) ? offset - pointer[tid.slotId - 1] : offset;
		
		cout << "size is " << re.size << endl;

		re.data = (char*)malloc(re.size);
		memcpy(re.data, (char*)frame.getData() + pageSize - offset, re.size);
		//cout << "data is "<<hex(re.data[0])<<endl;
		return re;

	}
};

int main(int argc, char** argv) {
	// create an extensive big file
	/*int maxPagesOnDisk = 10;
	int fd, ret;
	if ((fd = open("0", O_CREAT|O_TRUNC|O_WRONLY, S_IRUSR|S_IWUSR)) < 0) {
		std::cerr << "cannot open file '" << "0" << "': " << strerror(errno) << std::endl;
		return -1;
	}
	if ((ret = posix_fallocate(fd, 0, maxPagesOnDisk * pageSize * sizeof(uint64_t))) != 0)
		std::cerr << "warning: could not allocate file space: " << strerror(ret) << std::endl;
	close(fd);*/



	SPSegment* sps = new SPSegment();
/* 
	Record* r = new Record();
	r->size = 2;
	char* data = (char*)malloc(2);
	data[0] = 9;
	r->data = data;

	sps->insert(*r);*/


	Record* r = new Record();
	r->size = 1;
	char* data = (char*)malloc(1);
	data[0] = 6;
	r->data = data;

	sps->insert(*r);




	r = new Record();
	r->size = 1;
	data = (char*)malloc(1);
	data[0] = 56;
	r->data = data;

	sps->insert(*r);

	for (int i = 0; i < 10; i++) {
		r = new Record();
		r->size = 5;
		data = (char*)malloc(5);
		data[0] = 65;
		r->data = data;

		sps->insert(*r);
	}

	r = new Record();
	r->size = 1;
	data = (char*)malloc(1);
	data[0] = 10;
	r->data = data;

	TID tid = sps->insert(*r);
	sps->lookup(tid);

	sps->~SPSegment();

}