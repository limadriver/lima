#ifndef LIMARE_FROM_FLOAT_H
#define LIMARE_FROM_FLOAT_H 1

static inline unsigned int
from_float(float x)
{
	union cv {
		unsigned int i;
		float f;
	} cv;

	cv.f = x;
	return cv.i;
}

#endif /* LIMARE_FROM_FLOAT_H */
