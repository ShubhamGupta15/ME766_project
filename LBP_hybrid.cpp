#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <cmath>
#include <omp.h>
#include "mpi.h"
using namespace std;

#define MASTER 0
#define FROM_MASTER 1

void create_histogram(int *hist, int **img, int num_rows, int num_cols)
{
  int  smallMatrix[3][3];
    //int i = 1;
    int decimal = 0;
#pragma omp parallel
    {
#pragma omp for
    for(int i = 1; i <= num_rows; i++)
    {
        for(int j = 1; j <= num_cols; j++)
        {
            if (img[i][j] <= img[i - 1][j - 1]) {
                smallMatrix[0][0] = 0;
                //cout << img[i][j] << " " << endl;
            }
            else{
                smallMatrix[0][0] = 1;
                // cout << img[i][j] << " " << endl;
            }
            if (img[i][j] <= img[i - 1][j]) {
                smallMatrix[0][1] = 0;
            }
            else  {
                smallMatrix[0][1] = 1;

            }
            if (img[i][j] <= img[i - 1][j + 1]) {
                smallMatrix[0][2] = 0;
            }
            else {
                smallMatrix[0][2] = 1;
            }
            if (img[i][j] <= img[i][j - 1]) {
                smallMatrix[1][0] = 0;
            }
            else {
                smallMatrix[1][0] = 1;
            }
            if (img[i][j] <= img[i][j + 1]) {
                smallMatrix[1][2] = 0;
            }
            else  {
                smallMatrix[1][2] = 1;
            }
            if (img[i][j] <= img[i + 1][j - 1]) {
                smallMatrix[2][0] = 0;
            }
            else {
                smallMatrix[2][0] = 1;
            }
            if (img[i][j] <= img[i + 1][j]) {
                smallMatrix[2][1] = 0;
            }
            else {
                smallMatrix[2][1] = 1;
            }
            if (img[i][j] <= img[i + 1][j + 1]) {
                smallMatrix[2][2] = 0;
            }
            else {
                smallMatrix[2][2] = 1;
            }
            decimal = smallMatrix[0][0] * int(pow(2, 7)) + smallMatrix[0][1] * int(pow(2, 6)) + smallMatrix[0][2] * int(pow(2, 5)) +
                      smallMatrix[1][2] * int(pow(2, 4)) +
                      smallMatrix[2][2] * int(pow(2, 3)) + smallMatrix[2][1] * int(pow(2, 2)) + smallMatrix[2][0] * int(pow(2, 1)) +
                      smallMatrix[1][0] * 1;

            hist[decimal]++;
        }

    }
            
}
}

double distance(int * a, int *b, int size)
{
  double distance = 0;
  for (int i = 0; i < size; i ++) {
      if (a[i] + b[i] == 0) {
          distance += 0;
      }
      else {
          distance += 0.5 * pow ((a[i]- b[i]), 2) / (a[i] + b[i]);
      }
  }
   // printf("nbefore returning from distance function\n");
    return  distance;
}
int find_closest(int ***training_set, int num_persons, int num_training, int size, int * test_image, double &closestValue)
{
  double ** dist = new double * [num_persons]; //make an array which will store the comparison values
    for (int i = 0; i < num_persons; i++) {
        dist[i] = new double [num_training];
    }
//omp_set_num_threads(6);
#pragma omp parallel
    {
#pragma omp for
    for (int i = 0;  i < num_persons; i++) { //populate dhe distance array
        for (int j =0; j < num_training; j++) {
            dist[i][j] = distance(training_set[i][j], test_image, size);
            }
        }
    }

    closestValue = dist[0][0];
    int  closest = 1e9 ;

    for (int i = 0;  i < num_persons; i++) {
        for (int j =0; j < num_training; j++) {
            if (dist[i][j] <= closestValue){
                closestValue = dist[i][j];
                closest = i;
        }
        }
    }
    for (int i = 0; i < num_persons; ++i) {
        delete dist[i];
    }
    delete []dist;
   // printf("before returning from find closest function\n");
    return closest + 1;
}
int **alloc_2d_matrix(int r, int c)
{
  int** a = new int*[r];
  for(int i = 0;i<r;i++)
  {
    a[i] = new int[c];
  } 
  return a;
}
void dealloc_2d_matrix(int **a, int r, int c)
{
  for(int i = 0;i<r;i++)
  {
    delete a[i];
  }
  delete [] a;
}
int ** read_pgm_file(string file_name, int h, int w)
{
  ifstream File;
    File.open(file_name);
    //cout << file_name << endl;
    
    int** data=alloc_2d_matrix(h,w);
    int tmp;
    for (int i = 0; i < h;i++) {
        for (int j = 0; j < w; j++) {
            File >> tmp;
            data[i][j] = tmp;
        //   cout <<data[i][j] ;

        }

    }

    File.close();
    return data;
}


int main(int argc, char** argv)
{
    clock_t r;
    r = clock();
    int myrank, num_pes;
    int provided = 0 ;
    MPI_Init_thread(&argc,&argv, MPI_THREAD_MULTIPLE, &provided);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_pes);
    MPI_Status status;


    int nrOfIds = 34;
    int nrOfPhotosPerId = 30;
    int num_rows = 125;
    int num_cols = 94;
    int histogramSize = 256;
    //int start_s=clock();
    int *** training_set = new int **[nrOfIds]; //nr of people, nr of images per person, histogram size

    r = clock() - r;
    // double start_s = omp_get_wtime();



    /****************************************************************************************************************************/
    int averows = nrOfIds/num_pes;
    int extra = nrOfIds%num_pes;
    int myIds = (myrank < extra) ? averows+1 : averows;
    int offset = 0;
    if(myrank == MASTER){
        for(int pe = 0; pe<num_pes; pe++){
            int rows = (pe < extra) ? averows+1 : averows;
            if (pe!=MASTER){
                MPI_Send(&offset, 1, MPI_INT, pe, FROM_MASTER, MPI_COMM_WORLD);
            }
            offset += rows;
        }
        offset = 0;
    }
    else{
        MPI_Recv(&offset, 1, MPI_INT, MASTER, FROM_MASTER, MPI_COMM_WORLD, &status);
    }


    /*****************************************************************************************************************************/


