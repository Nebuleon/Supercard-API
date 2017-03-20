/* Lifted directly from the C standard. */

static unsigned int rng_seed = 1;

int rand(void)
{
	rng_seed = rng_seed * 1103515245 + 12345;
	return (int) ((rng_seed / 65536) % 32768);
}

void srand(unsigned int seed)
{
	rng_seed = seed;
}
