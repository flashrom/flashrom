#include <sys/utsname.h>
struct utsname osinfo;
int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	uname(&osinfo);
	return 0;
}
