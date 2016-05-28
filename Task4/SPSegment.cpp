#include <string.h>

#include "SPSegment.h"

using namespace std;


struct SegmentHeader {
	u_int numSlots;
};


// initializes a slotted pages segment, It is expected, that the file "0" does exist"
SPSegment::SPSegment() {
	bm = new BufferManager(10);
	// the first page is reserverd for the segement descriptor
	BufferFrame& frame = bm->fixPage(0, false);
	SegmentHeader* segmentHeader = (SegmentHeader*)frame.getData();
	segmentHeader->numSlots = 0;
	bm->unfixPage(frame, false);
}

Slot* SPSegment::getSlot(BufferFrame& frame, u_int idx) {
	return (Slot*) ((char *) frame.getData() + sizeof(SegmentHeader) + sizeof(Slot) * idx);
}
// deconstruct, flush all modified data to disk
SPSegment::~SPSegment() {
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


// searchs and returns a frame with at least size + pointer empty space
int SPSegment::getEmptyFrame(u_int size) {
	int searchFrame = 0;
	while (true) {
		BufferFrame& frame = bm->fixPage(searchFrame, false);
		
		u_int free_size = getFreeBytes(frame);
		if (free_size >= size + sizeof(Slot)) {
			bm->unfixPage(frame, true);
			return searchFrame;
		} else {
			bm->unfixPage(frame, false);

			searchFrame++;
			if (searchFrame >= 100) throw ("cant find empty page");
		}
	}
}




TID SPSegment::insert(const Record&r, TID tid) {
	BufferFrame& frame = bm->fixPage(tid.pageId, false);
	SegmentHeader* segmentHeader = (SegmentHeader*)frame.getData();
	u_int free_size = getFreeBytes(frame);
	u_int r_len = r.len;
	Slot* slot = getSlot(frame, segmentHeader->numSlots);

	if (free_size < r.len + sizeof(Slot)) {	// indirect insert
		TID orig = insert(r);
		slot->indirect.pageId = orig.pageId;
		slot->indirect.slotId = orig.slotId;
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


TID SPSegment::insert(const Record& r) {
	if (r.len <= 0) {
		cerr << "Record without data is not allowed. abort";
		exit(-1);
	}
	if (r.len >= pageSize) {
		cerr << "Data too large, use large pageSize. abort";
		exit(-1);
	}

	TID tid;
	tid.slotId = globalSidCounter;
	tid.pageId =  getEmptyFrame(r.len);
	
	insert(r, tid);
	return tid;
}

Record SPSegment::lookup(TID tid) {

	BufferFrame& frame = bm->fixPage(tid.pageId, false);
	SegmentHeader* segmentHeader = (SegmentHeader*)frame.getData();
	
	for (int i = 0; i < segmentHeader->numSlots; i++) {
		Slot* slot = getSlot(frame, i);
		if (slot->tid == tid) {
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
	bm->unfixPage(frame, false);
	return Record(0, NULL);
}

bool SPSegment::remove(TID tid) {
	BufferFrame& frame = bm->fixPage(tid.pageId, true);
	SegmentHeader* segmentHeader = (SegmentHeader*)frame.getData();


	for (int i = 0; i < segmentHeader->numSlots; i++) {
		Slot* slot = getSlot(frame, i);
		if (slot->tid == tid) {
			if (slot->size == 0) {	// must delete indirect
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
			
			bm->unfixPage(frame, true);
			return true;
		}
	}
	bm->unfixPage(frame, false);
	return false;	// wasnt able to find this record
}

u_int SPSegment::getFreeBytes(BufferFrame& frame) {
	SegmentHeader* segmentHeader = (SegmentHeader*)frame.getData();
	if (segmentHeader->numSlots == 0) {
		return pageSize - sizeof(SegmentHeader);
	}
	Slot* lastSlot = getSlot(frame, segmentHeader->numSlots - 1);
	return lastSlot->offset - sizeof(SegmentHeader) - segmentHeader->numSlots * sizeof(Slot);	
}

bool SPSegment::update(TID tid, const Record& r) {
	Record r2 = lookup(tid);
	remove(tid);
	insert(r, tid);
	return true;
}