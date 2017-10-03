#!/usr/bin/env bash

cfg_home=${ALADDIN_HOME}/integration-test/standalone/test_aes
bmk_home=${ALADDIN_HOME}/MachSuite/aes/aes
gem5_dir=${ALADDIN_HOME}/../..

${gem5_dir}/build/X86/gem5.opt \
  --outdir=${cfg_home}/outputs \
  ${gem5_dir}/configs/aladdin/aladdin_se.py \
  --num-cpus=0 \
  --mem-size=4GB \
  --mem-type=DDR3_1600_8x8  \
  --sys-clock=1GHz \
  --cpu-type=DerivO3CPU \
  --caches \
  --cacheline_size=32 \
  --accel_cfg_file=${cfg_home}/gem5.cfg \
  | gzip -c > stdout.gz
