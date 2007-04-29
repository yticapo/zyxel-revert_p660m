#ifndef _FILEDATA_H_
#define _FILEDATA_H_

struct filedata {
	int size;
	void *data[0];
};

struct filedata * get_filedata(const char *filename);
struct filedata * alloc_filedata(int size);
int put_filedata(const char *filename, struct filedata *filedata);

#endif /* _FILEDATA_H_ */
