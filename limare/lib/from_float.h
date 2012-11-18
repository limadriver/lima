#ifndef LIMARE_FROM_FLOAT_H
#define LIMARE_FROM_FLOAT_H 1

static inline unsigned int
from_float(float x)
{
	unsigned int i;
	memcpy(&i, &x, 4);
	return i;
}

#endif /* LIMARE_FROM_FLOAT_H */
