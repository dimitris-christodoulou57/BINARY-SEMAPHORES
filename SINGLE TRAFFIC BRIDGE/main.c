//ADAMAKIS CHRISTOS 2148
//CHRISTODOULOU DIMITRIS 2113
//OMADA 2

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <error.h>


#include "binary_semaphores.h"


#define MAX 2

struct assignments{
	mybsem_t *protection;//gia tin prostasia ton metablitwn
	mybsem_t *bridge_red, *bridge_blue;//gia ton elegxo tis gefiras
	mybsem_t *blue_fuel, *red_fuel;//gia to pote einai gemati i gefira
	mybsem_t *simulation_end;
};

// arxikopoiisi metablitwn pou xrisimopoioume sto sigxronismo
volatile int red_run=0, blue_run=0;
volatile int red_wait=0, blue_wait=0;
volatile int enter_blue=0, exit_blue=0;//boi8itikes metablites gia tin emfanisei ton minimaton
volatile int enter_red=0, exit_red=0;//boi8itikes metablites gia tin emfanisei ton minimaton
volatile int blue_max=0, red_max=0;//metablites gia na kseroume pote i gefura einai gemati
volatile int enter=0;

void *blue_function(void *assignment){

	struct assignments *args;
	mybsem_t *protection,*bridge_red, *bridge_blue;
	mybsem_t *simulation_end;

	args = (struct assignments *) assignment;


	protection = args->protection;
	bridge_red = args->bridge_red;
	bridge_blue = args->bridge_blue;
	simulation_end = args->simulation_end;


	mybsem_down(protection);
	enter++;
	// 	an trexoun h perimenoun ta kokkina tote auksnoume ta blue_wait kai mplokaroume to bridge_blue
	if(red_run + red_wait > 0){
		blue_wait++;
		mybsem_up(protection);
		mybsem_down(bridge_blue);
// 		an ta blue_max einai perisotera apo to max tote mplokaroume to bridge_blue
		if(blue_run > MAX){
			mybsem_down(protection);
// 			auksounemoe to blue max gia na kseroume poses fores kollise
			blue_max++;
			mybsem_up(protection);
			mybsem_down(bridge_blue);
			mybsem_down(protection);
//			meiwnoume to blue max gia na kseroume posa perimenoun akoma
			blue_max--;
			mybsem_up(protection);
		}
// 		an ta blue_wait einai megalitera tou midenos tote auksanoume to blue run kai kanoume up to bridge_blue
		else if(blue_wait > 0){
			blue_wait--;
			blue_run++;
			mybsem_up(bridge_blue);
		}
		else{
			mybsem_up(protection);
		}
	}
	else{
//  		an mporoun na treksoun blue tote auksanoumeto blue_run (an einai >MAX tote kanoume down to bridge_blue)
		blue_run++;
 		if(blue_run > MAX){
// 			auksounemoe to blue max gia na kseroume poses fores kollise kai meiwnome to blue_run afou tha kolisei
 			blue_max++;
			blue_run--;
 			mybsem_up(protection);
 			mybsem_down(bridge_blue);
			mybsem_down(protection);
// 			auksounemoe to blue run kathos ksekolise kai meiwnome to blue max afou exei ksekolisei
			blue_run++;
			blue_max--;
 		}
		mybsem_up(protection);
	}
	mybsem_down(protection);
	enter_blue++;
	mybsem_up(protection);
	printf("enter bridge blue %d\n", enter_blue);
	sleep(5);
	mybsem_down(protection);
	exit_blue++;
	mybsem_up(protection);
	printf("exit bridge blue %d\n",exit_blue);

	mybsem_down(protection);
// 	to autokinito perase tin gefira opote miwnoume to blue_run
	blue_run--;

// 	an to blue_max>0 kai den einai gemati i gefura kanoume up gia auta pou mplokaran epeidi h gefira itan gemati
	if((blue_max > 0) && (blue_run!=MAX)){
 			mybsem_up(bridge_blue);
	}
 	else{
// 	an perasoun ta mple kai perimenoun kokkina tote kanoume up to bridge_red
		if((blue_run <= 0) && (red_wait > 0)){
			red_wait--;
			red_run++;
			mybsem_up(bridge_red);
		}
		else{
			mybsem_up(protection);
		}
 	}

	if(enter == exit_blue + exit_red){
		mybsem_up(simulation_end);
	}

	mybsem_up(protection);

	return NULL;
}


