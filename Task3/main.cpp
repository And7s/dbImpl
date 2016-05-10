#include <iostream>
#include <vector>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <pthread.h>

#include "BufferManager.h"

/*

*/
using namespace std;
int main(int argc, char** argv) {

	BufferManager* bm = new BufferManager(10);
	bm->fixPage(0, false);

	cout << "hi";

}