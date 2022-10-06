#include <iostream>
#include <unistd.h>
#include <fstream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <pthread.h>

using namespace std;
using namespace std::chrono;

#pragma pack(1)
#pragma once


typedef int LONG;
typedef unsigned short WORD;
typedef unsigned int DWORD;

typedef struct tagBITMAPFILEHEADER
{
  WORD bfType;
  DWORD bfSize;
  WORD bfReserved1;
  WORD bfReserved2;
  DWORD bfOffBits;
} BITMAPFILEHEADER, *PBITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER
{
  DWORD biSize;
  LONG biWidth;
  LONG biHeight;
  WORD biPlanes;
  WORD biBitCount;
  DWORD biCompression;
  DWORD biSizeImage;
  LONG biXPelsPerMeter;
  LONG biYPelsPerMeter;
  DWORD biClrUsed;
  DWORD biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER;

typedef struct Pixel {
  char R;
  char G;
  char B;
} Pixel;

typedef struct worker_args {
  Pixel **npixels;
  int sx;
  int sy;
  int ex;
  int ey;
} worker_args;

Pixel **pixels;

int rows;
int cols;

char *fileBuffer;
int bufferSize;

bool fillAndAllocate(char *&buffer, const char *fileName, int &rows, int &cols, int &bufferSize)
{
  std::ifstream file(fileName);

  if (file)
  {
    file.seekg(0, std::ios::end);
    std::streampos length = file.tellg();
    file.seekg(0, std::ios::beg);

    buffer = new char[length];
    file.read(&buffer[0], length);

    PBITMAPFILEHEADER file_header;
    PBITMAPINFOHEADER info_header;

    file_header = (PBITMAPFILEHEADER)(&buffer[0]);
    info_header = (PBITMAPINFOHEADER)(&buffer[0] + sizeof(BITMAPFILEHEADER));
    rows = info_header->biHeight;
    cols = info_header->biWidth;
    bufferSize = file_header->bfSize;
    return 1;
  }
  else
  {
    cout << "File" << fileName << " doesn't exist!" << endl;
    return 0;
  }
}

void writeOutBmp24(const char *nameOfFileToCreate)
{
  std::ofstream write(nameOfFileToCreate);
  if (!write)
  {
    cout << "Failed to write " << nameOfFileToCreate << endl;
    return;
  }
  write.write(fileBuffer, bufferSize);
}

void *hmirror(void *arg) {
  worker_args *args = (worker_args *) arg;
  Pixel **npixels = args->npixels;
  int sx = args->sx;
  int sy = args->sy;
  int ex = args->ex;
  int ey = args->ey;
  for (int i = sx; i != ex; i++) {
    for (int j = sy; j != ey; j++) {
      npixels[i][cols-1-j] = pixels[i][j];
    }
  }
  return NULL;
}

void *vmirror(void *arg) {
  worker_args *args = (worker_args *) arg;
  Pixel **npixels = args->npixels;
  int sx = args->sx;
  int sy = args->sy;
  int ex = args->ex;
  int ey = args->ey;
  for (int i = sx; i != ex; i++) {
    for (int j = sy; j != ey; j++) {
      npixels[rows-1-i][j] = pixels[i][j];
    }
  }
  return NULL;
}

vector<Pixel> neighbours(int x, int y) {
  vector<Pixel> ns;
  for (int i = -1; i < 2; i++) {
    for (int j = -1; j < 2; j++) {
      if (x + i < 0 || x + i >= rows)
        continue;
      if (y + j < 0 || y + j >= cols)
        continue;
      ns.push_back(pixels[i + x][j + y]);
    }
  }
  return ns;
}



void *median(void *arg) {
  worker_args *args = (worker_args *) arg;
  Pixel **npixels = args->npixels;
  int sx = args->sx;
  int sy = args->sy;
  int ex = args->ex;
  int ey = args->ey;
  for (int i = sx; i != ex; i++) {
    for (int j = sy; j != ey; j++) {
    vector<Pixel> ns = neighbours(i, j);
    vector<char> Rs, Gs, Bs;
    for(Pixel &pixel : ns) {
      Rs.push_back(pixel.R);
      Gs.push_back(pixel.G);
      Bs.push_back(pixel.B);
    }
    sort(Rs.begin(), Rs.end());
    sort(Gs.begin(), Gs.end());
    sort(Bs.begin(), Bs.end());
    npixels[i][j].R = Rs[Rs.size() / 2];
    npixels[i][j].G = Gs[Gs.size() / 2];
    npixels[i][j].B = Bs[Bs.size() / 2];
    }
  }
  return NULL;
}


void *invert(void *arg) {
  worker_args *args = (worker_args *) arg;
  Pixel **npixels = args->npixels;
  int sx = args->sx;
  int sy = args->sy;
  int ex = args->ex;
  int ey = args->ey;
  for (int i = sx; i < ex; i++) {
    for (int j = sy; j < ey; j++) {
      npixels[i][j].R = 255 - pixels[i][j].R;
      npixels[i][j].G = 255 - pixels[i][j].G;
      npixels[i][j].B = 255 - pixels[i][j].B;
    }
  }
  return NULL;
}

void *partition(void *arg) {
  worker_args *args = (worker_args *) arg;
  Pixel **npixels = args->npixels;
  int sx = args->sx;
  int sy = args->sy;
  int ex = args->ex;
  int ey = args->ey;
  int hmid = rows / 2;
  int vmid = cols / 2;
  if (hmid >= sx && hmid < ex)
    for (int i = -1; i < 2; i++) {
      for (int j = sy; j != ey; j++) {
        pixels[i + hmid][j].R = 255;
        pixels[i + hmid][j].G = 255;
        pixels[i + hmid][j].B = 255;
      }
    }
  if (vmid >= sy && vmid < ey)
    for (int i = sx; i != ex; i++) {
      for (int j = -1; j < 2; j++) {
        pixels[i][j + vmid].R = 255;
        pixels[i][j + vmid].G = 255;
        pixels[i][j + vmid].B = 255;
      }
    }
  return NULL;
}

int n = 8;
pthread_t *ptids;
worker_args **args;
Pixel **npixels;

void init_workers() {
  ptids = new pthread_t[n*n];
  args = new worker_args*[n*n];
  npixels= new Pixel*[rows];
  for (int i = 0; i < rows; i++)
    npixels[i] = new Pixel[cols];
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      worker_args *arg = new worker_args;
      arg->npixels = npixels;
      arg->sx = i * rows / n;
      arg->sy = j * cols / n;
      arg->ex = i != n - 1 ? (i + 1) * rows / n : rows;
      arg->ey = j != n - 1 ? (j + 1) * cols / n : cols;
      args[i * n + j] = arg;
    }
  }
}

