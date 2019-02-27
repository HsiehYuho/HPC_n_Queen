#include "solver.h"


/*************************** DECLARE YOUR HELPER FUNCTIONS HERE ************************/


// Backtrack function for 
void seq_solver_backtrack(std::vector<std::vector<unsigned int> >& all_solns,
						  std::vector<unsigned int> &partial_soln,
						  std::vector<unsigned int> &flags,
						  unsigned int row, 
						  unsigned int& n
						  );


/*************************** solver.h functions ************************/


void seq_solver(unsigned int n, std::vector<std::vector<unsigned int> >& all_solns) {
	std::vector<unsigned int> partial_soln;
	std::vector<unsigned int> flags (5*n-2 , 0);
	seq_solver_backtrack(all_solns, partial_soln, flags, 0, n);
	return;
}

// The flags[0] to flags[n-1] indicates if the column had a queen
// The flags[n] to flags[3n-2], (2n - 1) 45 degree distinct lines
// The flags[3n-1] to flags[5n-3], (2n - 1) 135 degree distinct lines 
void seq_solver_backtrack(std::vector<std::vector<unsigned int> >& all_solns,
						  std::vector<unsigned int> &partial_soln,
						  std::vector<unsigned int> &flags,
						  unsigned int row, 
						  unsigned int& n
						  ){

	if(row == n){
		std::vector<unsigned int> copy_soln;
		copy_soln = partial_soln;
		all_solns.push_back(copy_soln);
		return;
	}

	for(int col = 0; col < n; col++){
		if(flags[col] == 0 && flags[n + row + col] == 0 && flags[4 * n - 2 + col - row] == 0){
			flags[col] = flags[n + row + col] = flags[4 * n - 2 + col - row] = 1;
			partial_soln.push_back(col);
			seq_solver_backtrack(all_solns, partial_soln, flags, row + 1, n);
			partial_soln.pop_back();
			flags[col] = flags[n + row + col] = flags[4 * n - 2 + col - row] = 0;
		}
	}

}



void nqueen_master(	unsigned int n,
					unsigned int k,
					std::vector<std::vector<unsigned int> >& all_solns) {

	// TODO: Implement this function

	/* Following is a general high level layout that you can follow
	 (you are not obligated to design your solution in this manner.
	  This is provided just for your ease). */


	/******************* STEP 1: Send one partial solution to each worker ********************/
	/*
	 * for (all workers) {
	 * 		- create a partial solution.
	 * 		- send that partial solution to a worker
	 * }
	 */


	/******************* STEP 2: Send partial solutions to workers as they respond ********************/
	/*
	 * while() {
	 * 		- receive completed work from a worker processor.
	 * 		- create a partial solution
	 * 		- send that partial solution to the worker that responded
	 * 		- Break when no more partial solutions exist and all workers have responded with jobs handed to them
	 * }
	 */

	/********************** STEP 3: Terminate **************************
	 *
	 * Send a termination/kill signal to all workers.
	 *
	 */





}

void nqueen_worker(	unsigned int n,
					unsigned int k) {



	// TODO: Implement this function

	// Following is a general high level layout that you can follow (you are not obligated to design your solution in this manner. This is provided just for your ease).

	/*******************************************************************
	 *
	 * while() {
	 *
	 * 		wait for a message from master
	 *
	 * 		if (message is a partial job) {
	 *				- finish the partial solution
	 *				- send all found solutions to master
	 * 		}
	 *
	 * 		if (message is a kill signal) {
	 *
	 * 				quit
	 *
	 * 		}
	 *	}
	 */


}



/*************************** DEFINE YOUR HELPER FUNCTIONS HERE ************************/







