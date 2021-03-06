#include <stdio.h>							//	Standard header C files
#include <stdlib.h>
#include <complex.h>
#include <math.h>

#include <omp.h>


#include "../SU3_parameters.h"				//	Simulation parameters
#include "../SU3_gaugefixing_parameters.h"	//	Gauge-fixing specific parameters

#include "lattice.h"						//	Initialization functions and 
											//	calculations of positions and
											//	links on the lattice.


#include "math_ops.h"						//	Math operations
#include "SU2_ops.h"						//	SU(2) operations
#include "SU3_ops.h"						//	SU(3) operations
#include "fourvector_field.h"				//	Calculation of A_mu(n) and related things

void SU3_local_update_U(double complex * U, pos_vec position, double complex * g){

	//	Updates U only at a given position
	
	double complex * g_dagger = (double complex *) malloc(3 * 3 * sizeof(double complex)) ;

	if ( g_dagger == NULL) {
		//	Test if allocation was successful.
        printf("Memory allocation failed in function SU3_local_update_U");
        exit(1); 

    }

	for (int mu = 0; mu < d ; mu++) {
 
		//	U'_mu(x)=g(x).U_mu(x).1 for red-black updates

		SU3_accumulate_left_product(g, get_link(U,position,mu));

		//	U'_mu(x-mu)=1.U_mu(x-mu).g_dagger(x) for red-black updates

		SU3_hermitean_conjugate(g, g_dagger);

		SU3_accumulate_right_product(get_link(U, hop_position_negative(position, mu), mu), g_dagger);
		
	}

	free(g_dagger);
}

void SU3_calculate_w(double complex * U, pos_vec position, double complex * w){	
	
	//	Calculates 	w(n) = sum_mu U_mu(n).1+U_dagger_mu(n-mu_hat).1 for red black subdivision, following the notation in hep-lat/9306018
	//	returns result in w.


	double complex * u_dagger_rear = (double complex *) malloc(3 * 3 * sizeof(double complex));	

	if ( u_dagger_rear == NULL) {
		//	Test if allocation was successful.
        printf("Memory allocation failed in function SU3_calculate_w");
        exit(1); 

    }

	SU3_set_to_null(w);		//	Initializing w(n)=0

	// h(n)	calculation

	for (int mu = 0; mu < d; mu++){
		//	h(n) = sum_mu U_mu(n).1+U_dagger_mu(n-mu_hat).1 for red black subdivision

		SU3_accumulate(get_link(U, position, mu), w);

		SU3_hermitean_conjugate(get_link(U, hop_position_negative(position, mu), mu), u_dagger_rear);

		SU3_accumulate(u_dagger_rear, w);

	}
	
	free(u_dagger_rear);
}


double SU3_calculate_e2_local(double complex *U, pos_vec position){
	double e2_local=0.0;
	
	double complex * div_A = (double complex *) malloc(3 * 3 * sizeof(double complex));	//	Quadridivergence of A
	double * div_A_components = (double *) malloc(9 * sizeof(double));	//	Components of the quadridergence projected onto the Gell-Mann matrices
	

	if ( div_A == NULL || div_A_components == NULL ) {

		//	Test if allocation was successful.
        printf("Memory allocation failed in function SU3_calculate_e2_local");
        exit(1); 

    }
	//	Calculates quadridivergence and projects to the SU(3) algebra

	SU3_divergence_A(U, position, div_A);
	SU3_decompose_algebra(div_A, div_A_components);

	for(int a = 1; a <= 8; a++){	
		//	Normalized sum of the squares of the color components of the divergence of A.
		
		e2_local += (double)pow2(div_A_components[a])/(Volume);
	}
	free(div_A); free(div_A_components);

	return e2_local;
}

double SU3_calculate_e2(double complex * U){

//	Calculates e2 (defined in hep-lat/0301019v2), 
//	used to find out distance to the gauge-fixed situation.
	
	pos_vec position;

	double e2 = 0.0;

	for (position.t = 0; position.t < Nt; position.t++) {
		for (position.i = 0; position.i < Nxyz; position.i++) {
			for (position.j = 0; position.j < Nxyz; position.j++) {
				for (position.k = 0; position.k < Nxyz; position.k++) {

					e2 += SU3_calculate_e2_local(U, position);

				}
			}
		}
	}

	return e2;					
}

