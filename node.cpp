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
int total_nodes, mpi_rank;
Block *last_block_in_chain;
// map<string,Block> node_blocks;
atomic<bool> probando;
queue<Block*> waiting;

//Envia el bloque minado a todos los nodos
void broadcast_block(const Block *block){
    //No enviar a mí mismo, desde el de la derecha en adelante
    int dest;
    for (int i =1; i <total_nodes; i++) {
        dest = (mpi_rank+i)%total_nodes;
        MPI_Send(block, 1, *MPI_BLOCK, dest, TAG_NEW_BLOCK, MPI_COMM_WORLD);
    }
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
void* proof_of_work(void *ptr){
    string hash_hex_str;
    Block block;
    unsigned int mined_blocks = 0;
    while(true){
		lock();
        block = *last_block_in_chain;
		unlock();
        //Preparar nuevo bloque
        block.index += 1;
        block.node_owner_number = mpi_rank;
        block.created_at = static_cast<unsigned long int> (time(NULL));
		block.arrived_at = static_cast<unsigned long int> (time(NULL));
        //Contar la cantidad de ceros iniciales (con el nuevo nonce)
        if(solves_problem(hash_hex_str)){
            //Verifico que no haya cambiado mientras calculaba
            lock();
            if(last_block_in_chain->index < block.index){
                mined_blocks += 1;
                *last_block_in_chain = block;
                // node_blocks[hash_hex_str] = *last_block_in_chain;
                printf("[%d] Agregué un producido con index %d \n",mpi_rank,last_block_in_chain->index);
                //Mientras comunico, no responder mensajes de nuevos nodos
                broadcast_block(last_block_in_chain);
				sleep(1);
            }
            unlock();
        }

		// if (last_block_in_chain->index  >= MAX_BLOCKS){
			// MPI_Abort(MPI_COMM_WORLD,0);
			// break;
		// }
    }
    return NULL;
}


int node(){
    probando = false;
    //Tomar valor de mpi_rank y de nodos totales
    MPI_Comm_size(MPI_COMM_WORLD, &total_nodes);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    //La semilla de las funciones aleatorias depende del mpi_ranking
    srand(time(NULL) + mpi_rank);
    printf("[MPI] Lanzando proceso %u\n", mpi_rank);

    last_block_in_chain = new Block;

    //Inicializo el primer bloque
    last_block_in_chain->index = 0;
    last_block_in_chain->node_owner_number = mpi_rank;
    last_block_in_chain->created_at = static_cast<unsigned long int> (time(NULL));

    //Crear thread para minar
    pthread_t minero;
    pthread_create(&minero, NULL, proof_of_work, NULL);

    char block_hash[HASH_SIZE];

	MPI_Status status;
    while(true){
		//Recibir mensajes de otros nodos
		MPI_Probe(MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,&status);
        auto tag = status.MPI_TAG;
        if (tag == TAG_NEW_BLOCK) {
			Block *block = new Block;
            MPI_Recv(block, 1, *MPI_BLOCK,  status.MPI_SOURCE, TAG_NEW_BLOCK, MPI_COMM_WORLD, &status);
			waiting.push(block);
            //Si es un mensaje de nuevo bloque, llamar a la función validate_block_for_chain con el bloque recibido y el estado de MPI
			printf("[%d] recibi un bloquede [%d] \n",mpi_rank,block->node_owner_number);
			printf("[%d] Mi largo  es %d \n",mpi_rank,waiting.size());

        } else if(tag == TAG_CHAIN_HASH) {
            //Si es un mensaje de pedido de cadena, responderlo enviando los bloques correspondientes
            MPI_Recv(block_hash, HASH_SIZE, MPI_CHAR, status.MPI_SOURCE, TAG_CHAIN_HASH, MPI_COMM_WORLD, &status);
        }

    }
	// ofstream myfile;
	// myfile.open(to_string(mpi_rank)+".txt");
	// myfile<<mpi_rank<<","<<vericaciones<<","<<fallidos<<endl;
	// myfile.close();
    delete last_block_in_chain;
    // delete block;
    return 0;
}
