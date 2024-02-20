#include <stdio.h>
#include <syslog.h>
#include <string.h>

int main(int argc, void **argv)
{
	int status = 1;
	if (argc != 3)
		return 1;
	FILE *fp = NULL;
	fp = fopen(argv[1], "wb");
	if (fp != NULL)
	{
		fwrite(argv[2], 1, strlen(argv[2]), fp); 
		openlog(NULL, 0, LOG_USER); 
		syslog(LOG_DEBUG, "Writing %s to %s", (char *)argv[2], (char *)argv[1]);
		closelog();
		fclose(fp);
		status = 0;
	}
	return status;
}