void SU3_update_sub_LosAlamos(double complex * matrix_SU3, char submatrix, complex double * update_SU3){

	int a, b;
	double * matrix_SU2 = (double *) malloc( 4 * sizeof(double));


	if ( matrix_SU2 == NULL ) {
		//	Test if allocation was successful.
        printf("Memory allocation failed in function project_SU3_to_sub_SU2");
        exit(1); 

    }

	SU3_set_to_null(update_SU3);

	switch(submatrix){
		case 'R':
			a = 0;
			b = 1;
			update_SU3[ 2 * 3 + 2] = 1.0;
			break;

		case 'S':
			a = 0;
			b = 2;
			update_SU3[ 1 * 3 + 1] = 1.0;
			break;

		case 'T':
			a = 1;
			b = 2;
			update_SU3[ 0 * 3 + 0] = 1.0;
			break;

		default:
			printf("Invalid submatrix");
			exit(1);
	}

	matrix_SU2[0] =  (creal(matrix_SU3[a * 3 + a]) + creal(matrix_SU3[b * 3 + b]));
	matrix_SU2[1] = -(cimag(matrix_SU3[a * 3 + b]) + cimag(matrix_SU3[b * 3 + a]));
	matrix_SU2[2] = -(creal(matrix_SU3[a * 3 + b]) - creal(matrix_SU3[b * 3 + a]));
	matrix_SU2[3] = -(cimag(matrix_SU3[a * 3 + a]) - cimag(matrix_SU3[b * 3 + b]));

	SU2_projection(matrix_SU2);

	update_SU3[a * 3 + a] =  matrix_SU2[0] + I * matrix_SU2[3];
	update_SU3[a * 3 + b] =  matrix_SU2[2] + I * matrix_SU2[1];
	update_SU3[b * 3 + a] = -matrix_SU2[2] + I * matrix_SU2[1];
	update_SU3[b * 3 + b] =  matrix_SU2[0] - I * matrix_SU2[3];

	free(matrix_SU2);
}

void SU3_LosAlamos_common_block(double complex * w, double complex * total_update){

	//	Calculates the update matrix A from w(n)=g(n).h(n) as in the Los Alamos
	//	algorithm for SU(3), with a division of the update matrix in submatrices
	//	following the Cabbibo-Marinari trick. Actual A(n) is obtained after a number
	//	of "hits" to be performed one after another.
	char submatrices[3] = {'R', 'S', 'T'};

	double complex * update = (double complex *) malloc(3 * 3 * sizeof(double complex));	//	First the conjugate to w, after that to Rw, then to SRw, ...
	double complex * updated_w   = (double complex *) malloc(3 * 3 * sizeof(double complex));	//	Accumulated update for w(x)

	if ( update == NULL || updated_w == NULL ) {
		//	Test if allocation was successful.
        printf("Memory allocation failed in function SU3_LosAlamos_common_block");
        exit(1); 

    }

	SU3_copy(w, updated_w);
	SU3_set_to_identity(total_update);

	for (int hits = 1; hits <= maxhits; hits++) {	
		//	Each hit contains the Cabbibo-Marinari subdivision
		for (int i = 0; i <= 2 ; i++){
			//	Submatrices are indicated by letter in subs

			SU3_update_sub_LosAlamos(updated_w, submatrices[i], update);
			SU3_accumulate_left_product(update, updated_w);

			SU3_accumulate_left_product(update, total_update);
			//	Updates matrix to total_update. It is the
			//	accumulated updates from the hits.
			
		}
	}
	 free(update);	free(updated_w);
}

