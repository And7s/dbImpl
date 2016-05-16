#include <string.h>

#include "SPSegment.h"

using namespace std;


SPSegment::SPSegment() {
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

SPSegment::~SPSegment() {
	// save the segements page
	BufferFrame& frame = bm->fixPage(0, true);
	int* pointer = (int*)frame.getData();
	pointer[0] = curUsedPages;
	pointer[1] = globalSidCounter;
	bm->unfixPage(frame, true);

	printAllRecords();

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
BufferFrame& SPSegment::getEmptyFrame(int size) {

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

TID SPSegment::insert(const Record& r) {
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

Record SPSegment::lookup(TID tid) {
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

void SPSegment::remove(TID tid) {
	
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
