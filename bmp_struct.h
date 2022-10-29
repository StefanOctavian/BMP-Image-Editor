#pragma once
#include "bmp_header.h"

#pragma pack(1)
typedef struct {
  unsigned char b, g, r;
} bgr;
#pragma pack()

typedef struct {
  bmp_fileheader fileheader;
  bmp_infoheader infoheader;
  bgr** bitmap;
} bmp;

// Sterea Stefan Octavian
// 314CB

