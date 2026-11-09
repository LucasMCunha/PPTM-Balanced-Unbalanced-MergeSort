#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <mpi.h>


int main (int argc, char *argv[]) {
    MPI_Init();
    int my_rank;
    int size = atoi(argv[1]);  // tamanho do vetor
    int *vetor = malloc(sizeof(int) * size);  // aloca vetor
    int *temp = malloc (sizeof (int) * size);
    int comm_size;
    MPI_Comm_size (MPI_COMM_WORLD, &comm_size);

    int delta = size / comm_size;
    my_rank = MPI_Comm_rank();  // pega pega o numero do processo atual (rank)
    
    // recebo vetor
    
    if ( my_rank != 0 )
       {
       //MPI_Recv ( vetor, pai);                       // não sou a raiz, tenho pai
       MPI_Recv ( vetor, pai, MPI_INT, pai, 0, MPI_COMM_WORLD, &Status );  // não sou a raiz, tenho pai
       MPI_Get_count(&Status, MPI_INT, &tam_vetor);  // descubro tamanho da mensagem recebida
       }
    else
       {
       tam_vetor = VETOR_SIZE;               // defino tamanho inicial do vetor
       Inicializa ( vetor, tam_vetor );      // sou a raiz e portanto gero o vetor - ordem reversa
       }
   
    // dividir ou conquistar?
   
    if ( tam_vetor <= delta )
       BubbleSort (vetor);  // conquisto
    else
           {
        // dividir
        // quebrar em duas partes e mandar para os filhos
        
        MPI_Send ( &vetor[0], filho esquerda, tam_vetor/2 );  // mando metade inicial do vetor
        MPI_Send ( &vetor[tam_vetor/2], filho direita , tam_vetor/2 );  // mando metade final
        
        // receber dos filhos
        
        MPI_Recv ( &vetor[0], filho esquerda);            
        MPI_Recv ( &vetor[tam_vetor/2], filho direita);   
        
        // intercalo vetor inteiro
        
        Intercala ( vetor );
        }
    
    // mando para o pai
    
    if ( my_rank !=0 ){
       MPI_Send ( vetor, pai, tam_vetor );  // tenho pai, retorno vetor ordenado pra ele
    }else{
       Mostra ( vetor );                    // sou o raiz, mostro vetor
    }
    MPI_Finalize();
    return 0;
}

void run_helper_mpi (int my_rank, int max_rank, int tag, MPI_Comm comm, int workableLoad) {
  int level = my_topmost_level_mpi (my_rank);
  // probe for a message and determine its size and sender
  MPI_Status status;
  int size;
  MPI_Probe (MPI_ANY_SOURCE, tag, comm, &status);
  MPI_Get_count (&status, MPI_INT, &size);
  int parent_rank = status.MPI_SOURCE;
  // allocate int a[size], temp[size] 
  int *a = malloc (sizeof (int) * size);
  int *temp = malloc (sizeof (int) * size);
  MPI_Recv (a, size, MPI_INT, parent_rank, tag, comm, &status);
  BubbleSort(a, size); //Mudar para bubble sort
  // Send sorted array to parent process
  MPI_Send (a, size, MPI_INT, parent_rank, tag, comm);
  return;
}

void BubbleSort(int a[], int size) {
  int i, j, temp;
  for (i = 0; i < size - 1; i++) {
    for (j = 0; j < size - i - 1; j++) {
      if (a[j] > a[j + 1]) {
        // Swap a[j] and a[j+1]
        temp = a[j];
        a[j] = a[j + 1];
        a[j + 1] = temp;
      }
    }
  }
}
