#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
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

//read first "count" bytes from device and save them to file ext2.txt in project directory
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
	//The block size is computed using this 32bit value as the number of bits to shift left the value 1024
	BLOCK_SIZE = 1024 << SuperBlock.s_log_block_size;
}

void ReadGroupDescriptorTable(Ext2GroupDescriptor* &GrpDscrTbl)
{
	if(GrpDscrTbl)
		delete GrpDscrTbl;
	int gd_count = SuperBlock.s_blocks_count/SuperBlock.s_blocks_per_group + 1;
	GrpDscrTbl = new Ext2GroupDescriptor[gd_count];
	//skip SuperBlock to archive right GroupDescriptor
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

	char *ibuf = new char[SuperBlock.s_inode_size];
	for(int i = 0; i <= inode_local_num; ++i)
	{
		//cause inode size may be different, read them into buffer and copy to our 128bit structure
		//TODO: not stupid implementation of this
		r = fread(ibuf, SuperBlock.s_inode_size, 1, f_dev);
	}
	memcpy(&inode, ibuf, sizeof(Ext2Inode));
	delete ibuf;
}

void ReadInodeData(UInt32 inode_num, char* &data, int &d_size)
{
	Ext2Inode inode;
	ReadInodeStruct(inode_num, inode);

	int data_size = inode.i_size;
	data = new char[data_size];

	//only direct links
	//TODO: double and triple links
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
	d_size = data_size;
}

void ReadDir(int inode_num)
{	
	char *data, next_data;
	int size;
	Ext2DirEntry *direntry;

	ReadInodeData(inode_num, (char *)data, size);
	direntry = (Ext2DirEntry *)data;
	while(direntry->file_type < 8)
	{
		char fname[255];

		memcpy(fname, direntry->name, direntry->name_len);
		fname[direntry->name_len] = 0;

		Ext2Inode inode;
		ReadInodeStruct(direntry->inode, inode);

		printf("%s \t%s\n", fname, direntry->file_type == EXT2_FT_DIR ? "DIR": "FILE");

		//go to the next dir entry in direntry linked list
		direntry = (Ext2DirEntry *)((char *)direntry + direntry->rec_len);
	}
	delete data;
}

int GetSubdirInode(UInt32 dir_inode, const char *subdir_name)
{
	char *data;
	Ext2DirEntry *direntry;
	int size, res = -1;

	ReadInodeData(dir_inode, (char *)data, size);
	direntry = (Ext2DirEntry *)data;
	while(direntry->file_type < 8)
	{
		if(direntry->file_type == EXT2_FT_DIR)
		{
			char dir_name[255];
			memcpy(dir_name, direntry->name, direntry->name_len);
			dir_name[direntry->name_len] = 0;
			if(strcmp(subdir_name, dir_name) == 0)
			{
				res = direntry->inode;
				break;
			}
		}
		direntry = (Ext2DirEntry *)((char *)direntry + direntry->rec_len);
	}
	//free mem
	delete data;

	return res;
}
int GetDirInodeByName(char *path)
{
	char dir[255];
	char *first, *sec;
	int dir_inode = EXT2_ROOT_INO;

	first = path;
	while(dir_inode != -1)
	{
		first = strchr(first, '\\');
		if(first == NULL)
		{
			break;
		}
		sec = strchr(first+1, '\\');

		if(sec == NULL)
		{
			sec = first + strlen(first);
		}
		strncpy(dir, first + 1, sec - first);

		first = sec;
		if(strlen(dir) == 0) continue;

		dir_inode = GetSubdirInode(dir_inode, dir);		
	}

	return dir_inode;
}

int GetFileInode(UInt32 dir_inode, char *fname)
{
	Ext2DirEntry *direntry;
	char *data;
	int size, res = -1;

	ReadInodeData(dir_inode, (char *)data, size);
	direntry = (Ext2DirEntry *)data;

	while(direntry->file_type < 8)
	{
		if(direntry->file_type == EXT2_FT_REG_FILE)
		{
			char file_name[255];
			memcpy(file_name, direntry->name, direntry->name_len);
			file_name[direntry->name_len] = 0;
			if(strcmp(file_name, fname) == 0)
			{
				res = direntry->inode;
				break;
			}
		}

		//go to the next dir entry in direntry linked list
		direntry = (Ext2DirEntry *)((char *)direntry + direntry->rec_len);
	}

	//free mem
	delete data;

	return res;
}


void Ext2ls(char *path)
{
	int dir_inode = GetDirInodeByName(path);
	if(dir_inode != -1)
	{
		char *data;
		Ext2DirEntry *direntry;
		int size, res = -1;

		ReadInodeData(dir_inode, (char *)data, size);
		direntry = (Ext2DirEntry *)data;
		printf("%16s%16s\t%s\n", "Name", "Lehgth", "Modification time");
		printf("%16s%16s\t%s\n", "====", "======", "=================");
		printf("\n");
		while(direntry->file_type < 8)
		{
			char fname[255];
			Ext2Inode inode;

			memcpy(fname, direntry->name, direntry->name_len);
			fname[direntry->name_len] = 0;

			ReadInodeStruct(direntry->inode, inode);
			printf("%16s%16d\t%s", fname, direntry->file_type == EXT2_FT_REG_FILE ? inode.i_size : 0, ctime((time_t *)&inode.i_mtime));

			//go to the next dir entry in direntry linked list
			direntry = (Ext2DirEntry *)((char *)direntry + direntry->rec_len);
		}

		delete data;
	}
	else
	{
		printf("Error. Directory \"%s\" not found\n");
		return;
	}
}

void Ext2Cat(const char *path)
{
	int size, pos = -1;
	char *dir_path, *file_name, *data;
	int i = 0;
	for(; path[i]; ++i)
	{
		if(path[i] == '\\')
			pos = i;			
	}
	strncpy(dir_path, path, pos - 1);
	strncpy(file_name, &path[i], i - pos);
	int dir_inode = GetDirInodeByName(dir_path);
	if(dir_inode != -1)
	{
		int file_inode = GetFileInode(dir_inode, file_name);
		ReadInodeData(dir_inode, (char *)data, size);
		printf("s", data);
		delete data;
	}
	else
	{
		printf("Error. Directory \"%s\" not found\n");
		return;
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
		int k, size;
		char buf[255];
		char *data;


		k = GetDirInodeByName("\\lost+found");
		k = GetFileInode(2, "hello.txt");
		ReadInodeData(k, data, size);
		strncpy(buf, data, size);
		buf[size] = '\0';
		printf("%s\n", buf);
		Ext2ls("\\");
		Ext2Cat("H");

		CloseDevice();
	}catch(...)
	{
		printf("Error detected. Program is done.\n");
	}
}