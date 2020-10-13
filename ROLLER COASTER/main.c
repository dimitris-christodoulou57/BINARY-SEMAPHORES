//ADAMAKIS CHRISTOS 2148
//CHRISTODOULOU DIMITRIS 2113
//OMADA 2

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include "binary_semaphores.h"

#define N 5

// ka8olikes metablites gia na kseroume posoi epibates mpikan kai posoi bgikan
// to number_in gia na kseroume to plh8os twn pelatwn pou einai mesa
// to number_passengers_enter gia na kseroume mexri poios epibatis exei mpei
// to number_passengers_exit gia na kseroume poios exei apobibastei
volatile int number_passengers, number_passengers_enter, number_passengers_exit, number_in;
volatile int train_situation, wait_passengers ,num_pass;

//simatoforoi pou 8a xrisimopoiisoume
// struct bsem{
// 	struct mybsem_struct *train;
// 	struct mybsem_struct *customer, *finish;
// 	struct mybsem_struct *protection;
// };


struct bsem{
	mybsem_t *train;
	mybsem_t *customer, *finish;
	mybsem_t *protection;
};

// sinartisi train elegxei pote 8a ksekinisei to treno
void *thread_train(void *assignment){

	struct bsem *args;
	int i;

	args = (struct bsem *) assignment;

// 	arxikopoioume tous epibates me 0
	train_situation=0;
	number_passengers=0;
	number_passengers_enter=0;
	number_passengers_exit=0;
	number_in=0;
	wait_passengers=0;

// 	while 1 giati den kseroume poses fores 8a paei to treno sto telos
	while(1){
// 		kanoume down gia na perimenoume na gemisei to treno gia na paei sto telos
		mybsem_down(args->train);
		printf("to treno ksekinise apo tin afetiria\n");
		sleep(5);
		printf("to treno eftase sto telos\n");
// 		afou ftasei sto telos kanoume up stous epibates gia na apobibastoun
		for(i=number_passengers_exit; i<number_passengers_enter; i++){
			mybsem_up(&args->customer[i]);
		}
// 		down sto train mexri na apobibastoun oloi gia na epistrepsei stin afetiria
		mybsem_down(args->train);
		printf("to treno ksekinise apo to telos\n");
		sleep(5);
		printf("to treno eftase stin afetiria\n");
// 		an den perimenoun alloi epibates kanoume to train_situation 0
		if(wait_passengers==0){
			train_situation=0;
		}
		else {
// 			kanoume up tous epomenous epibates pou xwrane sto treno kai oi epomeni pou 8a er8oun kai 8a kanoun down den kollane
			for(i=number_passengers_exit;i<number_passengers_exit+N; i++){
				mybsem_up(&args->customer[i]);
			}
		}
// 		an o exoun perasei oloi oi epibates tote kanoume break apo to while
		if(number_passengers==num_pass && number_passengers_exit==num_pass){
			break;
		}
	}
// 	kanoume up to finish gia na teleiwsei to programma
	mybsem_up(args->finish);

	return(NULL);
}

void *thread_customer(void *assignment){

	int my_number;

	struct bsem *args;

	args = (struct bsem *) assignment;
	mybsem_down(args->protection);
// 	auksanoume gia na kseroume poios epibateis einai
	number_passengers++;
// 	topiki metabliti gia to down tou pelati
	my_number=number_passengers-1;
// 	an to train_situation !=0 tote prepei na kollisoun
	if(train_situation!=0){
// 		auksanoume to wait number_passengers kai kanoume down sto antistoixo pelati
		wait_passengers++;
		mybsem_up(args->protection);
		mybsem_down(&args->customer[my_number]);
		mybsem_down(args->protection);
// 		afou ksekollisei meiwnoume to wait_passengers
		wait_passengers--;
	}
	printf("o epibatis %d epibibastike\n", number_passengers_enter);
// 	otan treno gemisei kanoume up to treno gia na ksekinisei
	if(number_in == N-1){
// 		kanoume to train_situation =1 gia na kollisoun oi upoloipoi
		train_situation=1;
		printf("to treno arxizei gemise\n");
		mybsem_up(args->protection);
		mybsem_up(args->train);
	}
	number_in++;
	number_passengers_enter++;
	mybsem_up(args->protection);
// 	perimenoume mexri na ftasei to treno sto telos gia na mporei na apobibastei
	mybsem_down(&args->customer[my_number]);
	mybsem_down(args->protection);
	printf("o epibatis %d apobibastike\n", number_passengers_exit);
	number_passengers_exit++;
	number_in--;
// 	otan apobibastoun oloi kanoume up to train gia na epistrepsei stin afetiria
	if(number_in==0){
		printf("to treno adeiase paei stin afetiria\n");
		mybsem_up(args->train);

	}
	mybsem_up(args->protection);

	return(NULL);
}


int main (int argc, char * argv[]){

	struct bsem assignment;
	pthread_t train,customer;
	int i;
// 	struct mybsem_struct train_p, protection, finish;
// 	struct mybsem_struct *customers;
	mybsem_t train_p, protection, finish;
	mybsem_t *customers;

	if(argc!=2){
		printf("error argument\n");
	}

	num_pass=atoi(argv[1]);

// 	arxikopoioume tous simatoforus
// 	customers = (struct mybsem_struct*)malloc(sizeof(struct mybsem_struct)*num_pass);
	customers = (mybsem_t*)malloc(sizeof(mybsem_t)*num_pass);
	if(customers == NULL){
		printf("error malloc \n");
	}
	for(i=0; i<num_pass; i++){
		mybsem_init(&customers[i],0);
	}
	mybsem_init(&train_p,0);
	mybsem_init(&protection,1);
	mybsem_init(&finish,0);

// 	pername san orisma tous simatoforous sto pthread tou trenou
	assignment.train = &train_p;
	assignment.protection = &protection;
	assignment.finish= &finish;
	assignment.customer = customers;

	pthread_create(&train , NULL , thread_train, &assignment);


	for(i=0; i<num_pass; i++){
		sleep(1);
		assignment.customer = customers;
		pthread_create(&customer , NULL , thread_customer, &assignment);
	}

// 	perimenoume mexri na teleiwsoun oloi kai to treno na epistrepsei stin afetiria
	mybsem_down(&finish);

	return 0;

}
