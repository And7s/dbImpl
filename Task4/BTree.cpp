#include "SPSegment.h"

using namespace std;
struct TreeHeader {
	u_int amount;
	u_int leftNode;
	bool isLeaf;
};


template <typename T>
struct TreeNode {
	T key;	
	TID tid;	// pointer to a slottedpage or the page where the children elements are
};


template <typename T, typename CMP> class BTree {
private: 
	BufferManager* bm;
	u_int rootIdx, curUsedPages;
	u_int numEl;
public:
	BTree();
	void insert(T key, TID tid);
	bool lookup(T key, TID& tid);
	void erase(T key);
	void printHex(unsigned char c);
	void printFrame(BufferFrame& frame);
	TreeNode<T>* getNode(BufferFrame& frame, u_int idx);
	char* getPointer(BufferFrame& frame, u_int idx);
	void splitFrames(BufferFrame& parent, BufferFrame& child, u_int updateIdx);
	u_int size();
};

template <typename T, typename CMP>
BTree<T, CMP>::BTree() {
	bm = new BufferManager(30);
	rootIdx = 0;
	curUsedPages = 1;
	BufferFrame& frame = bm->fixPage(rootIdx, true);
	TreeHeader* th = (TreeHeader*) frame.getData();
	th->amount = 0;
	th->isLeaf = true;
	bm->unfixPage(frame, true);
	cout << "constr btree " << endl;
	numEl = 0;
}

template <typename T, typename CMP>
u_int BTree<T, CMP>::size() {
	return numEl;
}

template <typename T, typename CMP>
void BTree<T, CMP>::insert(T key, TID tid) {
	//cout << "called with key "<<key<<endl;

	u_int curIdx = rootIdx;
	
	while (true) {
		BufferFrame& frame = bm->fixPage(curIdx, true);
		TreeHeader* th = (TreeHeader*) frame.getData();
		
		// there must be at least one free spot -> early split
		int size = pageSize - sizeof(TreeHeader) - sizeof(TreeNode<T>) * th->amount;
		if (size < sizeof(TreeNode<T>)) {
			// split the root
			BufferFrame& newFrame = bm->fixPage(curUsedPages, true);
th->isLeaf = true;
			TreeHeader* newTh = (TreeHeader*) newFrame.getData();
			newTh->amount = 0;
			newTh->leftNode = rootIdx;
			newTh->isLeaf = false;

			rootIdx = curUsedPages;
			curUsedPages++;
			splitFrames(newFrame, frame, 0);
			
			bm->unfixPage(frame, true);
			bm->unfixPage(newFrame, true);

			curIdx = rootIdx;
			th = (TreeHeader*) frame.getData();

		continue;
		} else {

			if (th->isLeaf) {
				
				TreeNode<T> tl;
				tl.key = key;
				tl.tid = tid;
				int insertAt = th->amount;
				for (int i = 0; i < th->amount; i++) {
					TreeNode<T>* leaf = getNode(frame, i);
					//cout << "compare "<<tl.key << " "<< leaf->key<<endl;
					if (tl.key < leaf->key) {
						insertAt = i;
						break;
					}
				}
				// move everything after
				u_int elAfter = th->amount - insertAt;
				memmove(
					getPointer(frame, insertAt + 1),
					getPointer(frame, insertAt),
					sizeof(TreeNode<T>) * elAfter
				);
				
				memcpy(
					getPointer(frame, insertAt),
					&tl, sizeof(TreeNode<T>)
				);
				th->amount++;
				//printFrame(frame);
				numEl++;
				bm->unfixPage(frame, true);

				return;	// finally inserted
			} else {
				// search where to insert
				u_int insertAt = th->leftNode;
				u_int updateIdx = 0;
				for (int i = 0; i < th->amount; i++) {
					TreeNode<T>* node = getNode(frame, i);
					if (key >= node->key) {	// to this right side
						insertAt = node->tid.pageId;
						updateIdx = i + 1;
					} else {
						break;	// already found
					}
				}
				curIdx = insertAt;

				// get child and check if size is sufficent
				
				BufferFrame& child = bm->fixPage(insertAt, true);
				TreeHeader* thChild = (TreeHeader*) child.getData();
				int size = pageSize - sizeof(TreeHeader) - sizeof(TreeNode<T>) * thChild->amount;
				
				if (size < sizeof(TreeNode<T>)) {
					splitFrames(frame, child, updateIdx);
					// will start searching on the upper level again
					curIdx = frame.pageId;
				}
				bm->unfixPage(child, true);
				
			}

			//printFrame(frame);
			bm->unfixPage(frame, true);
		}
	}
}

