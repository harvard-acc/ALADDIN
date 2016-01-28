open_project aes_syn

add_files aes.c
add_files input.data
add_files check.data
add_files -tb ../../common/harness.c

#add_files -tb aes_test.c

set_top aes256_encrypt_ecb

open_solution -reset solution
# set_part virtex7
set_part {xc7z020clg484-1}
create_clock -period 10
source ./aes_dir
#config_rtl -reset all -reset_level low
csynth_design
cosim_design -rtl verilog -tool modelsim -trace_level all -argv "input.data check.data"
export_design -format ip_catalog

exit
