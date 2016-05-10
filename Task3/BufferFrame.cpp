#include "BufferFrame.h"
using namespace std;

BufferFrame::BufferFrame(uint64_t id) {
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

BufferFrame::~BufferFrame() {
	free(data);
}
