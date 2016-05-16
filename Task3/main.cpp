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



#include "SPSegment.cpp"

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