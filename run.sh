path_to_traces="/Users/liusitan/Documents/ucsd2022spring/cse-240a-branch-predictor/traces"
path_to_predictor="/Users/liusitan/Documents/ucsd2022spring/cse-240a-branch-predictor/src"
bunzip2 -kc $path_to_traces/fp_1.bz2 |  $path_to_predictor/predictor $1
bunzip2 -kc $path_to_traces/fp_2.bz2 |  $path_to_predictor/predictor $1
bunzip2 -kc $path_to_traces/int_1.bz2 | $path_to_predictor/predictor $1
bunzip2 -kc $path_to_traces/int_2.bz2 | $path_to_predictor/predictor $1
bunzip2 -kc $path_to_traces/mm_1.bz2 |  $path_to_predictor/predictor $1
bunzip2 -kc $path_to_traces/mm_2.bz2 |  $path_to_predictor/predictor $1