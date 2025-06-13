#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <mpi.h>
#include <unistd.h>

void read_matrix_from_csv(const char *filename, double *matrix, int rows, int cols) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Gagal membuka file");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            fscanf(fp, "%lf,", &matrix[i * cols + j]);
        }
    }

    fclose(fp);
}

void write_matrix_to_csv(const char *filename, double *matrix, int rows, int cols) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        perror("Gagal membuka file untuk menulis");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            fprintf(fp, "%.1f", matrix[i * cols + j]);
            if (j < cols - 1) fprintf(fp, ",");
        }
        fprintf(fp, "\n");
    }

    fclose(fp);
}

int main(int argc, char **argv) {
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Ambil ukuran matriks dari argumen
    if (argc < 2) {
        if (rank == 0)
            fprintf(stderr, "Usage: %s <matrix_size>\n", argv[0]);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    int N = atoi(argv[1]);
    int q = (int)sqrt(size);
    if (q * q != size || N % q != 0) {
        if (rank == 0)
            fprintf(stderr, "Jumlah proses harus kuadrat sempurna dan N harus habis dibagi q.\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    int dims[2] = {q, q};
    int periods[2] = {0, 0};
    MPI_Comm grid_comm;
    MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 1, &grid_comm);

    int coords[2];
    MPI_Cart_coords(grid_comm, rank, 2, coords);
    int row = coords[0], col = coords[1];

    MPI_Comm row_comm, col_comm;
    MPI_Comm_split(grid_comm, row, col, &row_comm);
    MPI_Comm_split(grid_comm, col, row, &col_comm);

    int block_size = N / q;
    double *A_block = malloc(block_size * block_size * sizeof(double));
    double *B_block = malloc(block_size * block_size * sizeof(double));
    double *C_block = calloc(block_size * block_size, sizeof(double));

    MPI_Barrier(MPI_COMM_WORLD);
    double start_time = MPI_Wtime();

    double *A = NULL, *B = NULL;
    if (rank == 0) {
        A = malloc(N * N * sizeof(double));
        B = malloc(N * N * sizeof(double));
        read_matrix_from_csv("matrixA.csv", A, N, N);
        read_matrix_from_csv("matrixB.csv", B, N, N);
    }

    // Distribusi matriks A dan B
    if (rank == 0) {
        for (int i = 0; i < q; ++i) {
            for (int j = 0; j < q; ++j) {
                int dst = i * q + j;
                for (int ii = 0; ii < block_size; ++ii) {
                    double *src_ptr_A = &A[(i * block_size + ii) * N + j * block_size];
                    double *src_ptr_B = &B[(i * block_size + ii) * N + j * block_size];

                    if (dst == 0) {
                        memcpy(&A_block[ii * block_size], src_ptr_A, block_size * sizeof(double));
                        memcpy(&B_block[ii * block_size], src_ptr_B, block_size * sizeof(double));
                    } else {
                        MPI_Send(src_ptr_A, block_size, MPI_DOUBLE, dst, 0, MPI_COMM_WORLD);
                        MPI_Send(src_ptr_B, block_size, MPI_DOUBLE, dst, 1, MPI_COMM_WORLD);
                    }
                }
            }
        }
    } else {
        for (int ii = 0; ii < block_size; ++ii)
            MPI_Recv(&A_block[ii * block_size], block_size, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        for (int ii = 0; ii < block_size; ++ii)
            MPI_Recv(&B_block[ii * block_size], block_size, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    double *A_panel = malloc(block_size * block_size * sizeof(double));
    double *B_panel = malloc(block_size * block_size * sizeof(double));

    for (int k = 0; k < q; ++k) {
        if (col == k) memcpy(A_panel, A_block, block_size * block_size * sizeof(double));
        MPI_Bcast(A_panel, block_size * block_size, MPI_DOUBLE, k, row_comm);

        if (row == k) memcpy(B_panel, B_block, block_size * block_size * sizeof(double));
        MPI_Bcast(B_panel, block_size * block_size, MPI_DOUBLE, k, col_comm);

        for (int i = 0; i < block_size; ++i)
            for (int j = 0; j < block_size; ++j)
                for (int l = 0; l < block_size; ++l)
                    C_block[i * block_size + j] +=
                        A_panel[i * block_size + l] * B_panel[l * block_size + j];
    }

    MPI_Barrier(MPI_COMM_WORLD);
    double end_time = MPI_Wtime();
    if (rank == 0)
        printf("Waktu eksekusi: %.6f detik\n", end_time - start_time);

    if (rank == 0) {
        double *C = malloc(N * N * sizeof(double));
        for (int i = 0; i < block_size; ++i)
            memcpy(&C[i * N], &C_block[i * block_size], block_size * sizeof(double));

        for (int p = 1; p < size; ++p) {
            int coords[2];
            MPI_Cart_coords(grid_comm, p, 2, coords);
            int r = coords[0], c = coords[1];
            for (int i = 0; i < block_size; ++i)
                MPI_Recv(&C[(r * block_size + i) * N + c * block_size], block_size,
                        MPI_DOUBLE, p, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        write_matrix_to_csv("matrix_C.csv", C, N, N);
        printf("Hasil disimpan ke matrix_C.csv\n");
        free(C);
    } else {
        for (int i = 0; i < block_size; ++i)
            MPI_Send(&C_block[i * block_size], block_size, MPI_DOUBLE, 0, 2, MPI_COMM_WORLD);
    }

    free(A_block); free(B_block); free(C_block);
    free(A_panel); free(B_panel);
    if (rank == 0) { free(A); free(B); }

    MPI_Finalize();
    return 0;
}