void SU3_gaugefixing_overrelaxation(double complex * U, pos_vec position){	

//	Generalization of the algorithm described in hep-lat/0301019v2, using the 
//	Cabbibo-Marinari submatrices trick.
//	It updates the g at the given position.

	double complex * update_LA = (double complex *) malloc(3 * 3 * sizeof(double complex));	//	Final update to g coming from Los Alamos block.
						//	g_updated=A*g_old

	double complex * w  = (double complex *) malloc(3 * 3 * sizeof(double complex));	//	w(n)=g(n)*h(n), following the notation of hep-lat/0301019v2

	if ( update_LA == NULL || w == NULL ) {
		//	Test if allocation was successful.
        printf("Memory allocation failed in function SU3_gaugefixing_overrelaxation");
        exit(1); 

    }

	SU3_calculate_w(U, position, w);	//	Calculating w(n)=h(n) for red black subdivision 
	SU3_LosAlamos_common_block(w, update_LA);

	free(w);

	//	The above function determines update_LA which would be the na??ve
	//	update to bring the local function to its mininum. However
	//	we found out that actually using overrelaxation, which 
	//	means using update_LA^omega instead of  update_LA, where 1<omega<2 makes the
	//	converge faster. update_LA^omega is calculated using the first two terms
	//	of the binomial expansion: update_LA^omega=I+omega(update_LA-I)+...=I(1-omega)+omega*update_LA+...


	double complex * update_OR = (double complex *) malloc(3 * 3 * sizeof(double complex));	//	The over-relaxation update.

	if ( update_OR == NULL ) {
		//	Test if allocation was successful.
        printf("Memory allocation failed in function SU3_gaugefixing_overrelaxation");
        exit(1); 

    }


	// update_OR = update_LA^omega = Proj_SU3((I(1-omega)+omega*update_LA)
	SU3_set_to_identity(update_OR);
	SU3_substitution_multiplication_by_scalar(1.0 - omega_OR, update_OR);
	SU3_substitution_multiplication_by_scalar(omega_OR, update_LA);
	SU3_accumulate(update_LA, update_OR);	
	free(update_LA); 

	SU3_projection(update_OR);

	SU3_local_update_U(U, position, update_OR);

	free(update_OR);
}

int SU3_gauge_fix(double complex * U, int config){
	
	//	Fix the gauge and follows the process by calculating e2;
	
	pos_vec position;
	double e2 = SU3_calculate_e2(U);
	//	Gauge-fixing index, which will be less than the tolerance,
	//	when the gauge-fixing is considered to be attained.
	//	Following the notation of hep-lat/0301019v2	
	
	int	sweep = 0;
	//	Counter to the number of sweeps to fix config to Landau gauge

	do {
		sweep++;

		#pragma omp parallel for num_threads(2) private(position)
		// Paralelizing by slicing the time extent
			for (int t = 0; t < Nt; t++) {
				position.t = t;

				for ( position.i = 0; position.i < Nxyz; position.i++) {
					for ( position.j = 0; position.j < Nxyz; position.j++) {
						for ( position.k = 0; position.k < Nxyz; position.k++) {
							
							if( (position_is_even(position)+sweep) % 2 ) {
								//	Implementation of the red black subdivision of the lattice

								SU3_gaugefixing_overrelaxation(U, position);	
									//	The actual gauge-fixing algorithm
								
							}
							
						}
					}
				}
			}

		if (sweep % sweeps_to_measurement_e2 == 0) {
			//	No need to calculate e2 all the time
			//	because it will take some hundreds of sweeps
			//	to fix the gauge.
			

			e2 = SU3_calculate_e2(U);	
			//	Indicates how far we are to the Landau-gauge 
			//	condition, e2=0.
			
			printf("\nconfig: %d, sweep: %d, e2: %3.2e\n", config, sweep, e2);

		}
		
	} while (e2 > tolerance);	//	As long as e2 is larger than the tolerance
								//	repeats the process iteratively.
	
							
	printf("Sweeps needed to gauge-fix: %d \n", sweep);

	return sweep;
}

//void SU3_print_gaugefixed_U(double complex * U, double complex *U_aux, char filename[max_length_name]){	

// //	Prints the current configuration to a file, after the process
// //	of gauge-fixing. The configuration printed is therefore the one
// //	stored in U_aux.

//	FILE *gauge_configuration_file;

//	gauge_configuration_file = fopen(filename, "w+");

//	fwrite(U_aux, sizeof(U_aux), 1, gauge_configuration_file);

//	fclose(gauge_configuration_file);
		
//}