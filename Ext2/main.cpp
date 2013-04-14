#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include "Structures.h"

//global variables, must be changed by class properties
FILE *f_dev = NULL;

Ext2SuperBlock SuperBlock;
Ext2GroupDescriptor *GrpDscrTbl = NULL;
UInt32 BLOCK_SIZE = 0;

//const
const UInt32 DEVICE_BLOCK_SIZE = 512;
const char* device_name = "\\\\.\\O:";



void OpenDevice(const char *dev_name)
{
	f_dev = fopen(dev_name, "rb");
	if(!f_dev) throw (0);
}


void CloseDevice()
{
	fclose(f_dev);	
}


void ReadFromDev(const char *dev_name, int count)
{
	char *buf = new char[count];
	FILE *f = fopen(dev_name, "rb");
	FILE *fout = fopen("device.txt", "wb");
	int r;
	fread(buf, count, 1, f);
	fwrite(buf, count, 1, fout);

	fclose(f);
	fclose(fout);
	delete buf;
}

void ReadSupeblock(Ext2SuperBlock &SuperBlock)
{	
	int r = 0;
	//skip MBR
	r = fseek(f_dev, 1024, SEEK_SET);
	r = fread(&SuperBlock, sizeof(SuperBlock), 1, f_dev);
	//printf("Readed %d bytes", r);	
	BLOCK_SIZE = 1024 << SuperBlock.s_log_block_size;
}

void ReadGroupDescriptorTable(Ext2GroupDescriptor* &GrpDscrTbl)
{
	if(GrpDscrTbl)
		delete GrpDscrTbl;
	int gd_count = SuperBlock.s_blocks_count/SuperBlock.s_blocks_per_group + 1;
	GrpDscrTbl = new Ext2GroupDescriptor[gd_count];
	
	//skip some device blocks to archive right GroupDescriptor
	int pos = (SuperBlock.s_first_data_block + 1)*BLOCK_SIZE;
	int r = fseek(f_dev, pos, SEEK_SET);
	fread(GrpDscrTbl, sizeof(Ext2GroupDescriptor), gd_count, f_dev);
}

void GetInode(UInt32 &InodeTableAdress, UInt32 &InodeNumber)
{
	//Skip to inode adress


}

void ReadInode(int inode_num)
{
	int r = 0;
	int group_num = (inode_num - 1) / SuperBlock.s_inodes_per_group;
	int inode_local_num = (inode_num - 1) % SuperBlock.s_inodes_per_group;
	Ext2Inode inode;
	int inode_tbl_block = GrpDscrTbl[group_num].bg_inode_table;
	
	//fseek don't work with value that not divided by 512 (DEVICE_BLOCK_SIZE)
	//TODO: implement ext2_fseek() that can that
	//r = fseek(f_dev, inode_tbl_block*BLOCK_SIZE + inode_local_num*sizeof(Ext2Inode), SEEK_SET);
	r = fseek(f_dev, inode_tbl_block*BLOCK_SIZE, SEEK_SET);
	for(int i = 0; i <= inode_local_num + 1; ++i)
		r = fread(&inode, sizeof(inode), 1, f_dev);
}

void main()
{
	try
	{
		ReadFromDev(device_name, 1024*512);
		OpenDevice(device_name);
		ReadSupeblock(SuperBlock);
		ReadGroupDescriptorTable(GrpDscrTbl);
		
		ReadInode(2);


		CloseDevice();
	}catch(...)
	{
		printf("Error detected. Program is done.\n");
	}
}