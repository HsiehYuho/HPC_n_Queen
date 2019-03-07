#!/bin/bash
for P in 1 2 4 8 16; do
	for N in 16; do
		for K in 4; do
 			qsub -v p=$P,n=$N,k=$K ./pbs_script.pbs
		done
	done
done