//omp_set_num_threads(6);
    myIds = (myrank < extra) ? averows+1 : averows;
#pragma omp parallel
    {
#pragma omp for
    for (int i = 0; i < myIds; i++) {
        training_set[i] = alloc_2d_matrix(nrOfPhotosPerId,histogramSize);
        }

#pragma omp for 
    for (int i = 0; i < myIds; i++) { //initialize  training set to 0
        for (int j = 0; j < nrOfPhotosPerId; j++) {
            for (int e = 0; e < histogramSize; e++) {
                training_set[i][j][e] = 0;
            }
        }
    }



    //get file name
    string filename;
    #pragma omp for
    for (int w = 1; w <= myIds; w++) {
        for (int q = 1; q <= nrOfPhotosPerId; q++) { //get all the file's names

            filename = "s" + to_string(w-1+offset) + "_" + to_string(q) + ".txt";
            //cout<<filename<<endl;


            int **image = read_pgm_file(filename, num_rows, num_cols);
            int **img = alloc_2d_matrix((num_rows + 2), (num_cols + 2)); //enhanced image matrix with 0 in the corners

            for (int i = 0; i < (num_rows + 2); i++) { //initialize enhanced img matrix  0
                for (int j = 0; j < (num_cols + 2); j++) {
                    img[i][j] = 0;
                }
            }
            for (int i = 1; i <= num_rows ; i++) { //copy data from the image to enhanced img matrix
                for (int j = 1; j <= num_cols; j++) {
                    img[i][j] = image[i - 1][j - 1];
                }
            }

            create_histogram(training_set[w - 1][q - 1], img, num_rows, num_cols);
            //deallocate images
            dealloc_2d_matrix(image, num_rows, num_cols);
            dealloc_2d_matrix(img, (num_rows + 2), (num_cols + 2));
        }
    }
    // double stop_s=omp_get_wtime();

    }

    //TESTING PART
    string filenames;
    filenames = "s" + to_string(27) + "_" + to_string(30) + ".txt";
            //cout<<filename<<endl;


    int **image11 = read_pgm_file(filenames, num_rows, num_cols);
    int **img11 = alloc_2d_matrix((num_rows + 2), (num_cols + 2)); //enhanced image matrix with 0 in the corners

    for (int i = 0; i < (num_rows + 2); i++) { //initialize enhanced img matrix  0
        for (int j = 0; j < (num_cols + 2); j++) {
            img11[i][j] = 0;
        }
    }
    for (int i = 1; i <= num_rows ; i++) { //copy data from the image to enhanced img matrix
        for (int j = 1; j <= num_cols; j++) {
            img11[i][j] = image11[i - 1][j - 1];
        }
    }

    int *A = new int[256];
    for(int i =0; i< 256; i++){
        A[i] = 0;
    }

    create_histogram(A, img11, num_rows, num_cols);
    // for(int i =0; i<256; i++){
    //     cout<<A[i]<<endl;
    // }

    //deallocate images
    dealloc_2d_matrix(image11, num_rows, num_cols);
    dealloc_2d_matrix(img11, (num_rows + 2), (num_cols + 2));


    int testResultId, closest=0;
    double closestValue;
    testResultId = find_closest(training_set,myIds,nrOfPhotosPerId,histogramSize,A, closestValue);
    // cout<<"from process"<<myrank<<" "<< closestValue<< " ID: "<< testResultId<< " with offset "<< offset<<endl;


    int global_closestValue;

    if(myrank == MASTER){
        global_closestValue = closestValue;
        closest = testResultId;
        for( int pe = 0; pe<num_pes; pe++){
            if(pe != MASTER){
                MPI_Recv(&testResultId, 1, MPI_INT, pe, pe,  MPI_COMM_WORLD, &status);
                MPI_Recv(&closestValue, 1, MPI_DOUBLE, pe , 100+pe, MPI_COMM_WORLD, &status);
                MPI_Recv(&offset, 1, MPI_INT, pe, 200+pe, MPI_COMM_WORLD, &status);
                if(global_closestValue > closestValue){
                    global_closestValue = closestValue;
                    closest = testResultId + offset;
                }
            }
        }
    }

    else{
        MPI_Send(&testResultId, 1, MPI_INT, MASTER, myrank, MPI_COMM_WORLD);
        MPI_Send(&closestValue, 1, MPI_DOUBLE, MASTER, 100+myrank, MPI_COMM_WORLD);
        MPI_Send(&offset, 1, MPI_INT, MASTER, 200+myrank, MPI_COMM_WORLD);
    }


    if(myrank == MASTER)
    cout<<"The ID of person is: "<<closest<<endl;

    MPI_Barrier(MPI_COMM_WORLD);
    cout<<"Sucess!"<<endl;
    // delete [] A;
    // double stop_s=omp_get_wtime();
    clock_t t;
    t = clock();


//omp_set_num_threads(6);
// #pragma  omp parallel
//     {
// #pragma omp for
//     for (int i = 0; i < nrOfIds; i++) {
//         dealloc_2d_matrix(training_set[i], nrOfPhotosPerId, 256);
//         }
//     }

MPI_Finalize();
return 0;
}