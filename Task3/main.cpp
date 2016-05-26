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



#include "SPSegment2.cpp"

#include "Record.hpp"

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

	SPSegment2* sps = new SPSegment2();

	cout << sizeof(TID);
	cout << sizeof(SegmentHeader) << sizeof(Slot);


	
	char* data = (char*)malloc(2);
	data[0] = 6;
	data[1] = 7;
	Record* r = new Record(2, data);


	//TID tid = sps->insert(*r);
	//cout << "was inserted at p"<<tid.pageId<<"slot "<<tid.slotId<<endl;


	//tid = sps->insert(*r);
	//cout << "was inserted at p"<<tid.pageId<<"slot "<<tid.slotId<<endl;

	//TID tid = sps->insert(*r);
	//cout << "was inserted at p"<<tid.pageId<<"slot "<<tid.slotId<<endl;

	//sps->lookup(tid);

	data = (char*)malloc(5);
	data[0] = 1;
	data[1] = 2;
	data[2] = 3;
	data[3] = 4;
	data[4] = 5;
	Record* r_big = new Record(5, data);


	TID tidd = sps->insert(*r_big);
	
TID tid = sps->insert(*r);sps->insert(*r);
cout << "was inserted at p"<<tid.pageId<<"slot "<<tid.slotId<<endl;
tid = sps->insert(*r);cout << "insert finished"<<endl;
	//sps->remove(tidd);
	sps->insert(*r_big);

	sps->update(tid, *r_big);
	sps->lookup(tid);
	sps->remove(tid);
	sps->lookup(tid);

/*
	
	TID tid;
	tid.pageId = 1;
	tid.slotId = 5;
	sps->remove(tid);*/

	

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