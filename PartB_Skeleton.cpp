#include <algorithm>
#include <cstdlib>
#include <cctype>
#include <cmath>
#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>
#include <mutex>
#include <vector>
#include <thread>
using namespace std;

/* Global variables, Look at their usage in main() */
int image_height;
int image_width;
int image_maxShades;
int inputImage[1000][1000];
int outputImage[1000][1000];
int num_threads;
int chunkSize;
int maxChunk;
int chunkNumber = 0;
int maskX[3][3];
int maskY[3][3];
mutex chunckIncrease;

/*
1. while chunks are left
2. increase chunk count with mutex
3. given the height, caclulate the range of lines in this chunck
4. create the 2 sobel masks
5. filter the masks (inverting colors) and add value to output array
*/

void filter(int start_point, int end_point) {
  for (int x = start_point; x < end_point; ++x) {
    for (int y = 0; y < image_width; ++y) {
      int sumx = 0;
      int sumy = 0;
      int sum = 0;

      if(x == 0 || x == (image_height -1) || y == 0 || y == (image_width -1)) {
        sum = 0;
      } else {
        /* Gradient calculation in X Dimension */
        for (int i=-1; i <= 1; i++) {
          for (int j = -1; j <= 1; j++) {
            sumx += inputImage[x+i][y+j] * maskX[i+1][j+1];
          }
        }

        /* Gradient calculation in Y Dimension */
        for (int i=-1; i <= 1; i++) {
          for (int j = -1; j <= 1; j++) {
            sumy += inputImage[x+i][y+j] * maskY[i+1][j+1];
          }
        }
        sum = abs(sumx) + abs(sumy);
      }

      if (sum < 0) {
        sum = 0;
      } else if (sum > 255) {
        sum = 255;
      }

      outputImage[x][y] = sum;
    }
  }
}

void applyFilter(int w) {
  /* 3x3 Sobel mask for X Dimension. */
  maskX[0][0] = -1; maskX[0][1] = 0; maskX[0][2] = 1;
  maskX[1][0] = -2; maskX[1][1] = 0; maskX[1][2] = 2;
  maskX[2][0] = -1; maskX[2][1] = 0; maskX[2][2] = 1;
  /* 3x3 Sobel mask for Y Dimension. */
  maskY[0][0] = 1; maskY[0][1] = 2; maskY[0][2] = 1;
  maskY[1][0] = 0; maskY[1][1] = 0; maskY[1][2] = 0;
  maskY[2][0] = -1; maskY[2][1] = -2; maskY[2][2] = -1;

  while (maxChunk > 0) {
    chunckIncrease.lock();
    int start_point = chunkNumber * chunkSize;
    int end_point = min((start_point + chunkSize), image_height);
    ++chunkNumber;
    -- maxChunk;
    chunckIncrease.unlock();

    if(maxChunk < 0) {
      break;
    }
    filter(start_point, end_point);
  }
}




/* ****************Change and add functions below ***************** */
void dispatch_threads()
{
  // cout << "dispatching threads" << endl;
  vector<thread> threads;
  for (int i=0; i < num_threads; i++) {
    threads.push_back(thread(&applyFilter, i));
  }

  for (auto& thrd : threads) {
    if(thrd.joinable()) {
      thrd.join();
    }
  }
}


/* ****************Need not to change the function below ***************** */

int main(int argc, char* argv[])
{
    if(argc != 5)
    {
        std::cout << "ERROR: Incorrect number of arguments. Format is: <Input image filename> <Output image filename> <Threads#> <Chunk size>" << std::endl;
        return 0;
    }

    std::ifstream file(argv[1]);
    if(!file.is_open())
    {
        std::cout << "ERROR: Could not open file " << argv[1] << std::endl;
        return 0;
    }
    num_threads = std::atoi(argv[3]);
    chunkSize  = std::atoi(argv[4]);

    std::cout << "Detect edges in " << argv[1] << " using " << num_threads << " threads\n" << std::endl;

    /* ******Reading image into 2-D array below******** */

    std::string workString;
    /* Remove comments '#' and check image format */
    while(std::getline(file,workString))
    {
      if( workString.at(0) != '#' ){
            if( workString.at(1) != '2' ){
                std::cout << "Input image is not a valid PGM image" << std::endl;
                return 0;
            } else {
                break;
            }
        } else {
            continue;
        }
    }
    /* Check image size */
    while(std::getline(file,workString))
    {
        if( workString.at(0) != '#' ){
            std::stringstream stream(workString);
            int n;
            stream >> n;
            image_width = n;
            stream >> n;
            image_height = n;
            break;
        } else {
            continue;
        }
    }

    /* maxChunk is total number of chunks to process */
    maxChunk = ceil((float)image_height/chunkSize);

    /* Check image max shades */
    while(std::getline(file,workString))
    {
        if( workString.at(0) != '#' ){
            std::stringstream stream(workString);
            stream >> image_maxShades;
            break;
        } else {
            continue;
        }
    }
    /* Fill input image matrix */
    int pixel_val;
    for( int i = 0; i < image_height; i++ )
    {
        if( std::getline(file,workString) && workString.at(0) != '#' ){
            std::stringstream stream(workString);
            for( int j = 0; j < image_width; j++ ){
                if( !stream )
                    break;
                stream >> pixel_val;
                inputImage[i][j] = pixel_val;
            }
        } else {
            continue;
        }
    }

    /************ Function that creates threads and manage dynamic allocation of chunks *********/
    dispatch_threads();

    /* ********Start writing output to your file************ */
    std::ofstream ofile(argv[2]);
    if( ofile.is_open() )
    {
        ofile << "P2" << "\n" << image_width << " " << image_height << "\n" << image_maxShades << "\n";
        for( int i = 0; i < image_height; i++ )
        {
            for( int j = 0; j < image_width; j++ ){
                ofile << outputImage[i][j] << " ";
            }
            ofile << "\n";
        }
    } else {
        std::cout << "ERROR: Could not open output file " << argv[2] << std::endl;
        return 0;
    }
    return 0;
}
