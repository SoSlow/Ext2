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

std::string CURRENT_DIR = "/";
//const
const UInt32 DEVICE_BLOCK_SIZE = 512;
const char* device_name = "\\\\.\\S:";


std::string GetCurrentDirS()
{
	return(CURRENT_DIR);
}

const char *GetCurrentDirC()
{
	return(CURRENT_DIR.c_str());
}

void OpenDevice(const char *dev_name)
{
	f_dev = fopen(dev_name, "rb");
	if(!f_dev) throw (0);
}

void CloseDevice()
{
	fclose(f_dev);
	delete GrpDscrTbl;
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
	if(SuperBlock.s_magic != EXT2_SUPER_MAGIC)
		throw(0);

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

void ReadInodeStruct(UInt32 inode_num, Ext2Inode &inode)
{
	int r = 0;
	UInt32 group_num = (inode_num - 1) / SuperBlock.s_inodes_per_group;
	UInt32 inode_local_num = (inode_num - 1) % SuperBlock.s_inodes_per_group;
	UInt32 inode_tbl_block = GrpDscrTbl[group_num].bg_inode_table;
	
	//fseek does't work with value that not divided by 512 (DEVICE_BLOCK_SIZE)
	//TODO: implement ext2_fseek() that can that
	//we use _fseeki64 because native fseek dosen't support 64bit offset value. In the big drives this leads to errors
	//r = fseek(f_dev, inode_tbl_block*BLOCK_SIZE + inode_local_num*sizeof(Ext2Inode), SEEK_SET);
	r = _fseeki64(f_dev, inode_tbl_block*BLOCK_SIZE, SEEK_SET);

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

void ReadInodeData(UInt32 inode_num, char* &data, UInt32 &d_size)
{
	Ext2Inode inode;
	ReadInodeStruct(inode_num, inode);

	UInt32 data_size = inode.i_size;
	data = new char[data_size];

	//only direct links
	//TODO: double and triple links
	UInt32 cur = 0, size = 0;
	for(int i = 0; i < 13 && cur < data_size; ++i)
	{
		UInt32 block = inode.i_block[i];
		if(data_size - cur > BLOCK_SIZE) 
			size = BLOCK_SIZE;
		else 
			size = data_size - cur;

		_fseeki64(f_dev, BLOCK_SIZE*block, SEEK_SET);
		fread(data + cur, 1, size, f_dev);
		cur += size;
	}
	d_size = data_size;
}

UInt32 GetSubdirInode(UInt32 dir_inode, const char *subdir_name)
{
	char *data;
	Ext2DirEntry *direntry;
	UInt32 size, res = 0;

	ReadInodeData(dir_inode, (char *)data, size);
	direntry = (Ext2DirEntry *)data;
	while(direntry->name_len > 0)
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
		if((char *)direntry - data + direntry->rec_len >= size)
			break;
		//go to the next dir entry in direntry linked list		
		direntry = (Ext2DirEntry *)((char *)direntry + direntry->rec_len);
	}
	//free mem
	delete data;

	return res;
}

UInt32 GetDirInodeByName(char *path)
{
	char dir[255];
	char *first, *sec;
	int len = 0;
	UInt32 dir_inode = EXT2_ROOT_INO;

	first = path;
	do
	{
		first = strchr(first, '/');
		if(first == NULL)
		{
			dir_inode = 0;
			break;
		}
		//skip '/' char
		++first;
		//find the second '/' char position
		sec = strchr(first, '/');

		if(sec == NULL)
			len = strlen(first);
		else
			len = sec - first;

		strncpy(dir, first, len);
		dir[len] = '\0';

		first = sec;
		if(strlen(dir) == 0) continue;

		dir_inode = GetSubdirInode(dir_inode, dir);		
	}while(dir_inode != 0 && sec != NULL); //exit if no '/' char found or dir does not exist

	return dir_inode;
}

UInt32 GetFileInode(UInt32 dir_inode, char *fname)
{
	Ext2DirEntry *direntry;
	char *data;
	UInt32 size, res = 0;

	ReadInodeData(dir_inode, (char *)data, size);
	direntry = (Ext2DirEntry *)data;

	while(direntry->name_len > 0)
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

		if((char *)direntry - data + direntry->rec_len >= size)
			break;
		//go to the next dir entry in direntry linked list
		direntry = (Ext2DirEntry *)((char *)direntry + direntry->rec_len);
	}

	//free mem
	delete data;

	return res;
}

void RightsToString(UInt16 rights, char *buf)
{
	//TODO: change this shit
	buf[0] = (rights & 0x4000) ? 'd' : '-';
	
	for(int i = 0; i < 9; ++i)
	{
		UInt16 fl =  (rights >> (8 - i)) & 0x1;
		switch((i) % 3)
		{
			case 0:
				buf[i+1] = fl ? 'r' : '-';
			break;
			case 1:
				buf[i+1] = fl ? 'w' : '-';
			break;
			case 2:
				buf[i+1] = fl ? 'x' : '-';
			break;
		}
	}
	
	buf[10] = '\0';
}

