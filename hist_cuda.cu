#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <cmath>
using namespace std;

__global__ void create_histogram(int *hist, int *img, int *num_rows, int *num_cols){
	__shared__ int smallMatrix[3][3];
	__shared__ int decimal = 0;

	//each block handles one pixel in the image for histogram
	//hence each block has one small matrix

	int i = blockIdx.x;
	int j = blockIdx.y;
	int thx = threadIdx.x;
	
	
	if( img[i*num_cols j] < = img[(i - 1 + (thx / 3 ))*num_cols + j - 1 + (thx % 3)] ) {
		smallMatrix[(thx / 3 )][(thx % 3)] = 0;
	}
	else{
		smallMatrix[(thx / 3 )][(thx % 3)] = 1;
	}

	__syncthreads();

	if(threadIdx.x ==0){
		decimal = smallMatrix[0][0] * int(pow(2, 7)) + smallMatrix[0][1] * int(pow(2, 6)) + smallMatrix[0][2] * int(pow(2, 5)) +
		              smallMatrix[1][2] * int(pow(2, 4)) +
		              smallMatrix[2][2] * int(pow(2, 3)) + smallMatrix[2][1] * int(pow(2, 2)) + smallMatrix[2][0] * int(pow(2, 1)) +
		              smallMatrix[1][0] * 1;
	

		atomicAdd(*hist[decimal], 1);
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

int find_closest(int ***training_set, int num_persons, int num_training, int size, int * test_image)
{
  double ** dist = new double * [num_persons]; //make an array which will store the comparison values
    for (int i = 0; i < num_persons; i++) {
        dist[i] = new double [num_training];
    }
    for (int i = 0;  i < num_persons; i++) { //populate dhe distance array
        for (int j =0; j < num_training; j++) {
            dist[i][j] = distance(training_set[i][j], test_image, size);
        }
    }

    double closestValue = dist[0][0];
    int  closest = 1e9 ;

    for (int i = 0;  i < num_persons; i++) {
        for (int j =0; j < num_training; j++) {
            if (dist[i][j] < closestValue){
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

int ** read_image_data(string file_name, int h, int w)
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
int main()
{
    int nrOfIds = 34;
    int nrOfPhotosPerId = 30;
    int num_rows = 125;
    int num_cols = 94;
    int histogramSize = 256;
    int start_s=clock();
    int *hist, *d_img, *d_num_rows, *d_num_cols;


    int *** training_set = new int **[nrOfIds]; //nr of people, nr of images per person, histogram size
    for (int i = 0; i < nrOfIds; i++) {
        training_set[i] = alloc_2d_matrix(nrOfPhotosPerId,histogramSize);
    }

    for (int i = 0; i < nrOfIds; i++) { //initialize  training set to 0
        for (int j = 0; j < nrOfPhotosPerId; j++) {
            for (int e = 0; e < histogramSize; e++) {
                training_set[i][j][e] = 0;
            }
        }
    }

        //get file name

        string filename;
        for (int w = 1; w <= 9; w++) {
            for (int q = 1; q <= 5; q++) { //get all the file's names

                filename = "s0" + to_string(w) + "_0" + to_string(q) + "resized.txt";
                //cout<<filename<<endl;


                int **image = read_image_data(filename, num_rows, num_cols);
                int **img = alloc_2d_matrix((num_rows + 2), (num_cols + 2)); //enhanced image matrix with 0 in the corners

                for (int i = 0; i < (num_rows + 2); i++) { //initialize enhanced img matrix  0
                    for (int j = 0; j < (num_cols + 2); j++) {

                    	if(i==0 || j==0 || i == num_rows+1 || j == num_cols+1)  img[i][j] = 0;
                    	else img[i][j] = image[i - 1][j - 1];
                    }
                }

                cudaMalloc(void** &hist, sizeof(int)*histogramSize);
                cudaMalloc(void** &d_img, sizeof(int)*num_rows*num_cols);
                cudaMalloc(void** &d_num_cols, sizeof(int));
                cudaMalloc(void** &d_num_rows, sizeof(int));

                cudaMemcpy(d_img, img[0], sizeof(int)*num_rows*num_cols, cudaMemcpyHostToDevice);
                cudaMemcpy(d_num_rows, num_rows, sizeof(int), cudaMemcpyHostToDevice);
                cudaMemcpy(d_num_rows, num_cols, sizeof(int), cudaMemcpyHostToDevice);

				dim3 griddim(150,200);
				create_histogram<<<griddim,9>>>( hist, d_img, d_num_rows, d_num_cols);

                cudaMemcpy(training_set[w - 1][q - 1], hist, sizeof(int)*histogramSize, cudaMemcpyDeviceToHost);
                string err = cudaGetErrorString(cudaGetLastError ());
                cout<<err<<endl;
                cudaFree(hist);
                cudaFree(d_img);
                cudaFree(d_num_rows);
                cudaFree(d_num_cols);
                cudaDeviceSynchronize();

                //deallocate images
                dealloc_2d_matrix(image, num_rows, num_cols);
                dealloc_2d_matrix(img, (num_rows + 2), (num_cols + 2));
            }
        }


        //TESTING PART
        string filenames;
        filenames = "s" + to_string(36) + "_0" + to_string(1) + "resized.txt";
                //cout<<filename<<endl;


        int **image11 = read_image_data(filenames, num_rows, num_cols);
        int **img11 = alloc_2d_matrix((num_rows + 2), (num_cols + 2)); //enhanced image matrix with 0 in the corners

        for (int i = 0; i < (num_rows + 2); i++) { //initialize enhanced img matrix  0
            for (int j = 0; j < (num_cols + 2); j++) {

            	if(i==0 || j==0 || i == num_rows+1 || j == num_cols+1)  img[i][j] = 0;
            	else img[i][j] = image[i - 1][j - 1];
            }
        }

        int *A = new int[256];
        for(int i =0; i< 256; i++){
            A[i] = 0;
        }

        cudaMalloc(void** &hist, sizeof(int)*histogramSize);
                cudaMalloc(void** &d_img, sizeof(int)*num_rows*num_cols);
                cudaMalloc(void** &d_num_cols, sizeof(int));
                cudaMalloc(void** &d_num_rows, sizeof(int));

                cudaMemcpy(d_img, img11[0], sizeof(int)*num_rows*num_cols, cudaMemcpyHostToDevice);
                cudaMemcpy(d_num_rows, num_rows, sizeof(int), cudaMemcpyHostToDevice);
                cudaMemcpy(d_num_rows, num_cols, sizeof(int), cudaMemcpyHostToDevice);

				dim3 griddim(150,200);
				create_histogram<<<griddim,9>>>( hist, d_img, d_num_rows, d_num_cols);

                cudaMemcpy(A, hist, sizeof(int)*histogramSize, cudaMemcpyDeviceToHost);
                string err = cudaGetErrorString(cudaGetLastError ());
                cout<<err<<endl;
                cudaFree(hist);
                cudaFree(d_img);
                cudaFree(d_num_rows);
                cudaFree(d_num_cols);
                cudaDeviceSynchronize();

        for(int i =0; i<256; i++){
            //cout<<A[i]<<endl;
        }
        //deallocate images
        dealloc_2d_matrix(image11, num_rows, num_cols);
        dealloc_2d_matrix(img11, (num_rows + 2), (num_cols + 2));


        int testResultId;
        testResultId = find_closest(training_set,nrOfIds,nrOfPhotosPerId,histogramSize,A);
        cout<<testResultId<<endl;

        cout<<"Sucess!"<<endl;
        delete [] A;
}
