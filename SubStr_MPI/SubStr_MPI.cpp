#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <time.h>
#include <ctime>
#include <vector>
#include <fstream>
#include <string>
#include <mpi.h>

using namespace std;

int ProcNum = 0;      // Number of available processes 
int ProcRank = 0;     // Rank of current process


void Str(int* pProcRows, int RowNum, string a, string b)
{
    int* maxxArr = new int[ProcNum];
    int* indices = new int[ProcNum];
    int maxx;
    int index_a;
    int a_size = a.size();
    int b_size = b.size();

    MPI_Scatter(maxxArr, 1, MPI_INT, &maxx, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatter(indices, 1, MPI_INT, &index_a, 1, MPI_INT, 0, MPI_COMM_WORLD);

    //if (ProcRank == 0)
    for (int i = 0; i < RowNum; i++)
    {
        int tmpMax = 0;
        int t1 = pProcRows[2 * i];
        int t2 = pProcRows[2 * i + 1];
        int tmpIndexA = t1;

        while ((t1 < a_size) && (t2 < b_size))
        {
            if (a[t1] != b[t2])
            {
                if (maxx < tmpMax)
                {
                    maxx = tmpMax;
                    index_a = tmpIndexA;
                }
                tmpMax = 0;
                tmpIndexA = t1 + 1;
            }
            else
            {
                tmpMax++;
            }
            t1++;
            t2++;
        }
        if (maxx < tmpMax)
        {
            maxx = tmpMax;
            index_a = tmpIndexA;
        }
    }

    MPI_Gather(&maxx, 1, MPI_INT, maxxArr, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Gather(&index_a, 1, MPI_INT, indices, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (ProcRank == 0)
    {
        int res_len = 0, res_ind = 0;
        for (int i = 0; i < ProcNum; i++)
        {
            if (res_len < maxxArr[i])
            {
                res_len = maxxArr[i];
                res_ind = indices[i];
            }
        }
        cout << endl << endl;
        for (int i = 0; i < res_len; i++)
            cout << a[res_ind + i];
    }


}

// Function for definition of matrix
void DataInitialization(int* pMatrix, int SizeA, int SizeB) {
    int i, j;  // Loop variables

    int count = 0;
    for (i = SizeA - 1; i >= 0; i--)
    {
        pMatrix[count * 2] = i;
        pMatrix[count * 2 + 1] = 0;
        count++;
    }

    for (i = 1; i < SizeB; i++)
    {
        pMatrix[count * 2] = 0;
        pMatrix[count * 2 + 1] = i;
        count++;
    }
}

// Function for memory allocation and data initialization
void ProcessInitialization(int*& pMatrix, int*& pProcRows, int& SizeA, int& SizeB, int& RowNum) {

    int RestRows; // Number of rows, that haven’t been distributed yet
    int i;        // Loop variable

    setvbuf(stdout, 0, _IONBF, 0);
    // Determine the number of matrix rows stored on each process
    RestRows = SizeA + SizeB - 1;
    for (i = 0; i < ProcRank; i++)
        RestRows = RestRows - RestRows / (ProcNum - i);
    RowNum = RestRows / (ProcNum - ProcRank);

    // Memory allocation
    pProcRows = new int[SizeA + SizeB - 1];

    // Obtain the values of initial objects elements
    if (ProcRank == 0) {
        // Initial matrix exists only on the pivot process
        pMatrix = new int[(SizeA + SizeB - 1) * 2];
        // Values of elements are defined only on the pivot process
        DataInitialization(pMatrix, SizeA, SizeB); // E, SizeR, SizeC);
    }
}

// Function for distribution of the initial objects between the processes
void DataDistribution(int* pMatrix, int* pProcRows, int SizeA, int SizeB, int RowNum) {
    int* pSendNum; // The number of elements sent to the process
    int* pSendInd; // The index of the first data element sent to the process
    int RestRows = SizeA + SizeB - 1; // Number of rows, that haven’t been distributed yet

    // Alloc memory for temporary objects
    pSendInd = new int[ProcNum];
    pSendNum = new int[ProcNum];

    // Define the disposition of the matrix rows for current process
    RowNum = ((SizeA + SizeB - 1) / ProcNum);
    pSendNum[0] = RowNum * 2;
    pSendInd[0] = 0;

    for (int i = 1; i < ProcNum; i++)
    {
        RestRows -= RowNum;
        RowNum = RestRows / (ProcNum - i);
        pSendNum[i] = RowNum * 2;
        pSendInd[i] = pSendInd[i - 1] + pSendNum[i - 1];
    }
    // Scatter the rows
    MPI_Scatterv(pMatrix, pSendNum, pSendInd, MPI_INT, pProcRows, pSendNum[ProcRank], MPI_INT, 0, MPI_COMM_WORLD);

    // Free the memory
    delete[] pSendNum;
    delete[] pSendInd;
}


// Function for computational process termination
void ProcessTermination(int* pMatrix, int* pProcRows) {
    if (ProcRank == 0)
        delete[] pMatrix;
    delete[] pProcRows;
}


int main(int argc, char* argv[])
{
    string a = "", b = "", result, c;
    int* pMatrix{};
    int* pProcRows{};   // Stripe of the matrix on the current process
    int RowNum{};          // Number of rows in the matrix stripe
    double Start, Finish, Duration;

    setlocale(LC_ALL, "Russian");
    int a_size = 0;
    int b_size = 0;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &ProcNum);
    MPI_Comm_rank(MPI_COMM_WORLD, &ProcRank);

    if (ProcRank == 0)
    {
        ifstream a_file("C:\\Users\\Mary\\Desktop\\A.txt");
        if (a_file.is_open())
        {
            while (getline(a_file, c))
            {
                a += c;
            }
        }
        a_file.close();

        ifstream b_file("C:\\Users\\Mary\\Desktop\\B.txt");
        if (b_file.is_open())
        {
            while (getline(b_file, c))
            {
                b += c;
            }
        }
        b_file.close();

        a_size = a.size();
        b_size = b.size();

    }

    MPI_Barrier(MPI_COMM_WORLD);


    MPI_Bcast(&a_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&b_size, 1, MPI_INT, 0, MPI_COMM_WORLD);

    MPI_Bcast(&a, a.size(), MPI_CHAR, 0, MPI_COMM_WORLD);
    MPI_Bcast(&b, b.size(), MPI_CHAR, 0, MPI_COMM_WORLD);

    MPI_Barrier(MPI_COMM_WORLD);

    ProcessInitialization(pMatrix, pProcRows, a_size, b_size, RowNum);

    DataDistribution(pMatrix, pProcRows, a_size, b_size, RowNum);


    if (ProcRank == 0)
    {
        cout << a;
        cout << endl << endl << endl;
        cout << b;
        cout << endl;
    }
    MPI_Barrier(MPI_COMM_WORLD);

    Start = MPI_Wtime();

    Str(pProcRows, RowNum, a, b);

    Finish = MPI_Wtime();
    Duration = Finish - Start;

    if (ProcRank == 0)
    {
        printf("\nTime of execution = %f\n", Duration);
    }

    MPI_Finalize();

}