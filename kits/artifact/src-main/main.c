#include "windows.h"

void start(HINSTANCE handle);

int main(int argc, char * argv[]) {
	start(NULL);

	/* sleep so we don't exit */
	while (TRUE)
		WaitForSingleObject(GetCurrentProcess(), 10000);

	return 0;
}
