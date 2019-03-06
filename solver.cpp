#include "solver.h"

/*************************** DECLARE YOUR HELPER FUNCTIONS HERE ************************/


// Backtrack function for 
void seq_solver_backtrack(std::vector<std::vector<unsigned int> >& all_solns,
						  std::vector<unsigned int> &partial_soln,
						  std::vector<unsigned int> &flags,
						  unsigned int row, 
						  unsigned int& n,
						  unsigned int& k,
						  bool return_all
						  );

// Function for master processor to generate all possible partial solutions of size k for an nxn grid
// Returns true if new partial solution found, false if no more partial solutions can be found
bool generate_partial(std::vector<unsigned int> &soln,
					   std::vector<unsigned int> &flags,
					   unsigned int n,
					   unsigned int k
					   );

/*************************** solver.h functions ************************/


void seq_solver(unsigned int n, std::vector<std::vector<unsigned int> >& all_solns) {
	std::vector<unsigned int> partial_soln;
	std::vector<unsigned int> flags (5*n-2 , 0);
	seq_solver_backtrack(all_solns, partial_soln, flags, 0, n, n, true);
	return;
}

// The flags[0] to flags[n-1] indicates if the column had a queen
// The flags[n] to flags[3n-2], (2n - 1) 45 degree distinct lines
// The flags[3n-1] to flags[5n-3], (2n - 1) 135 degree distinct lines
// return_all is set to false when this function is used by the parallel algorithm's master processor to obtain the initial partial solution
// set k = n for sequential solver
void seq_solver_backtrack(std::vector<std::vector<unsigned int> >& all_solns,
						  std::vector<unsigned int> &partial_soln,
						  std::vector<unsigned int> &flags,
						  unsigned int row,
						  unsigned int& n,
						  unsigned int& k,
						  bool return_all
						  ){

	if(row == k){
		std::vector<unsigned int> copy_soln;
		copy_soln = partial_soln;
		all_solns.push_back(copy_soln);
		return;
	}

	for(unsigned int col = 0; col < n; col++){
		if(flags[col] == 0 && flags[n + row + col] == 0 && flags[4 * n - 2 + col - row] == 0){
			flags[col] = flags[n + row + col] = flags[4 * n - 2 + col - row] = 1;
			partial_soln.push_back(col);
			seq_solver_backtrack(all_solns, partial_soln, flags, row + 1, n, k, return_all);
			if (return_all == false) {
				// stop after finding the first partial solution length k
				return;
			}
			partial_soln.pop_back();
			flags[col] = flags[n + row + col] = flags[4 * n - 2 + col - row] = 0;
		}
	}

}

