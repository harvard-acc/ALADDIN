#!/usr/bin/env bash

bmk_home=${ALADDIN_HOME}/integration-test/test_load_store
gem5_dir=${ALADDIN_HOME}/../..

${gem5_dir}/build/X86/gem5.opt \
  --debug-flags=CacheDatapath,Aladdin \
  --outdir=${bmk_home}/outputs \
  ${gem5_dir}/configs/aladdin/aladdin_se.py \
  --num-cpus=1 \
  --enable-prefetchers \
  --mem-size=2GB \
  --mem-type=ddr3_1600_x64  \
  --sys-clock=1GHz \
  --cpu-type=timing \
  --caches \
  --aladdin_cfg_file=${bmk_home}/gem5.cfg \
  -c ${bmk_home}/test_load_store \
  > stdout.gz
