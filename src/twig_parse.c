#include "twig.h"
#include "supp/twig_bits.h"

static int twig_find_start_code(const uint32_t *data, int size, int start) {
	int pos, leading_zeros = 0;
	for (pos = start; pos < size; pos++) {
		if (data[pos] == 0x00)
			leading_zeros++;
		else if (data[pos] == 0x01 && leading_zeros >= 2)
			return pos - 2;
		else
			zeros = 0;
	}
	return -1;
}