void nqueen_master(	unsigned int n,
					unsigned int k,
					std::vector<std::vector<unsigned int> >& all_solns) {
	int num_procs; // total number of procs in the world
	MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
	std::vector<unsigned int> kill(k,0); // assume all processors agree on what the kill signal is
//	unsigned int size_tag = 111;
//	unsigned int sol_tag = 222;

	// Generate the first partial solution
	std::vector<unsigned int> assignment(k,0);
	std::vector<unsigned int> flags (5*n-2 , 0);
	int num_killed_workers = 0;
	bool partials_remaining;

	partials_remaining = generate_partial(assignment, flags, n, k);
	if (partials_remaining == false) { // immediately no solutions found (e.g. n=2)
		for (int ii = 1; ii < num_procs-1; ii++) {
			MPI_Send(&kill[0], k, MPI_UNSIGNED, ii, 222, MPI_COMM_WORLD);
			num_killed_workers++;
		}
		return;
	}

	for (int dest = 1; dest < num_procs; dest++) {
		if (partials_remaining == true) { // if assignment is valid, haven't run out yet
			MPI_Send(&assignment[0], k, MPI_UNSIGNED, dest, 222, MPI_COMM_WORLD);
//			for (int ii = 0; ii < assignment.size(); ii++) {
//				std::cout << assignment[ii] << std::endl;
//			}
//			std::cout << "sending initial assignment to" << dest << std::endl;
			partials_remaining = generate_partial(assignment, flags, n, k);
		} else { // there are fewer partial solutions than there are workers, none left
			MPI_Send(&kill[0], k, MPI_UNSIGNED, dest, 222, MPI_COMM_WORLD);
//			std::cout << "killed processor" << dest << std::endl;
			num_killed_workers++;
		}
	}

	MPI_Status stat;
	MPI_Request req;
	int test_flag;
	std::vector<unsigned int> soln(n);
	int num_soln;

	while (partials_remaining == true) {
		// First receive the soln size from any processor that is ready, access its rank with stat
		MPI_Irecv(&num_soln, 1, MPI_INT, MPI_ANY_SOURCE, 111, MPI_COMM_WORLD, &req);
		MPI_Test(&req, &test_flag, &stat);
		if (test_flag == 1) { // successful receive
			// Append to all_solns
			for (int ii = 0; ii < num_soln; ii++) {
				MPI_Recv(&soln[0], n, MPI_UNSIGNED, stat.MPI_SOURCE, 222, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				all_solns.push_back(soln);
			}
			// Immediately send a new assignment
			partials_remaining = generate_partial(assignment, flags, n, k);
			if (partials_remaining == true) {
				MPI_Send(&assignment[0], k, MPI_UNSIGNED, stat.MPI_SOURCE, 222, MPI_COMM_WORLD);
//				std::cout << "\nsending assignment ";
//				for (int xx = 0; xx < assignment.size(); xx++) {
//					std::cout << assignment[xx] << ' ';
//				}
			} else {
				MPI_Send(&kill[0], k, MPI_UNSIGNED, stat.MPI_SOURCE, 222, MPI_COMM_WORLD);
				num_killed_workers++;
				continue;
			}
		} else { // all workers are busy, nobody sent anything to P0
			std::cout << " all workers are busy" << std::endl;
		}

	}

	// Retrieve solutions from remaining workers and kill
	while (num_killed_workers != num_procs-1) {
		MPI_Recv(&num_soln, 1, MPI_INT, MPI_ANY_SOURCE, 111, MPI_COMM_WORLD, &stat);
		for (int ii = 0; ii < num_soln; ii++) {
			MPI_Recv(&soln[0], n, MPI_UNSIGNED, stat.MPI_SOURCE, 222, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			all_solns.push_back(soln);
		}
		MPI_Send(&kill[0], k, MPI_UNSIGNED, stat.MPI_SOURCE, 222, MPI_COMM_WORLD);
		num_killed_workers++;
	}

	return;
}

void nqueen_worker(	unsigned int n,
					unsigned int k) {

//	int proc_id;
//	MPI_Comm_rank(MPI_COMM_WORLD, &proc_id);

	std::vector<unsigned int> assignment(k);
	std::vector<unsigned int> kill(k,0); // assume all processors agree on what the kill signal is
//	unsigned int size_tag = 111;
//	unsigned int sol_tag = 222;
	bool fin = false;

	while (fin == false) {
		// Receive the assignment from P0
		MPI_Recv(&assignment[0], k, MPI_UNSIGNED, 0, 222, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		if (assignment == kill) {
			fin = true;
			continue;
		}

		// Populate flags with known information from the assignment
		int row = 0;
		std::vector<unsigned int> flags (5*n-2 , 0);
		for(std::vector<unsigned int>::iterator col = assignment.begin(); col != assignment.end(); ++col) {
			// each element is the col# of the queen in the row'th row
			flags[*col] = flags[n + row + *col] = flags[4 * n - 2 + *col - row] = 1;
			row++;
		}

		// Calculate the remaining solution and store all solutions found in all_solns
		std::vector<std::vector<unsigned int>> all_solns;
		std::vector<unsigned int> partial_solns(k);
		partial_solns = assignment;
		seq_solver_backtrack(all_solns, partial_solns, flags, row, n, n, true);

		// Tell P0 how many solutions to expect
		int num_solns = all_solns.size();
		MPI_Send(&num_solns, 1, MPI_INT, 0, 111, MPI_COMM_WORLD);

		// Send these solutions
//		std::cout << "sending " << num_solns << " solutions from " << proc_id << std::endl;
		for (int ii = 0; ii < num_solns; ii++) {
			MPI_Send(&all_solns[ii][0], n, MPI_UNSIGNED, 0, 222, MPI_COMM_WORLD);
		}
	}
	return;

}



/*************************** DEFINE YOUR HELPER FUNCTIONS HERE ************************/
bool generate_partial(std::vector<unsigned int> &soln,
					   std::vector<unsigned int> &flags,
					   unsigned int n,
					   unsigned int k) {
	// Handle base case, find the first partial solution
	std::vector<unsigned int> empty(k,0);
	if (soln == empty) {
		std::vector<unsigned int> partial_soln;
		std::vector<std::vector<unsigned int>> first_soln;
		seq_solver_backtrack(first_soln, partial_soln, flags, 0, n, k, false);
		soln = first_soln[0];
		return true;
	}

	// Clear the flag from the previous solution
	unsigned int col = soln.back();
	unsigned int row = k-1;
	flags[col] = flags[n + row + col] = flags[4 * n - 2 + col - row] = 0;
	soln.pop_back();
	col++;

	while (col <= n) {
		// Reached end of row k
		if (col == n) {
			if (soln.size() > 0) {
				col = soln.back();
				row--;
				flags[col] = flags[n + row + col] = flags[4 * n - 2 + col - row] = 0;
				soln.pop_back();
				col++;
				continue;
			} else {
				return false;
			}
		}

		// Found new valid solution
		if(flags[col] == 0 && flags[n + row + col] == 0 && flags[4 * n - 2 + col - row] == 0) {
			flags[col] = flags[n + row + col] = flags[4 * n - 2 + col - row] = 1;
			soln.push_back(col);
			if (row == k-1) {
				return true;
			} else {
				col = 0;
				row++;
				continue;
			}
		// Invalid solution, keep searching
		} else {
			col++;
		}
	}

	std::cout << "Error: reached end of generate_partial" << std::endl;
	exit(EXIT_FAILURE);
}
