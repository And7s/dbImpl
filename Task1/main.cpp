// compile with c11 flag
// g++ main.cpp -std=c++11

#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <algorithm>
#include <iostream>

using namespace std;

int externalSort(int fdInput, uint64_t size, int fdOutput, uint64_t memSize) {

	
  	int64_t nElMem = memSize / sizeof(uint64_t);	// how many items i can store in memory
  	int64_t n =  size;
  	cout << "will compute blocks of " << nElMem << endl;
	int64_t k = ceil((double) n / nElMem);
	cout << "K is "<<k <<endl;

	//n in nElMem
	cout << "calc "<<nElMem << " / "<< (k+1)<<endl;
	int64_t item_per_chunk = nElMem / (k + 1);
	cout << "per chunk" << item_per_chunk << endl;
	if (item_per_chunk < 1) {
		cerr << "not enough memory to store at lease one item per chunk." << endl;
		return -1;
	}
	uint64_t* val = (uint64_t*) malloc(sizeof(uint64_t) * nElMem);	// create the buffer

	// output file
	int fdTmp;
	if ((fdTmp = open("~.tmp",  O_CREAT|O_TRUNC|O_RDWR, S_IRUSR|S_IWUSR)) < 0) {
		std::cerr << "cannot open file " << endl;
		return -1;
	}
	uint64_t elPos = 0;
	int64_t remainEl = n;	// how many elements had not been proceeded so far
	
	while (remainEl > 0) {
		int64_t curReadEl = min(remainEl, nElMem);
		read(fdInput, val, sizeof(uint64_t) * curReadEl);	// read a block

		sort(val, val +curReadEl);

		if (write(fdTmp, val, sizeof(uint64_t) * curReadEl) < 0) {
			std::cout << "error writing to ";
		}
		remainEl -= nElMem;
		cout << "rem "<<remainEl<<endl;
	}
	
	cout << "did sort each chunk individually"<<endl;
	
	for (int i = 0; i < k; i++) {	// read the chunks into memory
		if (lseek(fdTmp, i * sizeof(uint64_t) * nElMem	, SEEK_SET) < 0) {
			cerr << "error lseek  " << strerror(errno) << std::endl;
		}
		read(fdTmp, &val[i * item_per_chunk], sizeof(uint64_t) * item_per_chunk);	// read a block
	}

	int64_t* offsets = (int64_t*) calloc(k + 1, sizeof(int64_t));	// get an offset pointer for efery chunk
	uint64_t* file_offsets = (uint64_t*) calloc(k, sizeof(uint64_t));	// offsets in the file
	// now compare and find the min val
	
	// merge
	for (uint64_t l = 0; l < n; l++) {
		uint64_t min_val = UINT64_MAX;
		uint64_t idx_min = -1;
		for (int i = 0; i < k; i++) {		
			if (offsets[i]!= -1 && val[i * item_per_chunk + offsets[i]] < min_val) {	// still a valid chunk?
				min_val = val[i * item_per_chunk + offsets[i]];
				idx_min = i;
			}
		}

		// take this value to the buffer
		val[k * item_per_chunk + offsets[k]] = min_val;
		// constrains, check if buffer is filled
		offsets[k]++;
		if (offsets[k] >= item_per_chunk) {
			cout << "buffer reached its limit, write to file " << l << endl;
			write(fdOutput, &val[k * item_per_chunk], sizeof(uint64_t) * item_per_chunk);	// write output buffer
			offsets[k] = 0;
		}
		// constrin source buffer
		offsets[idx_min]++;
		

		if (offsets[idx_min] + file_offsets[idx_min] >= nElMem) {
			offsets[idx_min] = -1;

		} else if (offsets[idx_min] >= item_per_chunk || offsets[idx_min] + file_offsets[idx_min] + idx_min * nElMem >= n) {	// array out of bounds, so either the next chunk, ot after the last
			file_offsets[idx_min] += item_per_chunk;
			
			// todo check if this reaches another limit
			
			if (file_offsets[idx_min] >= nElMem || // at the end of the current chunk
				offsets[idx_min] + file_offsets[idx_min] + idx_min * nElMem - item_per_chunk >= n) {	
				// finished chunk, set flag
				offsets[idx_min] = -1;	// signal this chunk to be finished with this flag
				
			} else {	// read onother chunk
				if (lseek(fdTmp, idx_min * sizeof(uint64_t) * (nElMem) + file_offsets[idx_min] * sizeof(uint64_t), SEEK_SET) < 0) {
					cerr << "error lseek  " << strerror(errno) << std::endl;
				}
				// todo: implement reading out of files, this actually reads more than allowed
				offsets[idx_min] = 0;
				read(fdTmp, &val[idx_min * item_per_chunk], sizeof(uint64_t) * item_per_chunk);	// read a block
			}
			
		}		
	}
	cout << "merging finished, write ermaining " << offsets[k] << endl;
	// flush the remaining output buffer
	if (offsets[k] > 0) {
		write(fdOutput, &val[k * item_per_chunk], sizeof(uint64_t) * offsets[k]);	// write output buffer
	}

	// cleanup
	close(fdTmp);
	free(val);
	free(offsets);
	free(file_offsets);
}

int main(int argc, char* argv[]) {
  	if (argc != 4) {
  		cout << "sort inputFile outputFile memoryBufferInMB" << endl;
  		return -1;
  	}
	
	uint64_t n = 1000 * 1000 * 625;	// 5GB / 8B = 625Mio elements	
	uint64_t memSize = 1000 * 1000 * atoi(argv[3]);  // how much main memory i can use
	uint64_t nElMem = memSize / sizeof(uint64_t);

	int fdIn, fdOut;
	// open the inputFile
	if ((fdIn = open(argv[1], O_RDWR)) < 0) {
		std::cerr << "cannot open file '" << argv[1] << "': " << strerror(errno) << std::endl;
		return -1;
	}

	// open the outputFile
	if ((fdOut = open(argv[2],  O_CREAT|O_TRUNC|O_RDWR, S_IRUSR|S_IWUSR)) < 0) {
		std::cerr << "cannot create file " << argv[2] << endl;
		return -1;
	}

	externalSort(fdIn, n, fdOut, memSize);
	close(fdIn);

	uint64_t* val = (uint64_t*) malloc(sizeof(uint64_t) * nElMem);	// create the buffer

	cout << "test now" << endl;
	
	bool valid = true;
	uint64_t i = 0;
	uint64_t idx = 0;
	lseek(fdOut, 0, SEEK_SET);
	while (i < n && valid) {
		
		if (i % nElMem == 0) {
			cout << "read chunk valid " << i << endl;
			uint64_t readEl = min((uint64_t) (n - i), nElMem);
			uint64_t last_val = val[nElMem - 1];
			
			read(fdOut, val, sizeof(uint64_t) * readEl);
			if (i > 0 && val[0] < last_val) {	// compare last el to fst el of new chunk
				cerr << "errr1 " << val[0] << "smaller " << last_val << " @" << i;
				valid = false;
			}
			idx = 0;
		} else {
			// todo check
			if (val[idx] < val[idx - 1]) {	
				cerr << "errr2 " << val[idx] << "smaller " << val[idx - 1] << " @" << idx;
				valid = false;
			}
		}
		i++;
		idx++;
		
	}
	cout << "valid? "<< valid;
	cout << endl;
	close(fdOut);
	free(val);
	cout << "\n";
	return 0;
}