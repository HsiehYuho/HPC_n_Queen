#!/bin/bash
for P in 16; do
	for N in 8 10 12 14 16; do
		for K in 8; do
 			qsub -v p=$P,n=$N,k=$K ./pbs_script.pbs
		done
	done
done
