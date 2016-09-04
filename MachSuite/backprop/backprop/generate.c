#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

//#include "sol.h"
//#include "train.h"
#include "backprop.h"

int main( int argc, const char* argv[] ){
    int i, j, fd;
    
    struct bench_args_t data;
    struct prng_rand_t state;

    prng_srand(1, &state);
    for( i = 0; i < input_dimension; i++){
        for( j = 0; j < nodes_per_layer; j++){
            data.weights1[i*nodes_per_layer + j] = (((TYPE)prng_rand(&state)/((TYPE)(PRNG_RAND_MAX))) * max) - offset;
        }
    }
    for( i = 0; i < nodes_per_layer; i++){
        data.biases1[i] = (((TYPE)prng_rand(&state)/((TYPE)(PRNG_RAND_MAX))) * max) - offset;
        data.biases2[i] = (((TYPE)prng_rand(&state)/((TYPE)(PRNG_RAND_MAX))) * max) - offset;
        for( j = 0; j < nodes_per_layer; j++){
            data.weights2[i*nodes_per_layer + j] = (((TYPE)prng_rand(&state)/((TYPE)(PRNG_RAND_MAX))) * max) - offset;
        }
    }
    for( i = 0; i < nodes_per_layer; i++){
        data.biases3[i] = (((TYPE)prng_rand(&state)/((TYPE)(PRNG_RAND_MAX))) * max) - offset;
        for( j = 0; j < possible_outputs; j++){
            data.weights3[i*possible_outputs + j] = (((TYPE)prng_rand(&state)/((TYPE)(PRNG_RAND_MAX))) * max) - offset;
        }
    }
    for( i = 0; i < training_sets; i++){
        for( j = 0; j < input_dimension; j++)
            data.training_data[i*input_dimension + j] = (TYPE)training_data[i][j];
        for( j = 0; j < possible_outputs; j++)
            data.training_targets[i*possible_outputs + j] = (TYPE)0;
        data.training_targets[i*possible_outputs + (training_targets[i] - 1)] = 1.0;
    }

    fd = open("input.data", O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
    assert( fd>0 && "Couldn't open input data file");

    data_to_input(fd, (void *)(&data));

    return 0;
}
