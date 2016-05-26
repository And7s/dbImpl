#pragma once
#include "BufferManager.h"
#include "Record.hpp"



struct TID {
public:
	uint64_t pageId: 32, slotId :32;
	bool operator==(const TID &tid) const {
		return tid.pageId == pageId && tid.slotId == slotId;
	}
};

namespace std {
	template <> struct hash<TID> {
		size_t operator()(const TID & tid) const {
			return (tid.slotId);
		}
	};
}

struct Slot {
	TID tid;
	TID indirect;
	u_int offset;
	u_int size;
};




class SPSegment {
public:

	SPSegment() ;

	~SPSegment();

	Slot* getSlot(BufferFrame& frame, u_int idx);

	void printHex(unsigned char c);
	void printFrame(BufferFrame& frame);

	// for debugging purposes, prints all the currently inserted records
	void printAllRecords();

	

	// searchs and returns a frame with at least size + pointer empty space
	int getEmptyFrame(u_int size);
	TID insert(const Record& r, TID tid);
	TID insert(const Record& r);
	u_int getFreeBytes(BufferFrame& frame);
	Record lookup(TID tid);

	bool remove(TID tid);

	bool update(TID tid, const Record& r);
private:

	BufferManager* bm;
	int globalSidCounter = 0;
};