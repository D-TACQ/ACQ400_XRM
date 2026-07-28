/* deque_test .. keep newest item at [0] (front), drop oldest (back),
   we're really interested in [0], but keep a short history in case it's needed

pgm@peter-XPS-13-7390:~/SANDBOX/DEQUE$ make deque_test
g++     deque_test.cpp   -o deque_test
pgm@peter-XPS-13-7390:~/SANDBOX/DEQUE$ ./deque_test 
pgmwashere
4 3 2 1 
5 4 3 2 
6 5 4 3 
7 6 5 4 
8 7 6 5 
*/
#include <iostream>
#include <deque>

int main(int argc, char* argv[]) {
	std::cout << "pgmwashere\n";
	
	std::deque<int> dq;

	// pre-fill push_front so that the latest item is [0]
	for (int ii = 1; ii < 5; ++ii){
		dq.push_front(ii);
	}
        // run, always show most recent first, remove oldest
	for (int ii = 5; ii < 10; ++ii){
		for (int ii = 0; ii < 4; ++ii){
			std::cout << dq[ii] << " ";
		}
		std::cout << "\n";
		dq.pop_back();
		dq.push_front(ii);
	}

	return 0;
}
