// compile with c11 flag
// g++ main.cpp -std=c++11

#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <unistd.h>

#include <algorithm>
#include <functional>
#include <array>
#include <iostream>

using namespace std;

class RandomLong
{
   /// The state
   uint64_t state;

   public:
   /// Constructor
   explicit RandomLong(uint64_t seed=88172645463325252ull) : state(seed) {}

   /// Get the next value
   //uint64_t next() { state^=(state<<13); state^=(state>>7); return (state^=(state<<17)); }
   uint64_t next() { state^=(state<<13); state^=(state>>7); return state / 1E17;}
};


int main(int argc, char* argv[]) {
  	
  	uint64_t memSize = 88;  // how much main memory i can use
  	int64_t nElMem = memSize / sizeof(uint64_t);	// how many items i can store in memory
  	int n = 100;
  	cout << "will compute blocks of " << nElMem << endl;
	int64_t k = ceil((double) n / nElMem);
	cout << "K is "<<k <<endl;
	RandomLong rand;

	int fd, ret, fdTmp, fdOut;
	if ((fd = open("bla.txt", O_CREAT|O_TRUNC|O_WRONLY, S_IRUSR|S_IWUSR)) < 0) {
		std::cerr << "cannot open file '" << argv[1] << "': " << strerror(errno) << std::endl;
		return -1;
	}
	
	if ((ret = posix_fallocate(fd, 0, n*sizeof(uint64_t))) != 0)
		std::cerr << "warning: could not allocate file space: " << strerror(ret) << std::endl;
	for (unsigned i=0; i<n; ++i) {
		uint64_t x = rand.next();
		cout << x << " | ";
		if (write(fd, &x, sizeof(uint64_t)) < 0) {
			std::cout << "error writing to " << argv[1] << ": " << strerror(errno) << std::endl;
		}
	}
	cout << endl;
	close(fd);


	// open the file again
	if ((fd = open("bla.txt", O_RDWR)) < 0) {
		std::cerr << "cannot open file '" << argv[1] << "': " << strerror(errno) << std::endl;
		return -1;
	}


	// output file

	if ((fdTmp = open("out.txt",  O_CREAT|O_TRUNC|O_RDWR, S_IRUSR|S_IWUSR)) < 0) {
		std::cerr << "cannot open file " << endl;
		return -1;
	}
	uint64_t elPos = 0;
	int64_t remainEl = n;	// how many elements had not been proceeded so far
	uint64_t* val = (uint64_t*) malloc(sizeof(uint64_t) * nElMem);	// create the buffer
	while (remainEl > 0) {
		int64_t curReadEl = min(remainEl, nElMem);
		cout << "read chunk: ";
		read(fd, val, sizeof(uint64_t) * curReadEl);	// read a block

		for (int i = 0; i < curReadEl; i++) {
			cout << val[i] << ", ";
		}
		cout << endl;

		uint64_t tmp;
		// stupid O(n2) sort
		for (int i = 0; i < curReadEl; i++) {
			for (int j = i + 1; j < curReadEl; j++) {
				if (val[i] > val[j]) {	// swap
					tmp = val[j];
					val[j] = val[i];
					val[i] = tmp;
				}
			}
		}
		cout << "sorted chunk: ";
		for (int i = 0; i < curReadEl; i++) {
			//val[i] = i;
			cout << val[i] << ", ";
		}
		cout << endl;

		if (write(fdTmp, val, sizeof(uint64_t) * curReadEl) < 0) {
			std::cout << "error writing to ";
		}
		remainEl -= nElMem;
	}
	

	// write it back	

	//close(fdTmp);

	

	//n in nElMem
	cout << "calc "<<nElMem << " / "<< (k+1)<<endl;
	int64_t item_per_chunk = nElMem / (k + 1);
	cout << "per chunk" << item_per_chunk << endl;
	if (item_per_chunk < 1) {
		cerr << "not enough memory to store at lease one item per chunk." << endl;
		return -1;
	}

	for (int i = 0; i < k; i++) {	// read the chunks into memory
		//lseek(fdTmp, 2, 1);	// put file descriptor at right position
		if (lseek(fdTmp, i * sizeof(uint64_t) * nElMem	, SEEK_SET) < 0) {
			cerr << "error lseek  " << strerror(errno) << std::endl;
		}
		read(fdTmp, &val[i * item_per_chunk], sizeof(uint64_t) * item_per_chunk);	// read a block
		cout << "this chunk: ";
		for (int j = 0; j < item_per_chunk; j++) {
			cout << val[i * item_per_chunk + j]<<",";
		}
		cout << endl;
		//cout << "did read "<<i<<": "<<val[i]<<endl;
	}
	cout << "did read ";
	for (int i = 0; i < item_per_chunk * (k+1); i++) {
		cout << val[i] << ", ";
	}
	cout << endl;


	if ((fdOut = open("sorted.txt",  O_CREAT|O_TRUNC|O_RDWR, S_IRUSR|S_IWUSR)) < 0) {
		std::cerr << "cannot open file " << endl;
		return -1;
	}


	int64_t* offsets = (int64_t*) calloc(k + 1, sizeof(int64_t));	// get an offset pointer for efery chunk
	uint64_t* file_offsets = (uint64_t*) calloc(k, sizeof(uint64_t));	// offsets in the file
	// now compare and find the min val
	


// merge
	for (int l = 0; l < n; l++) {
		uint64_t min_val = UINT64_MAX;
		uint64_t idx_min = -1;
		for (int i = 0; i < k; i++) {
			//cout << "compare to "<<val[i*item_per_chunk];
			
			if (offsets[i]!= -1 && val[i * item_per_chunk + offsets[i]] < min_val) {	// still a valid chunk?
				min_val = val[i * item_per_chunk + offsets[i]];
				idx_min = i;
			}
		}

		
		cout << "offsets "<<offsets[0]<<", "<<offsets[1]<<", "<<offsets[2]<<endl;
		cout << "fileO"<<file_offsets[0]<<", "<<file_offsets[1]<<", "<<file_offsets[2]<<endl;
		cout << "@"<<l<<"min is "<<min_val << "at idx" << idx_min<<endl;


		// take this value to the buffer
		val[k * item_per_chunk + offsets[k]] = min_val;
		// constrains, check if buffer is filled
		offsets[k]++;
		if (offsets[k] >= item_per_chunk) {
			cout << "buffer reached its limit, write to file " << endl;
			write(fdOut, &val[k * item_per_chunk], sizeof(uint64_t) * item_per_chunk);	// write output buffer
			offsets[k] = 0;
		}
		// constrin source buffer
		offsets[idx_min]++;
		

		if (offsets[idx_min] + file_offsets[idx_min] >= nElMem) {
			cout << "this chunk finished"<<endl;
			offsets[idx_min] = -1;

		} else if (offsets[idx_min] >= item_per_chunk || offsets[idx_min] + file_offsets[idx_min] + idx_min * nElMem >= n) {	// array out of bounds, so either the next chunk, ot after the last
			cout << "input buffer empy, refill" << endl;

			file_offsets[idx_min] += item_per_chunk;
			cout << "file offset"<<idx_min<<" "<< file_offsets[idx_min]<<endl;
			// todo check if this reaches another limit
			cout << "compare "<<(offsets[idx_min] + file_offsets[idx_min] + idx_min * nElMem) << "vs "<<n <<endl;
			if (file_offsets[idx_min] >= nElMem || // at the end of the current chunk
				offsets[idx_min] + file_offsets[idx_min] + idx_min * nElMem - item_per_chunk >= n) {	
				cout << "this chunk is finished, set offset to -1"<<endl;
				offsets[idx_min] = -1;	// signal this chunk to be finished with this flag
				cout << "offsets in if "<<offsets[0]<<", "<<offsets[1]<<endl;
			} else {	// read onother chunk
				if (lseek(fdTmp, idx_min * sizeof(uint64_t) * (nElMem) + file_offsets[idx_min] * sizeof(uint64_t), SEEK_SET) < 0) {
					cerr << "error lseek  " << strerror(errno) << std::endl;
				}
				// todo: implement reading out of files, this actually reads more than allowed
				offsets[idx_min] = 0;
				read(fdTmp, &val[idx_min * item_per_chunk], sizeof(uint64_t) * item_per_chunk);	// read a block
			}
			
		}

		// output 
		for (int i = 0; i < nElMem; i++) {
			cout << val[i]<< " , ";
		}
		cout << endl;

		
	}
	// flush the remaining output buffer
	cout << "remaining offset is "<<offsets[3]<<endl;
	if (offsets[k] > 0) {
		write(fdOut, &val[k * item_per_chunk], sizeof(uint64_t) * offsets[k]);	// write output buffer
	}

	// test read
	if ((fd = open("sorted.txt", O_RDWR)) < 0) {
		std::cerr << "cannot open file '" << endl;
		return -1;
	}

	uint64_t* val2 = (uint64_t*)malloc(sizeof(uint64_t) * n);
	read(fd, val2, sizeof(uint64_t) * n);
	for (int i = 0; i < n; i++) {
		cout  << val2[i] << " | ";
	}
	cout << endl;

	cout << "\n";
	return 0;

}