//Parâmetros da simulação

#define Nxyz 8 //Lado da rede na direção espacial
#define Nt 8 //Lado da rede na direção temporal
#define d 4	// Dimensão espaço temporal da rede

#define Volume Nxyz*Nxyz*Nxyz*Nt //	Número de pontos da rede

// Paramêtros gerais da Simulação

#define max_configs 4 //	Número máximo de configurações descorrelacionadas a serem geradas

#define max_length_name 2000

//	Codes
#define REAR -1	//	Usados para se referir a posições vizinhas na rede 
#define FRONT 1