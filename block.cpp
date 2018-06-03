#include "block.h"
#include "node.h"
#include "picosha2.h"
#include <cstring>
#include <cstddef>
#include <string>


using namespace std;

//Verifica que el bloque llegado sea válido (tiempo creado y hash)
//No verifica si corresponde agregarlo a la cadena (tampoco el caso de que no tenga antepasados válidos)
bool valid_new_block(const Block *block){
  unsigned long int current_time = static_cast<unsigned long int> (time(NULL));

  //Que no haya pasado más de VALIDATION_MINUTES desde que se creo
  bool valid = block->created_at + 60 * VALIDATION_MINUTES >= current_time;

  //Que el hash guardado sea válido
  // string hash_hex_str;
  // block_to_hash(block,hash_hex_str);
  // valid = valid && (hash_hex_str.compare(block->block_hash) == 0);
  return valid;
}

//Caracter hexagesimal a binario
const char* hex_char_to_bin(char c)
{
    switch(toupper(c))
    {
        case '0': return "0000";
        case '1': return "0001";
        case '2': return "0010";
        case '3': return "0011";
        case '4': return "0100";
        case '5': return "0101";
        case '6': return "0110";
        case '7': return "0111";
        case '8': return "1000";
        case '9': return "1001";
        case 'A': return "1010";
        case 'B': return "1011";
        case 'C': return "1100";
        case 'D': return "1101";
        case 'E': return "1110";
        default: return "1111";
    }
}

void block_to_hash(const Block *block, string& result){
  string tmp = block_to_str(block);
  picosha2::hash256_hex_string(tmp, result);
}
//Convertir el contenido del bloque a un string
string block_to_str(const Block *block){
  string str = "";
  str +=  block->index;
  str +=  block->node_owner_number;
  str +=  block->created_at;
  str +=  block->arrived_at;
  return str;
}

//Contar el número de ceros en la representación binaria del hash
bool solves_problem(const string& hash){
	return true;
  //cout << "El hash en hexagesimal: " << hash << endl;
  //cout << "El hash en binario:" << hex_str_to_bin_str(hash) << endl;
  // string start = string(DEFAULT_DIFFICULTY,'0');
  // return hex_str_to_bin_str(hash).compare(0, DEFAULT_DIFFICULTY, start) == 0;
}

//Aca definimos el tipo de datos MPI_BLOCK para MPI
void define_block_data_type_for_MPI(MPI_Datatype *new_type){
  int status;

  //Definir las propiedades
  MPI_Aint displacements[2] = {offsetof(Block, index), offsetof(Block, created_at)};
  int block_lengths[2]  = {2,2};
  MPI_Datatype types[2] = {MPI_INT,MPI_UNSIGNED_LONG};

  //Crear la estructura para el nuevo tipo
  status = MPI_Type_create_struct(2, block_lengths, displacements, types, new_type);
  if (status != MPI_SUCCESS){
      fprintf(stderr, "Error al crear el tipo\n");
      MPI_Abort(MPI_COMM_WORLD, status);
  }

  //Confirmar el nuevo tipo
  status = MPI_Type_commit(new_type);
  if (status != MPI_SUCCESS){
    fprintf(stderr, "Error al confirmar el tipo\n");
    MPI_Abort(MPI_COMM_WORLD, status);
  }
}
