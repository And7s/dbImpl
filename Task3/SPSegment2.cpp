#include <string.h>

#include "SPSegment2.h"

using namespace std;



struct SegmentHeader {
	u_int numSlots;
};


// initializes a slotted pages segment, It is expected, that the file "0" does exist"
SPSegment2::SPSegment2() {
	bm = new BufferManager(10);
	// the first page is reserverd for the segement descriptor
	BufferFrame& frame = bm->fixPage(0, false);
	SegmentHeader* segmentHeader = (SegmentHeader*)frame.getData();
	cout << "SegmentHeader " << segmentHeader->numSlots << " " <<endl;
	segmentHeader->numSlots = 0;


	for (int i = 0; i < segmentHeader->numSlots; i++) {
		Slot* s = getSlot(frame, i);
		s->offset = 4;
		s->size = 66;
	}
	printFrame(frame);
	curUsedPages = 0;	// on creating the data ther is a zero, but zero page is reserved for internal usage
	bm->unfixPage(frame, false);

}

Slot* SPSegment2::getSlot(BufferFrame& frame, u_int idx) {
	return (Slot*) ((char *) frame.getData() + sizeof(SegmentHeader) + sizeof(Slot) * idx);
}
// deconstruct, flush all modified data to disk
SPSegment2::~SPSegment2() {
	// save the segements page
	BufferFrame& frame = bm->fixPage(0, true);
	int* pointer = (int*)frame.getData();
	bm->unfixPage(frame, true);
	bm->~BufferManager();
}

void SPSegment2::printHex(unsigned char c) {
	if (c < 16) {
		cout  << " " << (int)c;
	} else {
		cout << (int)c;
	}
}

void SPSegment2::printFrame(BufferFrame& frame) {
	cout << std::hex;
	for (int i = 0; i < pageSize; i++) {
		if (i % 32 == 0) cout << endl;
		printHex(((unsigned char*)frame.getData())[i]);
	}
	cout << dec << endl;
}

// for debugging purposes, prints all the currently inserted records
void SPSegment2::printAllRecords() {
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
int SPSegment2::getEmptyFrame(u_int size) {
	lastTochedFrame = 0;
	while (true) {
		cout << lastTochedFrame<<endl;
		BufferFrame& frame = bm->fixPage(lastTochedFrame, false);
		
		u_int free_size = getFreeBytes(frame);
		if (free_size >= size + sizeof(Slot)) {
			bm->unfixPage(frame, true);
			return lastTochedFrame;
		} else {
			bm->unfixPage(frame, false);

			lastTochedFrame++;
			if (lastTochedFrame >= 100) throw ("error");
		}
		cout << "error "<< free_size << "want "<< (size + sizeof(Slot)) <<" size "<<size<<endl;

	}
}




TID SPSegment2::insert(const Record&r, TID tid) {
	cout << "will insert on this page" << tid.pageId <<endl;

	BufferFrame& frame = bm->fixPage(tid.pageId, false);
	SegmentHeader* segmentHeader = (SegmentHeader*)frame.getData();
	u_int free_size = getFreeBytes(frame);
	u_int r_len = r.len;
	Slot* slot = getSlot(frame, segmentHeader->numSlots);

	if (free_size < r.len + sizeof(Slot)) {
		cout << "indirect";
		TID orig = insert(r);

		slot->indirect.pageId = orig.pageId;
		slot->indirect.slotId = orig.slotId;

		cout << "did insert at "<<orig.pageId<< " "<<orig.slotId<<endl;

		r_len = 0;	// no payload
	} 
	
	slot->tid.slotId = tid.slotId;
	slot->tid.pageId = tid.pageId;
	
	if (segmentHeader->numSlots > 0) {
		Slot* prevSlot = getSlot(frame, segmentHeader->numSlots - 1);
		slot->offset = prevSlot->offset - r_len;
	} else {
		slot->offset = pageSize - r_len;
	}
	slot->size = r_len;

	segmentHeader->numSlots++;
	
	memcpy((char*)frame.getData() + slot->offset, r.data, r_len);
	bm->unfixPage(frame, false);
	
	globalSidCounter++;
}


TID SPSegment2::insert(const Record& r) {
	if (r.len <= 0) {
		cerr << "Record without data is not allowed. abort";
		exit(-1);
	}
	if (r.len >= pageSize) {
		cerr << "Data too large, use large pageSize. abort";
		exit(-1);
	}
	cout << "insert now ";

	TID tid;
	tid.slotId = globalSidCounter;
	tid.pageId =  getEmptyFrame(r.len);
	
	insert(r, tid);


	return tid;
}

Record SPSegment2::lookup(TID tid) {

	BufferFrame& frame = bm->fixPage(tid.pageId, false);
	SegmentHeader* segmentHeader = (SegmentHeader*)frame.getData();
	
	for (int i = 0; i < segmentHeader->numSlots; i++) {
		Slot* slot = getSlot(frame, i);
		if (slot->tid == tid) {
		cout << "found"<<slot->size;
			if (slot->size == 0) {	// indirect lookup
				bm->unfixPage(frame, false);
				return lookup(slot->indirect);
			}

			char* data = (char*)malloc(slot->size);
			memcpy(data, (char*)frame.getData() + slot->offset, slot->size);

			bm->unfixPage(frame, false);
			return Record(slot->size, data);
		}
	}
	cout << "could not found";
	bm->unfixPage(frame, false);
	return Record(0, NULL);
}

bool SPSegment2::remove(TID tid) {
	BufferFrame& frame = bm->fixPage(tid.pageId, true);
	SegmentHeader* segmentHeader = (SegmentHeader*)frame.getData();


	for (int i = 0; i < segmentHeader->numSlots; i++) {
		Slot* slot = getSlot(frame, i);
		if (slot->tid == tid) {
			cout <<"found to remove "<<i<<endl;
			if (slot->size == 0) {	// must delete indirect
				cout << "delete indirect";
				remove(slot->indirect);
			}
			u_int elAfter = segmentHeader->numSlots - i - 1;
			
			char* begin = (char*)frame.getData() + sizeof(SegmentHeader) + sizeof(Slot) * i;

			// align the payload			
			Slot* lastSlot = getSlot(frame, segmentHeader->numSlots - 1);
			segmentHeader->numSlots--;

			char* src = (char*)frame.getData() + lastSlot->offset;
			memmove(src + slot->size, src, slot->offset - lastSlot->offset);
			for (int j = 0; j < elAfter; j++) {
				Slot* slotAfter = getSlot(frame, j + i + 1);
				slotAfter->offset += slot->size;
				//cout << "off "<<slotAfter->offset<< " size "<<slot->size <<endl;
			}
			// align the slots
			memmove(begin, begin + sizeof(Slot), elAfter * sizeof(Slot));
			
			cout << "remove"<<endl;
			cout << "free now "<< getFreeBytes(frame)<<endl;

			bm->unfixPage(frame, true);
			return true;
		}
	}
	bm->unfixPage(frame, false);
	return false;	// wasnt able to find this record
}

u_int SPSegment2::getFreeBytes(BufferFrame& frame) {
	SegmentHeader* segmentHeader = (SegmentHeader*)frame.getData();
	if (segmentHeader->numSlots == 0) {
		return pageSize - sizeof(SegmentHeader);
	}
	Slot* lastSlot = getSlot(frame, segmentHeader->numSlots - 1);
	return lastSlot->offset - sizeof(SegmentHeader) - segmentHeader->numSlots * sizeof(Slot);	
}

bool SPSegment2::update(TID tid, const Record& r) {
	Record r2 = lookup(tid);
	remove(tid);
	insert(r, tid);
	return true;
}