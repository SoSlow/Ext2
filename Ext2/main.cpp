#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Structures.h"


const UInt32 BLOCK_SIZE = 512;


void ReadFromDev(int count)
{
	char *buf = new char[count];
	FILE *f = fopen("\\\\.\\O:", "rb");
	FILE *fout = fopen("a.txt", "wb");
	
	fread(buf, count, 1, f);
	fwrite(buf, count, 1, fout);

	fclose(f);
	fclose(fout);
	delete buf;
}

void main()
{
	//ReadFromDev(8192);
	Ext2SuperBlock SuperBlock;
	FILE *f, *fout;
	int r = 0;
	char buf[4096];
	char block[1024];

	f = fopen("\\\\.\\O:", "rb+");

	//skeep MBR
	r = fseek(f, 1024, SEEK_SET);
	
	r = fread(&SuperBlock, sizeof(SuperBlock), 1, f);
	printf("Readed %d bytes", r);

	//r = fseek(f, 1024, SEEK_SET);
	Ext2GroupDescriptorTable GrpDscrTbl;
	r = fread(&GrpDscrTbl, sizeof(GrpDscrTbl), 1, f);

	fclose(f);
}