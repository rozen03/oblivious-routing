#include "node.h"
#include "picosha2.h"
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <cstdlib>
#include <queue>
#include <atomic>
#include <mpi.h>
#include <map>
#include <unistd.h>
#include <mutex>
#include <random>
int total_nodes, mpi_rank;
// map<string,Block> node_blocks;
atomic<bool> probando;
queue<Block> waiting;
std::mt19937 gen;
int contador;

int  getRandom(int a){
	int res = (gen()+mpi_rank+1)%total_nodes;
	while(res==a){
		res = (gen()+mpi_rank+1)%total_nodes;
	}
	return res;
}

void send_block(const Block *block){
    int dest= getRandom(mpi_rank);
	MPI_Send(block, 1, *MPI_BLOCK, dest, TAG_NEW_BLOCK, MPI_COMM_WORLD);
}

void lock(){
    bool expected = false;
    while (!probando.compare_exchange_weak(expected, true)){
        expected = false;
    }
}

void unlock(){
    probando = false;
}

//Proof of work
void* create_blocks(void *ptr){

    string hash_hex_str;
	int index =0;
    while(true){
		index++;
		Block* block = new Block;
        //Preparar nuevo bloque
        block->index = index;
        block->node_owner_number = mpi_rank;
		block->node_destination_number =getRandom(mpi_rank);
        block->created_at = static_cast<unsigned long int> (time(NULL));
		block->arrived_at = static_cast<unsigned long int> (time(NULL));
		// printf("[%d] Agregué un producido con index %d \n",mpi_rank,last_block_in_chain->index);
		// node_blocks[hash_hex_str] = *last_block_in_chain;
		lock();
		waiting.push(*block);
		unlock();
		sleep(0.05);
		if (index  >= MAX_BLOCKS){
			// MPI_Abort(MPI_COMM_WORLD,0);
			break;
		}
    }
	printf("[%d] listo los noodo \n",mpi_rank);
    return NULL;
}
void* send_blocks(void *ptr){
	Block block;
	while(true){
		lock();
		if(!waiting.empty()){
			block =waiting.front();
			waiting.pop();
			unlock();
			send_block(&block);
			// printf("[%d] mando bauhura \n",mpi_rank);
			// delete block;

		}else{
			unlock();
			// printf("[%d] nada por aqui, nada por allá \n",mpi_rank);
			sleep(0.5);
		}
	}
}

int node(){
	contador=0;
	int taim =time(0);
	seed_seq sseq{taim,mpi_rank,total_nodes};
	gen.seed(sseq);
    probando = false;
    //Tomar valor de mpi_rank y de nodos totales
    MPI_Comm_size(MPI_COMM_WORLD, &total_nodes);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    //La semilla de las funciones aleatorias depende del mpi_ranking
    srand(time(NULL) + mpi_rank);
    printf("[MPI] Lanzando proceso %u\n", mpi_rank);

    //Crear thread para minar
    pthread_t creater;
    pthread_create(&creater, NULL, create_blocks, NULL);

	pthread_t sender;
	pthread_create(&sender, NULL, send_blocks, NULL);
    char block_hash[HASH_SIZE];

	MPI_Status status;
	Block *block = new Block;
    while(true){
		//Recibir mensajes de otros nodos
		MPI_Probe(MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,&status);
        auto tag = status.MPI_TAG;
        if (tag == TAG_NEW_BLOCK) {
            MPI_Recv(block, 1, *MPI_BLOCK,  status.MPI_SOURCE, TAG_NEW_BLOCK, MPI_COMM_WORLD, &status);
			if(block->node_destination_number!=(uint)mpi_rank){
				lock();
				waiting.push(*block);
				unlock();
			}else{
				printf("[%d] El bloque %d/%d llego a destino \n",mpi_rank,block->index,block->node_owner_number);
				// delete block;
			}
			contador++;
            //Si es un mensaje de nuevo bloque, llamar a la función validate_block_for_chain con el bloque recibido y el estado de MPI
			// printf("[%d] recibi %d un bloques \n",mpi_rank,contador);

        } else if(tag == TAG_CHAIN_HASH) {
            //Si es un mensaje de pedido de cadena, responderlo enviando los bloques correspondientes
            MPI_Recv(block_hash, HASH_SIZE, MPI_CHAR, status.MPI_SOURCE, TAG_CHAIN_HASH, MPI_COMM_WORLD, &status);
        }

    }
	// ofstream myfile;
	// myfile.open(to_string(mpi_rank)+".txt");
	// myfile<<mpi_rank<<","<<vericaciones<<","<<fallidos<<endl;
	// myfile.close();
    // delete block;
    return 0;
}
