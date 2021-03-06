#ifndef LATTICE_H
#define LATTICE_H

typedef struct {
	int t;
	int i, j, k;
} pos_vec;	//	struct for position vectors


pos_vec add_position_vector(pos_vec u, pos_vec v);

pos_vec add_pos_vec(pos_vec u, pos_vec v);

void print_pos_vec(pos_vec u);

pos_vec hop_position_positive(pos_vec u, int mu);

pos_vec hop_position_negative(pos_vec u, int mu);

pos_vec translate_index_to_position(int index);

int position_is_even(pos_vec position);

int position_is_odd(pos_vec position);

double complex * get_link(double complex * U, pos_vec position, int mu);

void get_link_matrix(double complex * U, pos_vec position, int mu, int direction, double complex * u);


void SU3_load_config(char filename[max_length_name], double complex *U);

void SU3_copy_config(double complex *U, double complex *U_copy);


#endif