void *red_function(void *assignment){

	struct assignments *args;
	mybsem_t *protection,*bridge_red, *bridge_blue;
	mybsem_t *simulation_end;

	args = (struct assignments *) assignment;

	protection = args->protection;
	bridge_red = args->bridge_red;
	bridge_blue = args->bridge_blue;
	simulation_end = args->simulation_end;

	mybsem_down(protection);
	enter++;
	//an trexoun h perimenoun ta mple tote auksnoume ta red_wait kai mplokaroume to bridge_red
	if(blue_run + blue_wait > 0){
		red_wait++;

		mybsem_up(protection);
		mybsem_down(bridge_red);

		//an ta red_max einai perisotera apo to max tote mplokaroume to bridge_red
		if(red_run > MAX){
 			mybsem_down(protection);
// 			auksounemoe to red max gia na kseroume poses fores kollise
			red_max++;
 			mybsem_up(protection);
			mybsem_down(bridge_red);
//			meiwnoume to red max gia na kseroume posa perimenoun akoma
 			red_max--;
		}
// 		an ta red_wait einai megalitera tou midenos tote auksanoume to red_run kai kanoume up to bridge_red
		else if(red_wait > 0){
 			mybsem_down(protection);
			red_wait--;
			red_run++;
 			mybsem_up(protection);
			mybsem_up(bridge_red);
		}
		else{
			mybsem_up(protection);
		}
	}
	else{
		//an mporoun na treksoun red tote auksanoumeto red_run (an einai >MAX tote kanoume down to bridge_red)
		red_run++;
 		if(red_run > MAX){
// 			auksounemoe to red max gia na kseroume poses fores kollise kai meiwnome to red_run afou tha kolisei
 			red_max++;
			red_run--;
 			mybsem_up(protection);
 			mybsem_down(bridge_red);
			mybsem_down(protection);
// 			auksounemoe to red run kathos ksekolise kai meiwnome to red max afou exei ksekolisei
			red_run++;
 			red_max--;
 		}

		mybsem_up(protection);
	}
	mybsem_down(protection);
	enter_red++;
	mybsem_up(protection);
	printf("enter bridge red %d\n", enter_red);
	sleep(5);
	mybsem_up(protection);
	exit_red++;
	printf("exit bridge red %d\n",exit_red);

	mybsem_down(protection);
	// 	to autokinito perase tin gefira opote miwnoume to red_run
	red_run--;

	//an to blue_max>0 kai den einai gemati i gefura kanoume up gia auta pou mplokaran epeidi h gefira itan gemati
	if(red_max > 0 && red_run!=MAX){
 			mybsem_up(bridge_red);
	}
 	else{
 		// an perasoun ta kokkina kai perimenoun mple tote kanoume up to bridge_blue

 		if((red_run <= 0) && (blue_wait > 0)){
			blue_wait--;
			blue_run++;
			mybsem_up(bridge_blue);
		}
		else{
			mybsem_up(protection);
		}
 	}


	if(enter == exit_blue + exit_red){
		mybsem_up(simulation_end);
	}

	mybsem_up(protection);

	return NULL;
}



int main(int argc, char *argv[]) {

	int res;
	int file_read;
	pthread_t thread_red,thread_blue;
	char c;
	struct assignments arguments;
	mybsem_t protection, bridge_red, bridge_blue, red_fuel, blue_fuel;
	mybsem_t simulation_end;

	if(argc!=2){
		printf("error argument\n");
		return -1;
	}


	file_read=open(argv[1], O_RDONLY, 0);
	if(file_read==-1){
		printf("file not found.\n");
		return -1;
	}

// 	arxikopoioume tous simatoforous
	mybsem_init(&protection, 1);
	mybsem_init(&bridge_red, 0);
	mybsem_init(&bridge_blue, 0);
	mybsem_init(&red_fuel, 0);
	mybsem_init(&blue_fuel, 0);
	mybsem_init(&simulation_end, 0);

	while(read(file_read, &c, sizeof(char)) != 0){
		sleep(1);

// 		printf("give the color of next car b/r (e to exit) \n");
// 		scanf("%c", &c);
// 		getchar();
// 		if(c == 'e'){
// 			break;
// 		}
		if(c=='b'){
// 			stelnoume ta dedomena stin sinartisi blue
			arguments.protection = &protection;
			arguments.bridge_red = &bridge_red;
			arguments.bridge_blue = &bridge_blue;
			arguments.blue_fuel = &blue_fuel;
			arguments.simulation_end = &simulation_end;
			res = pthread_create(&thread_blue, NULL, blue_function, &arguments);
			if (res){
				printf("error pthread controler \n");
			}
		}
		if(c=='r'){
// 			stelnoume ta dedomena stin sinartisi red
			arguments.protection = &protection;
			arguments.bridge_red = &bridge_red;
			arguments.bridge_blue = &bridge_blue;
			arguments.red_fuel = &red_fuel;
			arguments.simulation_end = &simulation_end;
			res = pthread_create(&thread_red, NULL, red_function, &arguments);
			if (res){
				printf("error pthread controler \n");
			}
		}

	}

	mybsem_down(&simulation_end);

	return 0;

}
