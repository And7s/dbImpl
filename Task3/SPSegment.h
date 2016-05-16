#pragma once
#include "BufferManager.h"
#include "Record.hpp"

struct TID {
public:
	uint64_t pageId, slotId;
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


class SPSegment {
public:
	BufferManager* bm;
	int lastTochedFrame = 1;	// zero is reserved for the segment
	int curUsedPages = 0;	// how many pages are (partially) filled with user data
	int globalSidCounter = 0;
	SPSegment() ;

	~SPSegment();

	void printHex(unsigned char c);
	void printFrame(BufferFrame& frame);

	// for debugging purposes, prints all the currently inserted records
	void printAllRecords();

	// searchs and returns a frame with at least size + pointer empty space
	BufferFrame& getEmptyFrame(int size);
	TID insert(const Record& r);

	Record lookup(TID tid);

	bool remove(TID tid);

	bool update(TID tid, const Record& r);
};