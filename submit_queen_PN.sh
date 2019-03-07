#!/bin/bash
for P in 8; do
	for N in 14; do
		for K in 1 2 3 4 5 6 7 8 9 10 11 12 13 14; do
 			qsub -v p=$P,n=$N,k=$K ./pbs_script.pbs
		done
	done
done
