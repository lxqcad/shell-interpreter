#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

char * get_home_dir(char *name)
{
	struct passwd *p;
	p=getpwnam(name);
	return (p->pw_dir);
}
