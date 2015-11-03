#!/usr/bin/env bash

bmk_home=${ALADDIN_HOME}/integration-test/with-cpu/test_mmap
gem5_dir=${ALADDIN_HOME}/../..

${gem5_dir}/build/X86/gem5.opt \
  --debug-flags=HybridDatapath,Aladdin \
  --outdir=${bmk_home}/outputs \
  ${gem5_dir}/configs/aladdin/aladdin_se.py \
  --num-cpus=1 \
  --enable_prefetchers \
  --mem-size=4GB \
  --mem-type=ddr3_1600_x64  \
  --sys-clock=1GHz \
  --cpu-type=detailed \
  --caches \
  -c ${bmk_home}/test_mmap \
  > outputs/stdout.gz
