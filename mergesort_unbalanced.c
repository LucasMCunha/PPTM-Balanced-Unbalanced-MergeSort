
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mpi.h>

#define TAG_DATA 123
#define TAG_STOP 124
#define SMALL 32

void merge(int a[], int n1, int n2, int temp[]);
void BubbleSort(int a[], int size);
void mergesort_serial(int a[], int size, int temp[]);

void mergesort_unbalanced(int a[], int size, int temp[], int rank, int comm_size, MPI_Comm comm);

void run_helper_mpi(int rank, int comm_size, MPI_Comm comm);

void stop_descendants(int rank, int comm_size, MPI_Comm comm);


// --------------------------------------------------
// MAIN
// --------------------------------------------------

int main(int argc, char *argv[]) {

    MPI_Init(&argc, &argv);

    int my_rank, comm_size;

    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);

    if (argc != 2) {

        if (my_rank == 0)
            printf("Usage: %s array-size\n", argv[0]);

        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    int size = atoi(argv[1]);

    if (size <= 0) {
        if (my_rank == 0)
            printf("Invalid array size.\n");

        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (my_rank == 0) {

        printf("-MPI Unbalanced Merge Sort-\n");
        printf("Array size = %d\n", size);
        printf("Processes = %d\n", comm_size);

        int *a = malloc(sizeof(int) * size);
        int *temp = malloc(sizeof(int) * size);

        if (a == NULL || temp == NULL)
            MPI_Abort(MPI_COMM_WORLD, 1);

        // Initialize vector
        srand(314159);

        for (int i = 0; i < size; i++)
            a[i] = rand() % size;

        double start = MPI_Wtime();

        mergesort_unbalanced(a, size, temp, my_rank, comm_size, MPI_COMM_WORLD);

        double end = MPI_Wtime();

         printf ("Start = %.2f\nEnd = %.2f\nElapsed = %.2f\n",start, end, end - start);

        free(a);
        free(temp);

    } else {
      run_helper_mpi(my_rank, comm_size, MPI_COMM_WORLD);
    }

    MPI_Finalize();

    return 0;
}


// --------------------------------------------------
// ROOT / RECURSIVE MERGE SORT
// --------------------------------------------------

void mergesort_unbalanced(int a[], int size, int temp[], int rank, int comm_size, MPI_Comm comm) {

    int left_child = 2 * rank + 1;
    int right_child = 2 * rank + 2;

    int has_left = (left_child < comm_size);
    int has_right = (right_child < comm_size);

    MPI_Status status;

    // No useful division possible: sort locally
    if (size <= 1 || (!has_left && !has_right)) {

        mergesort_serial(a, size, temp);

        stop_descendants(rank, comm_size, comm);

        return;
    }

    // CASE 1: Two children available
    if (has_left && has_right) {

        int n1 = size / 2;
        int n2 = size - n1;

        // Send first half to left child
        MPI_Send(a, n1, MPI_INT, left_child, TAG_DATA, comm);

        // Send second half to right child
        MPI_Send(a + n1, n2, MPI_INT, right_child, TAG_DATA, comm);

        // Receive first sorted half
        MPI_Recv(a, n1, MPI_INT, left_child, TAG_DATA, comm, &status);

        // Receive second sorted half
        MPI_Recv(a + n1, n2, MPI_INT, right_child, TAG_DATA, comm, &status);

        // Merge both sorted halves
        merge(a, n1, n2, temp);
    }

    // CASE 2: Only one child available
    else if (has_left) {

        // Delegate the entire vector to the only child.
        // The parent does not sort any part locally.

        MPI_Send(a, size, MPI_INT, left_child, TAG_DATA, comm);

        MPI_Recv(a, size, MPI_INT, left_child, TAG_DATA, comm, &status);
    }

    return;
}


// --------------------------------------------------
// HELPER PROCESS
// --------------------------------------------------

void run_helper_mpi(int rank, int comm_size, MPI_Comm comm) {

    MPI_Status status;

    int size;
    int parent_rank;

    // Wait for work or termination signal
    MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &status);

    parent_rank = status.MPI_SOURCE;

    // Check whether this process received a STOP message
    if (status.MPI_TAG == TAG_STOP) {

        MPI_Recv(NULL, 0, MPI_INT, parent_rank, TAG_STOP, comm, &status);

        stop_descendants(rank, comm_size, comm);

        return;
    }

    // Discover received vector size
    MPI_Get_count(&status, MPI_INT, &size);

    int *a = malloc(sizeof(int) * size);
    int *temp = malloc(sizeof(int) * size);

    if (a == NULL || temp == NULL)
        MPI_Abort(comm, 1);

    // Receive vector
    MPI_Recv(a, size, MPI_INT, parent_rank, TAG_DATA, comm, &status);

    // Process vector recursively
    mergesort_unbalanced(a, size, temp, rank, comm_size, comm);

    // Return sorted vector to parent
    MPI_Send(a, size, MPI_INT, parent_rank, TAG_DATA, comm);

    free(a);
    free(temp);
}


// --------------------------------------------------
// STOP UNUSED DESCENDANTS
// --------------------------------------------------

void stop_descendants(int rank, int comm_size, MPI_Comm comm) {

    int left_child = 2 * rank + 1;
    int right_child = 2 * rank + 2;

    if (left_child < comm_size) {

        MPI_Send(NULL, 0, MPI_INT, left_child, TAG_STOP, comm);
    }

    if (right_child < comm_size) {

        MPI_Send(NULL, 0, MPI_INT, right_child, TAG_STOP, comm);
    }
}


// --------------------------------------------------
// SERIAL MERGE SORT
// --------------------------------------------------

void mergesort_serial(int a[], int size, int temp[]) {

    if (size <= SMALL) {
        BubbleSort(a, size);
        return;
    }

    int n1 = size / 2;
    int n2 = size - n1;

    mergesort_serial(a, n1, temp);
    mergesort_serial(a + n1, n2, temp);

    merge(a, n1, n2, temp);
}


// --------------------------------------------------
// MERGE
// --------------------------------------------------

void merge(int a[], int n1, int n2, int temp[]) {

    int i = 0;
    int j = n1;
    int k = 0;

    int total = n1 + n2;

    while (i < n1 && j < total) {

        if (a[i] <= a[j])
            temp[k++] = a[i++];

        else
            temp[k++] = a[j++];
    }

    while (i < n1)
        temp[k++] = a[i++];

    while (j < total)
        temp[k++] = a[j++];

    memcpy(a, temp, total * sizeof(int));
}


// --------------------------------------------------
// BUBBLE SORT
// --------------------------------------------------

void BubbleSort(int a[], int size) {

    for (int i = 0; i < size - 1; i++) {

        for (int j = 0; j < size - i - 1; j++) {

            if (a[j] > a[j + 1]) {

                int aux = a[j];
                a[j] = a[j + 1];
                a[j + 1] = aux;
            }
        }
    }
}
