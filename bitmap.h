#ifndef _BITMAP_H_
#define _BITMAP_H_


typedef unsigned char  BYTE; // 1byte
typedef unsigned short  WORD; // 2bytes
typedef unsigned long  DWORD; //4bytes
typedef long LONG;// 4 bytes

#pragma pack(2)     // set alignment to 2 byte boundary
typedef struct tagBITMAPFILEHEADER
{
	WORD bfType;  //specifies the file type
	DWORD bfSize;  //specifies the size in bytes of the bitmap file
	WORD bfReserved1;  //reserved; must be 0
	WORD bfReserved2;  //reserved; must be 0
	DWORD bOffBits;  //species the offset in bytes from the bitmapfileheader to the bitmap bits
}BITMAPFILEHEADER;

#pragma pack(2)     // set alignment to 2 byte boundary
typedef struct tagBITMAPINFOHEADER
{
	DWORD biSize;  //specifies the number of bytes required by the struct
	LONG biWidth;  //specifies width in pixels
	LONG biHeight;  //species height in pixels
	WORD biPlanes; //specifies the number of color planes, must be 1
	WORD biBitCount; //specifies the number of bit per pixel
	DWORD biCompression;//spcifies the type of compression
	DWORD biSizeImage;  //size of image in bytes
	LONG biXPelsPerMeter;  //number of pixels per meter in x axis
	LONG biYPelsPerMeter;  //number of pixels per meter in y axis
	DWORD biClrUsed;  //number of colors used by th ebitmap
	DWORD biClrImportant;  //number of colors that are important
}BITMAPINFOHEADER;

unsigned char *LoadBitmapFile(char *filename, BITMAPINFOHEADER *bitmapInfoHeader);

#endif