void swap_pixels() {
  for (int i = 0; i < rows; i++)
    free(pixels[i]);
  free(pixels);
  pixels = npixels;
  npixels= new Pixel*[rows];
  for (int i = 0; i < rows; i++)
    npixels[i] = new Pixel[cols];
  for (int i = 0; i < n*n; i++) {
    args[i]->npixels = npixels;
  }
}

void *read_and_hmirror(void *arg) {
  worker_args *args = (worker_args *) arg;
  Pixel **npixels = args->npixels;
  int sx = args->sx;
  int sy = args->sy;
  int ex = args->ex;
  int ey = args->ey;
  int count = 1;
  int extra = cols % 4;
  count += sx * (3 * cols + extra);
  for (int i = sx; i != ex; i++)
  {
    count += extra + 3 *(cols - ey);
    for (int j = ey - 1; j != sy - 1; j--) {
      for (int k = 0; k < 3; k++)
      {
        switch (k)
        {
        case 0:
          // fileReadBuffer[end - count] is the red value
          pixels[i][j].R = fileBuffer[bufferSize - count];
          break;
        case 1:
          // fileReadBuffer[end - count] is the green value
          pixels[i][j].G = fileBuffer[bufferSize - count];
          break;
        case 2:
          // fileReadBuffer[end - count] is the blue value
          pixels[i][j].B = fileBuffer[bufferSize - count];
          break;
        // go to the next position in the buffer
        }
        count++;
      }
    }
    count += 3 * sy;
  }
  hmirror(args);
  return NULL;
}

void *partition_and_write(void *arg) {
  partition(arg);
  worker_args *args = (worker_args *) arg;
  Pixel **npixels = args->npixels;
  int sx = args->sx;
  int sy = args->sy;
  int ex = args->ex;
  int ey = args->ey;
  int count = 1;
  int extra = cols % 4;
  count += sx * (3 * cols + extra);
  for (int i = sx; i != ex; i++)
  {
    count += extra + 3 *(cols - ey);
    for (int j = ey - 1; j != sy - 1; j--) {
      for (int k = 0; k < 3; k++)
      {
        switch (k)
        {
        case 0:
          // write red value in fileBuffer[bufferSize - count]
          fileBuffer[bufferSize - count] = pixels[i][j].R;
          break;
        case 1:
          // write green value in fileBuffer[bufferSize - count]
          fileBuffer[bufferSize - count] = pixels[i][j].G;
          break;
        case 2:
          // write blue value in fileBuffer[bufferSize - count]
          fileBuffer[bufferSize - count] = pixels[i][j].B;
          break;
        // go to the next position in the buffer
        }
        count++;
      }
    }
    count += 3 * sy;
  }
  return NULL;
}


void run_filters() {
  for (int i = 0; i < n*n; i++) {
    pthread_create(&ptids[i], NULL, &read_and_hmirror, (void *)(args[i]));
  }
  for (int i = 0; i < n*n; i++) {
    pthread_join(ptids[i], NULL);
  }
  swap_pixels();
  for (int i = 0; i < n*n; i++) {
    pthread_create(&ptids[i], NULL, &vmirror, (void *)(args[i]));
  }
  for (int i = 0; i < n*n; i++) {
    pthread_join(ptids[i], NULL);
  }
  swap_pixels();
  for (int i = 0; i < n*n; i++) {
    pthread_create(&ptids[i], NULL, &median, (void *)(args[i]));
  }
  for (int i = 0; i < n*n; i++) {
    pthread_join(ptids[i], NULL);
  }
  swap_pixels();
  for (int i = 0; i < n*n; i++) {
    pthread_create(&ptids[i], NULL, &invert, (void *)(args[i]));
  }
  for (int i = 0; i < n*n; i++) {
    pthread_join(ptids[i], NULL);
  }
  swap_pixels();
  for (int i = 0; i < n*n; i++) {
    pthread_create(&ptids[i], NULL, &partition_and_write, (void *)(args[i]));
  }
  for (int i = 0; i < n*n; i++) {
    pthread_join(ptids[i], NULL);
  }
}


int main(int argc, char *argv[])
{
  auto start = high_resolution_clock::now();
  char *fileName = argv[1];
  if (!fillAndAllocate(fileBuffer, fileName, rows, cols, bufferSize))
  {
    cout << "File read error" << endl;
    return 1;
  }
  pixels = new Pixel*[rows];
  for (int i = 0; i < rows; i++)
    pixels[i] = new Pixel[cols];
  init_workers();
  run_filters();
  writeOutBmp24("output.bmp");
  auto stop = high_resolution_clock::now();
  auto duration = duration_cast<microseconds>(stop - start);
  cout << "Execution Time: " << duration.count() / 1000000.0 << endl;
  return 0;
}