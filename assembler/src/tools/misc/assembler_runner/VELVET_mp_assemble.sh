#!/bin/bash

OUTPUT_DIR=RUNNER_PARAM_OUTPUT_DIR

RUNNER_PARAM_VELVET_BASE/velveth $OUTPUT_DIR RUNNER_PARAM_K -shortPaired -separate -fastq RUNNER_PARAM_LEFT RUNNER_PARAM_RIGHT -shortPaired2 -separate -fastq RUNNER_PARAM_MP_LEFT RUNNER_PARAM_MP_RIGHT
RUNNER_PARAM_VELVET_BASE/velvetg $OUTPUT_DIR -exp_cov auto -cov_cutoff auto -ins_length RUNNER_PARAM_INSERT_SIZE -ins_length_sd RUNNER_PARAM_DEVIATION -ins_length2 RUNNER_PARAM_MP_INSERT_SIZE -ins_length2_sd RUNNER_PARAM_MP_DEVIATION -scaffolding yes 
mv $OUTPUT_DIR/contigs.fa $OUTPUT_DIR/scaffolds.fa
RUNNER_PARAM_VELVET_BASE/velvetg $OUTPUT_DIR -exp_cov auto -cov_cutoff auto -ins_length RUNNER_PARAM_INSERT_SIZE -ins_length_sd RUNNER_PARAM_DEVIATION -ins_length2 RUNNER_PARAM_MP_INSERT_SIZE -ins_length2_sd RUNNER_PARAM_MP_DEVIATION -scaffolding no
