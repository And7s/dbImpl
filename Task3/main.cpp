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

class Record {
public:
	uint size;
	char* data;

};

struct HexCharStruct {
  unsigned char c;
  HexCharStruct(unsigned char _c) : c(_c) { }
};

inline std::ostream& operator<<(std::ostream& o, const HexCharStruct& hs) {
  if (((int)hs.c) < 16) {
  	return (o << " " << std::hex << (int)hs.c);
  } else {
  	return (o << std::hex << (int)hs.c);
  }
  
}

inline HexCharStruct hex(unsigned char _c) {
  return HexCharStruct(_c);
}



class SPSegment {
public:
	BufferManager* bm;
	SPSegment() {
		cout << "create spsegment";
		bm = new BufferManager(10);
	}

	void printFrame() {
		BufferFrame& frame = bm->fixPage(0, false);

		for (int i = 0; i < pageSize; i++) {
			if (i % 32 == 0) cout << endl;
			cout << hex(((char*)frame.getData())[i]);

		}
	}

	void insert(const Record& r) {
		if (r.size <= 0) {
			cerr << "Record without data is not allowed. abort";
			exit(-1);
		}
		BufferFrame& frame = bm->fixPage(0, false);
		int* pointer = (int*)frame.getData();
		int pos = 0;

		// the whichth element this had been insereted (fist element is elemnt 1), is equal to the position wherr the pointer has been stored
		int numPos = pointer[0] + 1;	
		int offset = pointer[numPos - 1]  + r.size;	// offset where the record will be stored 
		
		
		if (pageSize - offset <= numPos * sizeof(int)) {
			cout << "im full "<<endl;
			return;
		}
		pointer[0]++;	// counter is being incremented
		pointer[numPos] = offset;

		cout << endl << "remaining: " << (pageSize - offset - (numPos + 1) * sizeof(int)) << endl;

		cout << "go record with "<<r.size << endl;
		cout << "holds value "<< hex(r.data[0])<<endl;
		cout << "nth "<<numPos<<"at offst "<<offset<< " = position" << (pageSize - offset) << endl;
		memcpy((char*)frame.getData() + pageSize - offset, r.data, r.size);
		printFrame();
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

	sps->insert(*r);





}