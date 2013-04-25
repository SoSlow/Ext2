#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <vector>
#include <locale>
#include <windows.h>
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

void mb2ascii(char *s)
{
	std::locale russian("Russian");
	setlocale(LC_ALL, "Russian");
	switch(s[0])
    {
        case ' ': case '!': case '"': case '#': case '%':
        case '&': case '\'': case '(': case ')': case '*':
        case '+': case ',': case '-': case '.': case '/':
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
        case ':': case ';': case '<': case '=': case '>':
        case '?':
        case 'A': case 'B': case 'C': case 'D': case 'E':
        case 'F': case 'G': case 'H': case 'I': case 'J':
        case 'K': case 'L': case 'M': case 'N': case 'O':
        case 'P': case 'Q': case 'R': case 'S': case 'T':
        case 'U': case 'V': case 'W': case 'X': case 'Y':
        case 'Z':
        case '[': case '\\': case ']': case '^': case '_':
        case 'a': case 'b': case 'c': case 'd': case 'e':
        case 'f': case 'g': case 'h': case 'i': case 'j':
        case 'k': case 'l': case 'm': case 'n': case 'o':
        case 'p': case 'q': case 'r': case 's': case 't':
        case 'u': case 'v': case 'w': case 'x': case 'y':
        case 'z': case '{': case '|': case '}': case '~':
			return;
			break;
		default:
		{
			int len = strlen(s);

			wchar_t *tmp = new wchar_t[len];

			MultiByteToWideChar(CP_UTF8, 0, s, len+1, tmp, len+1);
			
			//int r = mbstowcs(tmp, s, strlen(s) + 1);
		
			std::wstring source(tmp);
			
			std::string result(source.begin(), source.end());
			//wprintf(L"%s", tmp);//(wchar_t *)s, strlen(s)/2)
			
			strcpy(s, result.c_str());
		}
	}
}


void treeFunc(UInt32 dir_inode, int deep, const char *parent_name)
{
	const char *hor_line = "|---/";
	const char *spc_line = "|   ";
	char *data;
	char dir_name[255];

	Ext2DirEntry *direntry;
	UInt32 size, res = 0;

	if(deep != 0)
	{
		for(int i = 0; i < deep-1; ++i)
			printf("%s", spc_line);
		//printf("%*s", deep*strlen(hor_line), ;
		printf("%s%s\n", hor_line, parent_name);
	}

	ReadInodeData(dir_inode, (char *)data, size);
	direntry = (Ext2DirEntry *)data;

	while(direntry->name_len > 0)
	{
		if(direntry->file_type == EXT2_FT_DIR)
		{
			
			memcpy(dir_name, direntry->name, direntry->name_len);
			dir_name[direntry->name_len] = 0;
			if(strcmp(".", dir_name) != 0 && strcmp("..", dir_name) != 0)
			{
				//unicode2ascii(dir_name);
				treeFunc(direntry->inode, deep + 1, dir_name);
			}
		}
		if((char *)direntry - data + direntry->rec_len >= size)
			break;
		//go to the next dir entry in direntry linked list		
		direntry = (Ext2DirEntry *)((char *)direntry + direntry->rec_len);
	}
	//free mem
	delete data;
}

void Ext2tree()
{
	char *c_dir = (char *)GetCurrentDirC();
	printf("Directory tree \n%s\n", c_dir);
	treeFunc(GetDirInodeByName(c_dir), 0, "");
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

			arg = strchr(buf, ' ');
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
			if(strcmp(buf, "tree") == 0)
				Ext2tree();
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