template <typename T, typename CMP>
void BTree<T, CMP>::splitFrames(BufferFrame& parent, BufferFrame& child, u_int updateIdx) {
	TreeHeader* thParent = (TreeHeader*) parent.getData();
	TreeHeader* thChild = (TreeHeader*) child.getData();

	u_int splitAt = thChild->amount / 2;
	TreeNode<T>* splitEl = getNode(child, splitAt);	// copy the pointer elment one level atop

	BufferFrame& rightPage = bm->fixPage(curUsedPages, true);
	TreeHeader* rightTh = (TreeHeader*) rightPage.getData();
	rightTh->amount = thChild->amount - splitAt;
	rightTh->isLeaf = true;
	curUsedPages++;
	memcpy(getPointer(rightPage, 0), splitEl, sizeof(TreeNode<T>) * rightTh->amount);	// parent pointer
	//printFrame(rightPage);
	bm->unfixPage(rightPage, true);

	thChild->amount = splitAt;
	//printFrame(child);

	u_int elAfter = thParent->amount - updateIdx;
	memmove(
		getPointer(parent, updateIdx + 1),
		getPointer(parent, updateIdx),
		sizeof(TreeNode<T>) * elAfter
	);

	TreeNode<T>* rightPointer = getNode(parent, updateIdx);
	rightPointer->tid.pageId = curUsedPages - 1;
	rightPointer->key = splitEl->key;
	thParent->amount++;
}

template <typename T, typename CMP>
char* BTree<T, CMP>::getPointer(BufferFrame& frame, u_int idx) {
	return (char*)frame.getData() + sizeof(TreeHeader) + sizeof(TreeNode<T>) * idx;
}

template <typename T, typename CMP>
TreeNode<T>* BTree<T, CMP>::getNode(BufferFrame& frame, u_int idx) {
	return (TreeNode<T>*)getPointer(frame, idx);
}

template <typename T, typename CMP>
void BTree<T, CMP>::erase(T key) {

	u_int curIdx = rootIdx;
	
	while (true) {
		
		BufferFrame& frame = bm->fixPage(curIdx, true);
		TreeHeader* th = (TreeHeader*) frame.getData();
		

		u_int lookupAt = th->leftNode;
		u_int lookupIdx = 0;
		for (int i = 0; i < th->amount; i++) {
			TreeNode<T>* node = getNode(frame, i);
			if (key >= node->key) {	// to this right side
				lookupAt = node->tid.pageId;
				lookupIdx = i;
			} else {
				break;	// already found
			}
		}

		if (th->isLeaf) {
			TreeNode<T>* node = getNode(frame, lookupIdx);
			if (key == node->key) {
				
				u_int elAfter = th->amount - lookupIdx - 1;
				memmove(
					getPointer(frame, lookupIdx),
					getPointer(frame, lookupIdx + 1),
					sizeof(TreeNode<T>) * elAfter
				);
				numEl--;
				th->amount--;
				//printFrame(frame);
				bm->unfixPage(frame, true);
				return;


			} else {
				cout << "not fount"<<endl;
				bm->unfixPage(frame, true);
				// how can i indicate?
				return;
				
			}
		}
		bm->unfixPage(frame, true);
		curIdx = lookupAt;
	}
}

template <typename T, typename CMP>
bool BTree<T, CMP>::lookup(T key, TID& tid) {
	//cout << "lookup with key "<<key<<endl;
	u_int curIdx = rootIdx;
	while (true) {
		
		BufferFrame& frame = bm->fixPage(curIdx, true);
		TreeHeader* th = (TreeHeader*) frame.getData();
		
		u_int lookupAt = th->leftNode;
		u_int lookupIdx = 0;
		for (int i = 0; i < th->amount; i++) {
			TreeNode<T>* node = getNode(frame, i);
			if (key >= node->key) {	// to this right side
				lookupAt = node->tid.pageId;
				lookupIdx = i;
			} else {
				break;	// already found
			}
		}

		if (th->isLeaf) {
			TreeNode<T>* node = getNode(frame, lookupIdx);
			if (key == node->key) {
				bm->unfixPage(frame, true);
				tid.pageId = node->tid.pageId;
				tid.slotId = node->tid.slotId;
				return true;	// this could be dangeous, as the memory could be freed by the bm
			} else {
				bm->unfixPage(frame, true);
				return false;
			}
		} else {
			bm->unfixPage(frame, true);
		}
		curIdx = lookupAt;
	}

}

template <typename T, typename CMP>
void BTree<T, CMP>::printHex(unsigned char c) {
	if (c < 16) {
		cout  << " " << (int)c;
	} else {
		cout << (int)c;
	}
}

template <typename T, typename CMP>
void BTree<T, CMP>::printFrame(BufferFrame& frame) {
	cout << std::hex;
	for (int i = 0; i < pageSize; i++) {
		if (i % 32 == 0) cout << endl;
		printHex(((unsigned char*)frame.getData())[i]);
	}
	cout << dec << endl;
}