void Ext2ls(char *path)
{	
	char c_path[512];
	//checkin' path in corectness. If it's empty or local then use current dir
	if(path == NULL)
		strcpy(c_path, GetCurrentDirC());
	else
	if(path[0] != '/')
	{
		strcpy(c_path, GetCurrentDirC());
		strcat(c_path, path);
	}
	else 
		strcpy(c_path, path);

	UInt32 dir_inode = GetDirInodeByName(c_path);
	if(dir_inode != 0)
	{
		char *data;
		Ext2DirEntry *direntry;
		UInt32 size, res = 0;

		ReadInodeData(dir_inode, (char *)data, size);
		direntry = (Ext2DirEntry *)data;
		
		printf("\n\n\t\tFolder: %s\n\n\n", c_path);
		printf("%10s%3s%16s%28s  %s\n", "Rights", "Lc", "Size", "Modification time", "Name");
		printf("%10s%3s%16s%28s  %s\n", "======", "==", "====", "=================", "====");
		printf("\n");
		while(direntry->name_len > 0)
		{
			char fname[255];
			Ext2Inode inode;

			memcpy(fname, direntry->name, direntry->name_len);
			fname[direntry->name_len] = 0;

			ReadInodeStruct(direntry->inode, inode);

			char *time = ctime((time_t *)&inode.i_mtime);
			if(time)
				time[strlen(time) - 1] = '\0';

			char rights[11];
			RightsToString(inode.i_mode, rights);

			printf("%10s%3hu%16u%28s  %s\n", rights, inode.i_links_count, inode.i_size, time, fname);

			if((char *)direntry - data + direntry->rec_len >= size)
				break;
			//go to the next dir entry in direntry linked list
			direntry = (Ext2DirEntry *)((char *)direntry + direntry->rec_len);
		}

		delete data;
	}
	else
	{
		printf("Error. Directory \"%s\" not found.\n", path);
		return;
	}
}

void Ext2cat(char *path)
{
	UInt32 size, len;
	char dir_path[256], c_path[512], *file_name, *data;
	//chaange path to file in corecness 
	if(path == NULL)
	{
		printf("Error. Not enough actual parametrs.\n\n", path);
		return;
	}

	strcpy(c_path, path);
	if(path[0] != '/') //if local file then using current dir
	{
		strcpy(c_path, GetCurrentDirC());
		strcat(c_path, "/");
		strcat(c_path, path);
	}

	file_name = strrchr(c_path, '/');
	if(file_name == NULL)
	{
		printf("Error. Path \"%s\" has bad format.\n", c_path);
		return;
	}
	//TODO: error handling may be changed to raise ecxeption
	
	len = file_name - c_path + 1;
	strncpy(dir_path, c_path, len);
	dir_path[len] = '\0';
	
	//skip '/' char
	++file_name;

	UInt32 dir_inode = GetDirInodeByName(dir_path);
	if(dir_inode != 0)
	{
		UInt32 file_inode = GetFileInode(dir_inode, file_name);
		if(file_inode != 0)
		{
			ReadInodeData(file_inode, (char *)data, size);
			fwrite(data, 1, size, stdout);
			delete data;
		}
		else
		{
			printf("Error. File \"%s\" not found.\n", file_name);
			return;
		}
	}
	else
	{
		printf("Error. Dir \"%s\" not found.\n", dir_path);
		return;
	}
}

void Ext2cd(char *path)
{
	if(path == NULL)
	{
		printf("Error. Not enough actual parametrs.\n\n", path);
		return;
	}
	
	std::string new_dir = GetCurrentDirS();
	if(path[0] == '/')
		new_dir = path;
	else
	{
		if(new_dir.back() != '/') new_dir += "/";
		new_dir += path;
	}

	if(GetDirInodeByName((char *)new_dir.c_str()) != 0)
		CURRENT_DIR = new_dir;
	else
		printf("Path not found.\n");

}

void main()
{
	try
	{
		//ReadFromDev(device_name, 4096*512);
		OpenDevice(device_name);
		ReadSupeblock(SuperBlock);
		ReadGroupDescriptorTable(GrpDscrTbl);

		while(1)
		{
			char buf[255];
			char *arg;

			printf("x-ha][0r $hell  %s>", GetCurrentDirC());			
			gets(buf);

			arg = strrchr(buf, ' ');
			if(arg != NULL)
			{
				arg[0] = 0;
				arg++;
			}
			if(strcmp(buf, "ls") == 0)
				Ext2ls(arg);
			else
			if(strcmp(buf, "cat") == 0)
				Ext2cat(arg);
			else
			if(strcmp(buf, "cd") == 0)
				Ext2cd(arg);
			else 
			if(strcmp(buf, "quit") == 0)
				break;
			else
				printf("Uknown command.\n");
		}
		CloseDevice();
	}catch(...)
	{
		printf("Error detected. Program is done.\n");
	}
	getchar();
}