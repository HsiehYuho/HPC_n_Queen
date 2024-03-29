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
	std::vector<unsigned int> kill(k,n); // assume all processors agree on what the kill signal is
	std::vector<unsigned int> assignment;
	assignment.reserve(k);
	std::vector<unsigned int> flags (5*n-2 , 0);
	int num_killed_workers = 0;
	bool gen_success;
	bool skip = false;
	bool found_last = false;

	// Handle cases where excess workers can immediately be killed
	if (k == 0 || k == n) { // only 1 worker needed || no workers needed
		gen_success = false;
		skip = true;
		for (int ii = 1; ii < num_procs; ii++) {
			// for k=0 case, P1 will interpret kill signal differently: will produce full solutions then terminate itself
			MPI_Send(&kill[0], k, MPI_UNSIGNED, ii, 222, MPI_COMM_WORLD);
			num_killed_workers++;
		}
		if (k == n) {
			seq_solver(n, all_solns);
		} else {
			num_killed_workers--; // do not mark P1 as terminated yet for k=0 case
		}
	} else {
		gen_success = generate_partial(assignment, flags, n, k);
		if (!gen_success) { // immediately zero solutions found (e.g. n=2, k=2)
			for (int ii = 1; ii < num_procs; ii++) {
				MPI_Send(&kill[0], k, MPI_UNSIGNED, ii, 222, MPI_COMM_WORLD);
				num_killed_workers++;
			}
		return;
		}

		// Generate the initial partial solutions and distribute
		assignment.clear(); // reset
		assignment.reserve(k);
		std::fill(flags.begin(), flags.end(), 0); // reset
		for (int dest = 1; dest < num_procs; dest++) {
			if (!found_last) {
				gen_success = generate_partial(assignment, flags, n, k);
				if (gen_success) {
					MPI_Send(&assignment[0], k, MPI_UNSIGNED, dest, 222, MPI_COMM_WORLD);
				} else {// there are fewer partial solutions than there are workers, none left
					found_last = true;
					MPI_Send(&kill[0], k, MPI_UNSIGNED, dest, 222, MPI_COMM_WORLD);
					num_killed_workers++;
					skip = true;
				}
			} else {
				MPI_Send(&kill[0], k, MPI_UNSIGNED, dest, 222, MPI_COMM_WORLD);
				num_killed_workers++;
				skip = true;
			}
		}
	}

	MPI_Status stat;
	MPI_Request req;
	int recv_success = true;
	std::vector<unsigned int> soln(n);
	std::vector<unsigned int> soln_vec;
	unsigned int num_soln;
	std::vector<std::vector<unsigned int>> partials_surplus; // implemented like a LIFO queue, order of solutions doesn't matter
	std::vector<unsigned int> temp(k);

	while ((!found_last || !partials_surplus.empty()) && !skip) {
		// First receive the soln size from any processor that is ready, access its rank with stat
		if (recv_success) {
			MPI_Irecv(&num_soln, 1, MPI_UNSIGNED, MPI_ANY_SOURCE, 111, MPI_COMM_WORLD, &req);
		}
		MPI_Test(&req, &recv_success, &stat);
		if (recv_success) { // successful receive
			soln_vec.resize(num_soln*n);
			MPI_Recv(&soln_vec[0], num_soln*n, MPI_UNSIGNED, stat.MPI_SOURCE, 222, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			// Catch bug
//			if (soln_vec.size() != num_soln*n) {
//				std::cout << "Solution vector is expected to be size " << num_soln*n << " but is size " << soln_vec.size() << " upon receiving.\n" << std::endl;
//				for (unsigned int ii = 0; ii < soln_vec.size(); ii++) {
//					std::cout << soln_vec[ii] << ' ';
//				}
//				exit(EXIT_FAILURE);
//			}
			for (unsigned int ii = 0; ii < num_soln; ii++) {
				soln.assign(soln_vec.begin()+n*ii, soln_vec.begin()+n*(ii+1)-1);
				all_solns.push_back(soln);
			}
			// Immediately send a new assignment
			if (!partials_surplus.empty()) { // check surplus first
				temp = partials_surplus.back();
				MPI_Send(&temp[0], k, MPI_UNSIGNED, stat.MPI_SOURCE, 222, MPI_COMM_WORLD);
				partials_surplus.pop_back();
				continue;
			}
			if (!found_last) {
				gen_success = generate_partial(assignment, flags, n, k);
				if (gen_success) {
					MPI_Send(&assignment[0], k, MPI_UNSIGNED, stat.MPI_SOURCE, 222, MPI_COMM_WORLD);
				} else {
					MPI_Send(&kill[0], k, MPI_UNSIGNED, stat.MPI_SOURCE, 222, MPI_COMM_WORLD);
					num_killed_workers++;
					skip = true;
					found_last = true;
				}
			}
		} else {
			if (!found_last) {
				gen_success = generate_partial(assignment, flags, n, k);
				if (gen_success) {
					partials_surplus.push_back(assignment);
				} else {
					found_last = true;
				}
			}
		}

	}

	// Retrieve solutions from remaining workers and kill
	while (num_killed_workers != num_procs-1) {
		MPI_Recv(&num_soln, 1, MPI_UNSIGNED, MPI_ANY_SOURCE, 111, MPI_COMM_WORLD, &stat);
		soln_vec.resize(num_soln*n);
		MPI_Recv(&soln_vec[0], num_soln*n, MPI_UNSIGNED, stat.MPI_SOURCE, 222, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		for (unsigned int ii = 0; ii < num_soln; ii++) {
			soln.assign(soln_vec.begin()+n*ii, soln_vec.begin()+n*(ii+1)-1);
			all_solns.push_back(soln);
		}
		if (k != 0) { // P1 terminates itself for k=0 case, do not send kill signal
			MPI_Send(&kill[0], k, MPI_UNSIGNED, stat.MPI_SOURCE, 222, MPI_COMM_WORLD);
		}
		num_killed_workers++;
	}

	return;
}

void nqueen_worker(	unsigned int n,
					unsigned int k) {

	int proc_id;
	MPI_Comm_rank(MPI_COMM_WORLD, &proc_id);

	std::vector<unsigned int> assignment(k);
	std::vector<unsigned int> kill(k,n); // assume all processors agree on what the kill signal is
	std::vector<std::vector<unsigned int>> all_solns;
	std::vector<unsigned int> partial_solns(k);
	std::vector<unsigned int> soln_vec;
	bool fin = false;
	unsigned int num_solns;

	while (fin == false) {
		// Receive the assignment from P0
		MPI_Recv(&assignment[0], k, MPI_UNSIGNED, 0, 222, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		if (assignment == kill && k != 0) {
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

		// Calculate the remaining solution and store all solutions found in all_soln
		std::vector<std::vector<unsigned int>> all_solns;
		std::vector<unsigned int> partial_solns(k);
		partial_solns = assignment;
		seq_solver_backtrack(all_solns, partial_solns, flags, row, n, n, true);
//		std::cout << "\nNUM SOL FOUND: " << all_solns.size() << std::endl;

		// Tell P0 how many solutions to expect
		num_solns = all_solns.size();
		MPI_Send(&num_solns, 1, MPI_UNSIGNED, 0, 111, MPI_COMM_WORLD);

		// Concatenate all these solutions to send a single vector to P0
		soln_vec.clear();
		for (unsigned int ii = 0; ii < num_solns; ii++) {
			if (soln_vec.empty()) {
				soln_vec = all_solns[ii];
			} else {
				soln_vec.insert(soln_vec.end(), all_solns[ii].begin(), all_solns[ii].end());
//				soln_vec.insert(soln_vec.end(), std::make_move_iterator(all_solns[ii].begin()), std::make_move_iterator(all_solns[ii].end()));
//				std::move(std::begin(all_solns[ii]), std::end(all_solns[ii]), std::back_inserter(soln_vec));
//				std::copy(std::begin(all_solns[ii]), std::end(all_solns[ii]), std::back_inserter(soln_vec));
			}

		}

		// Catch bug
//		if 	(soln_vec.size() != num_solns * n) {
//			std::cout << "Solution vector is expected to be size " << n*num_solns << " but is size " << soln_vec.size() << " upon sending." << std::endl;
//			exit(EXIT_FAILURE);
//		} else {
//			std::cout << "\nSending concat solutions: " << std::endl;
//			for (unsigned int ii = 0; ii < num_solns * n; ii++) {
//				std::cout << soln_vec[ii] << ' ';
//			}
//		}

		// Send these solutions
		MPI_Send(&soln_vec[0], num_solns*n, MPI_UNSIGNED, 0, 222, MPI_COMM_WORLD);

		// Terminate itself if k=0
		if (k == 0) {
			fin = true;
		}
	}
	return;

}



/*************************** DEFINE YOUR HELPER FUNCTIONS HERE ************************/
bool generate_partial(std::vector<unsigned int> &soln,
					   std::vector<unsigned int> &flags,
					   unsigned int n,
					   unsigned int k) {
	// Catch bugs
	if (k == 0) {
		std::cout << "Error: Input value for k is invalid. Must be greater than 0." << std::endl;
		exit(EXIT_FAILURE);
	}

	// Handle base case, find the first partial solution
	if (soln.empty()) {
		std::vector<unsigned int> partial_soln;
		std::vector<std::vector<unsigned int>> first_soln;
		seq_solver_backtrack(first_soln, partial_soln, flags, 0, n, k, false);
		soln = first_soln[0];
		return true;
	}

	// Catch bugs
	if (soln.size() != k) {
		std::cout << "Error: Size of previous solution input to generate_partial() is " << soln.size() << ". Expected size is " << k << std::endl;
		exit(EXIT_FAILURE);
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
