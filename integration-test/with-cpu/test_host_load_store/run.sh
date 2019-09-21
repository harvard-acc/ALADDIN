#!/usr/bin/env bash

bmk_home=${ALADDIN_HOME}/integration-test/with-cpu/test_host_load_store
gem5_dir=${ALADDIN_HOME}/../..

${gem5_dir}/build/X86/gem5.opt \
  --debug-flags=HybridDatapath,Aladdin \
  --outdir=${bmk_home}/outputs \
  ${gem5_dir}/configs/aladdin/aladdin_se.py \
  --num-cpus=1 \
  --enable_prefetchers \
  --mem-size=4GB \
  --mem-type=DDR3_1600_8x8  \
  --sys-clock=1GHz \
  --cpu-type=DerivO3CPU \
  --caches \
  --ruby \
  --cacheline_size=64 \
  --accel_cfg_file=${bmk_home}/gem5.cfg \
  -c ${bmk_home}/test_host_load_store \
  | gzip -c > stdout.gz

