#include <string.h>

#include "SPSegment.h"

using namespace std;


// initializes a slotted pages segment, It is expected, that the file "0" does exist"
SPSegment::SPSegment() {
	bm = new BufferManager(10);
	// the first page is reserverd for the segement descriptor
	BufferFrame& frame = bm->fixPage(0, false);
	int* pointer = (int*)frame.getData();
	curUsedPages = 0;	// on creating the data ther is a zero, but zero page is reserved for internal usage
	bm->unfixPage(frame, false);
}

// deconstruct, flush all modified data to disk
SPSegment::~SPSegment() {
	// save the segements page
	BufferFrame& frame = bm->fixPage(0, true);
	int* pointer = (int*)frame.getData();
	bm->unfixPage(frame, true);
	bm->~BufferManager();
}

void SPSegment::printHex(unsigned char c) {
	if (c < 16) {
		cout  << " " << (int)c;
	} else {
		cout << (int)c;
	}
}

void SPSegment::printFrame(BufferFrame& frame) {
	cout << std::hex;
	for (int i = 0; i < pageSize; i++) {
		if (i % 32 == 0) cout << endl;
		printHex(((unsigned char*)frame.getData())[i]);
	}
	cout << dec << endl;
}

// for debugging purposes, prints all the currently inserted records
void SPSegment::printAllRecords() {
	for (int i = 0; i <= curUsedPages; i++) {
		BufferFrame& frame = bm->fixPage(i, false);
		printFrame(frame);
		int* pointer = (int*)frame.getData();
		int count = pointer[0];
		for (int j = 0; j < count; j++) {
			int size = (j > 0) ? pointer[j * 2 + 1] - pointer[j * 2 - 1] : pointer[j * 2 + 1];

			for (int k = 0; k < size; k++) {
				printHex(((unsigned char*)frame.getData())[pageSize - pointer[j * 2 + 1] + k]);
			}
			cout << endl;
		}

		bm->unfixPage(frame, false);
	}
}

// searchs and returns a frame with at least size + pointer empty space
BufferFrame& SPSegment::getEmptyFrame(int size) {
	bool jumped = false;
	while (true) {
		BufferFrame& frame = bm->fixPage(lastTochedFrame, false);
		int* pointer = (int*)frame.getData();

		// the whichth element this had been insereted (fist element is elemnt 1), is equal to the position wherr the pointer has been stored

		int numEntries = pointer[0];

		int offset = (numEntries > 0) ? pointer[numEntries * 2 - 1] + size: size;	// offset where the record will be stored 
		
		if (pageSize - offset < (numEntries * 2 + 3) * (int)(sizeof(int))) {
			//cout << endl << "im full has: " << numEntries <<endl;
			bm->unfixPage(frame, false);

			lastTochedFrame++;
			if (lastTochedFrame > curUsedPages - 1) {	// ath tjhe end of the pages
				// only take a new page if unavoidable
				if (!jumped) {
					lastTochedFrame = 0;
					jumped = true;
				} else {
					curUsedPages = lastTochedFrame + 1;
			
				}
			}
			
		} else {
			// this frame is lage enough to be used
			return frame;
		}

	}
}

TID SPSegment::insert(const Record& r) {
	if (r.len <= 0) {
		cerr << "Record without data is not allowed. abort";
		exit(-1);
	}
	if (r.len >= pageSize) {
		cerr << "Data too large, use large pageSize. abort";
		exit(-1);
	}

	BufferFrame& frame = getEmptyFrame(r.len);
	int* pointer = (int*)frame.getData();
	pointer[0]++;	// counter is being incremented
	int numEntries = pointer[0];
	int offset = (numEntries > 0) ? pointer[numEntries * 2 - 3] : 0;	// pointer to the last element
	offset += r.len;	// pointer where i will store the current record 
		
	pointer[numEntries * 2 - 1] = offset;
	pointer[numEntries * 2] = globalSidCounter;

	memcpy((char*)frame.getData() + pageSize - offset, r.data, r.len);

	bm->unfixPage(frame, true);
	TID tid = *new TID();
	tid.pageId = frame.pageId;	
	tid.slotId = globalSidCounter;
	
	globalSidCounter++;

	return tid;
}

Record SPSegment::lookup(TID tid) {

	BufferFrame& frame = bm->fixPage(tid.pageId, false);
	int* pointer = (int*)frame.getData();
	int numEntries = pointer[0];	// the offset position

	for (int i = 1; i <= numEntries; i++) {
		//cout << "there is "<<pointer[i * 2]<< "i" << i << "of " << numEntries << endl; 
		if (pointer[i * 2] == tid.slotId) {	// found the element
			int offset = pointer[i * 2 - 1];
			int len = (i > 0) ? offset - pointer[i * 2 - 3] : offset;
			//cout << "size is " << len << endl;

			char* data = (char*)malloc(len);
			memcpy(data, (char*)frame.getData() + pageSize - offset, len);

			bm->unfixPage(frame, false);
			return Record(len, data);
		}
	}
	bm->unfixPage(frame, false);
	return Record(0, NULL);
}

bool SPSegment::remove(TID tid) {
	BufferFrame& frame = bm->fixPage(tid.pageId, true);
	int* pointer = (int*)frame.getData();
	int numEntries = pointer[0];	// the offset position

	for (int i = 0; i < numEntries; i++) {
		if (pointer[i * 2 + 2] == tid.slotId) {	// found the element
			int elAfter = numEntries - i - 1;
			int offset = pointer[i * 2 + 1];
			int size = (i > 0) ? offset - pointer[i * 2 - 1] : offset;

			int lastOffset = pointer[numEntries * 2 - 1];

			char* dataPointer = (char*)frame.getData();
			// shift the data after the removed element by size
			memmove(dataPointer + pageSize - lastOffset + size, dataPointer + pageSize - lastOffset, lastOffset - offset);	 

			memmove(dataPointer + (i * 2 + 1) * sizeof(int), dataPointer + (i * 2 + 3)* sizeof(int), elAfter * 2 * sizeof(int));	 
			// reduce offsets
			for (int j = 0; j < elAfter; j++) {
				pointer[(i +j) * 2 + 1] -= size;
			}

			pointer[0]--;
			
			bm->unfixPage(frame, true);
			return true;
		}
	}
	return false;	// wasnt able to find this record
}


bool SPSegment::update(TID tid, const Record& r) {
	Record r2 = lookup(tid);
	remove(tid);
	lastTochedFrame = tid.pageId;
	TID tid2 = insert(r);
	if (tid.pageId == tid2.pageId) {	// insert is on the same page, can reuse

		BufferFrame& frame = bm->fixPage(tid.pageId, true);
		//printFrame(frame);
		int* pointer = (int*)frame.getData();
		int numEntries = pointer[0];	// the offset position

		for (int i = 0; i < numEntries; i++) {
			if (pointer[i * 2 + 2] == tid2.slotId) {	// found the element
				pointer[i * 2 + 2] = tid.slotId;
			}
		}
		bm->unfixPage(frame, true);
	} else {
		cout << "PROBLEM now on different page"<<endl;
		// TODO: implement indirection
	}
	return true;
}