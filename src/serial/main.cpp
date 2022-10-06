#include <iostream>
#include <unistd.h>
#include <fstream>
#include <vector>
#include <algorithm>
#include <chrono>

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

Pixel **pixels;

int rows;
int cols;

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

void getPixlesFromBMP24(int end, int rows, int cols, char *fileReadBuffer)
{
  int count = 1;
  int extra = cols % 4;
  for (int i = 0; i < rows; i++)
  {
    count += extra;
    for (int j = cols - 1; j >= 0; j--)
      for (int k = 0; k < 3; k++)
      {
        switch (k)
        {
        case 0:
          // fileReadBuffer[end - count] is the red value
          pixels[i][j].R = fileReadBuffer[end - count];
          break;
        case 1:
          // fileReadBuffer[end - count] is the green value
          pixels[i][j].G = fileReadBuffer[end - count];
          break;
        case 2:
          // fileReadBuffer[end - count] is the blue value
          pixels[i][j].B = fileReadBuffer[end - count];
          break;
        // go to the next position in the buffer
        }
        count++;
      }
      
  }
}

void writeOutBmp24(char *fileBuffer, const char *nameOfFileToCreate, int bufferSize)
{
  std::ofstream write(nameOfFileToCreate);
  if (!write)
  {
    cout << "Failed to write " << nameOfFileToCreate << endl;
    return;
  }
  int count = 1;
  int extra = cols % 4;
  for (int i = 0; i < rows; i++)
  {
    count += extra;
    for (int j = cols - 1; j >= 0; j--)
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
  write.write(fileBuffer, bufferSize);
}

void hmirror() {
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols / 2; j++) {
      Pixel tmp = pixels[i][j];
      pixels[i][j] = pixels[i][cols-1-j];
      pixels[i][cols-1-j] = tmp;
    }
  }
}

void vmirror() {
  for (int i = 0; i < rows / 2; i++) {
    for (int j = 0; j < cols; j++) {
      Pixel tmp = pixels[i][j];
      pixels[i][j] = pixels[rows-1-i][j];
      pixels[rows-1-i][j] = tmp;
    }
  }
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

void median() {
  Pixel **npixels = new Pixel*[rows];
  for (int i = 0; i < rows; i++) {
    npixels[i] = new Pixel[cols];
    for (int j = 0; j < cols; j++) {
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
  for (int i = 0; i < rows; i++)
    free(pixels[i]);
  free(pixels);
  pixels = npixels;
}

void invert() {
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++) {
      pixels[i][j].R = 255 - pixels[i][j].R;
      pixels[i][j].G = 255 - pixels[i][j].G;
      pixels[i][j].B = 255 - pixels[i][j].B;
    }
  }
}

void partition() {
  int vmid = rows / 2;
  int hmid = cols / 2;
  for (int i = -1; i < 2; i++) {
    for (int j = 0; j < cols; j++) {
      pixels[i + vmid][j].R = 255;
      pixels[i + vmid][j].G = 255;
      pixels[i + vmid][j].B = 255;
    }
  }
  for (int i = 0; i < rows; i++) {
    for (int j = -1; j < 2; j++) {
      pixels[i][j + hmid].R = 255;
      pixels[i][j + hmid].G = 255;
      pixels[i][j + hmid].B = 255;
    }
  }
}

int main(int argc, char *argv[])
{
  auto start = high_resolution_clock::now();
  char *fileBuffer;
  int bufferSize;
  char *fileName = argv[1];
  if (!fillAndAllocate(fileBuffer, fileName, rows, cols, bufferSize))
  {
    cout << "File read error" << endl;
    return 1;
  }
  pixels = new Pixel*[rows];
  for (int i = 0; i < rows; i++)
    pixels[i] = new Pixel[cols];
  getPixlesFromBMP24(bufferSize, rows, cols, fileBuffer);
  hmirror();
  vmirror();
  median();
  invert();
  partition();
  writeOutBmp24(fileBuffer, "output.bmp", bufferSize);
  auto stop = high_resolution_clock::now();
  auto duration = duration_cast<microseconds>(stop - start);
  cout << "Execution Time: " << duration.count() / 1000000.0 << endl;
  return 0;
}