#ifndef HFLOAT_H
#define HFLOAT_H

#define HFLOAT_1_0 0x3C00
#define HFLOAT_NEG_1_0 0xBC00

typedef unsigned short hfloat;

hfloat float_to_hfloat(float fp);

#endif /* HFLOAT_H */
