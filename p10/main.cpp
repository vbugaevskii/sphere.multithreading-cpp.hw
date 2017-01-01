// exp(x) = sum x^i/i!, i=0,n

#include "mpi.h"

int main(int argc, char* argv[])
{
    int n_max = 25;
    double x = 1.0;

    MPI_Init(&argc, &argv);

    int size, rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int n = (n_max + 1) / size + (n_max % size != 0);
    int i_begin = rank * n, i_end = (rank + 1) * n;

    double f = 1.0, sum = 0.0;
    for (int i = i_begin; i <= ((i_end > n) ? n : i_end); i++)
    {
        f *= i ? x / i : 1.0;
        sum += f;
    }

    if (rank != size - 1)
        MPI_Send(&f, 1, MPI_DOUBLE, rank+1, 0, MPI_COMM_WORLD);

    if (rank)
    {
        MPI_Recv(&f, 1, MPI_DOUBLE, rank-1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        sum *= f;
    }

    double res = 0.0;
    MPI_Reduce(&sum, &res, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0)
        printf("%f\n", res);

    MPI_Finalize();
    return 0;
}
