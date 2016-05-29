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



#include "SPSegment.h"

#include "BTree.cpp"

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

	//SPSegment* sps = new SPSegment();

//	cout << sizeof(TID);
	//cout << sizeof(SegmentHeader) << sizeof(Slot);


	BTree<int> b = BTree<int>();
	TID tid;
	tid.pageId = 0;
	tid.slotId = 0;
	b.insert(4, tid);
	b.insert(3, tid);
	b.insert(2, TID());
		b.insert(5, TID());
	b.insert(1, TID());
	b.insert(6, TID());
	b.insert(7, TID());
	b.insert(8, TID());

		b.insert(4, TID());

				b.insert(9, TID());
b.insert(10, TID());
b.lookup(5);b.lookup(5);
b.erase(5);
b.lookup(5);
	//b.insert(8, TID());
	//new BTree(4);*/
	


}