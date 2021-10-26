#include <time.h>
int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	struct timespec res;
	clock_gettime(CLOCK_REALTIME, &res);
	return 0;
}
