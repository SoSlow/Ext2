#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include "Structures.h"

//global variables, must be replaced by class properties
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
	FILE *fout = fopen(".\\..\\..\\ext2.txt", "wb");
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
	r = fread(&SuperBlock, sizeof(Ext2SuperBlock), 1, f_dev);
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

void ReadInodeStruct(int inode_num, Ext2Inode &inode)
{
	int r = 0;
	int group_num = (inode_num - 1) / SuperBlock.s_inodes_per_group;
	int inode_local_num = (inode_num - 1) % SuperBlock.s_inodes_per_group;
	int inode_tbl_block = GrpDscrTbl[group_num].bg_inode_table;
	
	//fseek does't work with value that not divided by 512 (DEVICE_BLOCK_SIZE)
	//TODO: implement ext2_fseek() that can that
	//r = fseek(f_dev, inode_tbl_block*BLOCK_SIZE + inode_local_num*sizeof(Ext2Inode), SEEK_SET);
	r = fseek(f_dev, inode_tbl_block*BLOCK_SIZE, SEEK_SET);
	for(int i = 0; i <= inode_local_num + 1; ++i)
		r = fread(&inode, sizeof(Ext2Inode), 1, f_dev);

}

void ReadInodeData(int inode_num, char* &data)
{
	Ext2Inode inode;
	ReadInodeStruct(inode_num, inode);

	int data_size = inode.i_size;
	data = new char[data_size];

	//only direct links
	int cur = 0, size = 0;
	for(int i = 0; i < 13 && cur < data_size; ++i)
	{
		UInt32 block = inode.i_block[i];
		if(data_size - cur > BLOCK_SIZE) 
			size = BLOCK_SIZE;
		else 
			size = data_size - cur;

		fseek(f_dev, BLOCK_SIZE*block, SEEK_SET);
		fread(data + cur, 1, size, f_dev);
		cur += size;
	}
}

void ReadDir(int inode_num)
{
	Ext2DirEntry *direntry;
	char *data;
	ReadInodeData(inode_num, (char *)data);
	direntry = (Ext2DirEntry *)data;
	while(direntry->rec_len)
	{
		//printf("%s", 
	}

}

void main()
{
	try
	{
		//ReadFromDev(device_name, 4096*512);
		OpenDevice(device_name);
		ReadSupeblock(SuperBlock);
		ReadGroupDescriptorTable(GrpDscrTbl);
		
		ReadDir(2);


		CloseDevice();
	}catch(...)
	{
		printf("Error detected. Program is done.\n");
	}
}