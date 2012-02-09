#include "hfloat.h"

/*
 * Cleaned up copy of the routine from "OpenGL ES 2.0 Programming guide" by
 * Aaftab Munshi, Dan Ginsburg and Dave Shreiner. Code to be found in
 * Appendix A: GL_HALF_FLOAT_OES
 *
 * Book:      OpenGL(R) ES 2.0 Programming Guide
 * Authors:   Aaftab Munshi, Dan Ginsburg, Dave Shreiner
 * ISBN-10:   0321502795
 * ISBN-13:   9780321502797
 * Publisher: Addison-Wesley Professional
 * URLs:      http://safari.informit.com/9780321563835
 *            http://www.opengles-book.com
 *
 * The below license is inferred from the code they made available
 * online, and the publishing dates of the book.
 */

/*
 * (c) 2009 Aaftab Munshi, Dan Ginsburg, Dave Shreiner
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

hfloat
float_to_hfloat(float fp)
{
	unsigned int x = (*((unsigned int *) &fp));
	unsigned int sign = x >> 31;
	unsigned int mantissa = x & 0x007FFFFF;
	unsigned int exp = (x >> 23) & 0xFF;

	if (exp > 0x8F) { /* max of float -> hfloat */
		if (mantissa && (exp == 0xFF)) /* single max */
			mantissa = 0x7FFFFF; /* nan */
		else
			mantissa = 0; /* infinity */

		exp = 0x1F; /* half max ... well, infinity */
	} else if (exp < 0x70) { /* min of float to hfloat */
		exp = 0x70 - exp;
		if (exp > 14)
			mantissa = 0;
		else
			mantissa >>= (14 + exp); /* shift the mantissa away instead */
		exp = 0;
	} else {
		mantissa >>= 13;
		exp -= 0x70;
	}

	return (hfloat) ((sign << 15) | (exp << 10) | (mantissa));
}
