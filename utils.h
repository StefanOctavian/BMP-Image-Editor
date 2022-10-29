#pragma once
#include <stdio.h>
#include <stdlib.h>
#include "bmp_struct.h"

short edit(bmp*, char*);
void save(bmp*, char*);
short insert(bmp*, char*, unsigned, unsigned);
void draw_line(bmp*, bgr, unsigned char, int, int, int, int);
void draw_rect(bmp*, bgr, unsigned char, int, int, int, int);
void draw_tri(bmp*, bgr, unsigned char, int, int, int, int, int, int);
void fill(bmp*, bgr, int, int);
void dealloc(bmp*);

// Sterea Stefan Octavian
// 314CB

