#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include "Structures.h"


const UInt32 DEVICE_BLOCK_SIZE = 512;
const char* device = "\\\\.\\K:";


void ReadFromDev(int count, const char* device)
{
	char *buf = new char[count];
	FILE *f = fopen(device, "rb");
	FILE *fout = fopen("a.txt", "wb");
	int r;
	fread(buf, count, 1, f);
	fwrite(buf, count, 1, fout);

	fclose(f);
	fclose(fout);
	delete buf;
}

void ReadSupeblock(Ext2SuperBlock &SuperBlock, const char* device)
{	
	FILE *f;
	f = fopen(device, "rb+");
	//skip MBR
	int r = 0;
	r = fseek(f, 1024, SEEK_SET);
	r = fread(&SuperBlock, sizeof(SuperBlock), 1, f);
	//printf("Readed %d bytes", r);	
	fclose(f);
}

void GetInode(const char* device, UInt32 &InodeTableAdress, UInt32 &InodeNumber)
{
	FILE *f, *fout;
	f = fopen(device, "rb+");
	//Skip to inode adress


}

void ReadGroupDescriptor(std::vector <Ext2GroupDescriptor> &GrpDscrTbl, Ext2GroupDescriptor &GrpDscr, const char* device, UInt32 BlockSize, UInt32 DescriptorNumber)
{
	BlockSize = (1024 << 2);
	UInt32 seek = BlockSize + (DescriptorNumber / DEVICE_BLOCK_SIZE) * DEVICE_BLOCK_SIZE;//Наркомания какая %-)
	FILE *f, *fout;
	f = fopen(device, "rb+");
	//skip some device blocks to archive right GroupDescriptor
	int r = 0;
	int GrpDscrSize = sizeof(GrpDscr);
	r = fseek(f, seek, SEEK_SET);
	for(int i = 0; i < DEVICE_BLOCK_SIZE/GrpDscrSize; ++i)
		r = fread(&GrpDscrTbl[i], GrpDscrSize, 1, f);
	GrpDscr = GrpDscrTbl[DescriptorNumber];
	fclose(f);
}

void main()
{
	//ReadFromDev(8192);
	Ext2SuperBlock SuperBlock;
	ReadSupeblock(SuperBlock, device);
	Ext2GroupDescriptor GrpDscr;
	std::vector <Ext2GroupDescriptor> GrpDscrTbl(DEVICE_BLOCK_SIZE/sizeof(GrpDscr));
	ReadGroupDescriptor(GrpDscrTbl, GrpDscr, device, SuperBlock.s_log_block_size, 0);
	ReadFromDev(5000, device);
}