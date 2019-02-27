#include "solver.h"


/*************************** DECLARE YOUR HELPER FUNCTIONS HERE ************************/





/*************************** solver.h functions ************************/


void seq_solver(unsigned int n, std::vector<std::vector<unsigned int> >& all_solns) {

	// TODO: Implement this function



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







