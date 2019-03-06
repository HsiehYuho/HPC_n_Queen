#!/bin/bash
for P in 1; do
	for N in 8 12; do
		for M in 1 2 4; do
 	        	let K=$((M*(N/8)))
 			qsub -v p=$P,n=$N,k=$K ./pbs_script.pbs
		done
	done
done
