#!/usr/bin/env bash

cfg_home=${ALADDIN_HOME}/integration-test/test_machsuite
bmk_home=${ALADDIN_HOME}/MachSuite/aes/aes
gem5_dir=${ALADDIN_HOME}/../..

${gem5_dir}/build/X86/gem5.opt \
  --debug-flags=CacheDatapath,Aladdin \
  --outdir=${cfg_home}/outputs \
  ${gem5_dir}/configs/aladdin/aladdin_se.py \
  --num-cpus=1 \
  --mem-size=2GB \
  --mem-type=ddr3_1600_x64  \
  --sys-clock=1GHz \
  --cpu-type=timing \
  --caches \
  --aladdin_cfg_file=${cfg_home}/gem5.cfg \
  -c ${bmk_home}/aes \
  -o "${bmk_home}/input.data ${bmk_home}/check.data" \
  > stdout.gz
