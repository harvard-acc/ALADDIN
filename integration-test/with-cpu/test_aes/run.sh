#!/usr/bin/env bash

cfg_home=${ALADDIN_HOME}/integration-test/with-cpu/test_aes
bmk_home=${ALADDIN_HOME}/MachSuite/aes/aes
gem5_dir=${ALADDIN_HOME}/../..

${gem5_dir}/build/X86/gem5.opt \
  --debug-flags=HybridDatapath,Aladdin \
  --outdir=${cfg_home}/outputs \
  ${gem5_dir}/configs/aladdin/aladdin_se.py \
  --num-cpus=1 \
  --mem-size=4GB \
  --mem-type=DDR3_1600_8x8  \
  --sys-clock=1GHz \
  --cpu-type=DerivO3CPU \
  --caches \
  --cacheline_size=64 \
  --enable_prefetchers \
  --accel_cfg_file=${cfg_home}/gem5.cfg \
  -c aes \
  -o "input.data check.data" \
  | gzip -c > stdout.gz
