#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "filedata.h"

struct filedata * get_filedata(const char *filename)
{
	int fd = open(filename, O_RDONLY);
	if (fd < 0) {
		perror("get_filedata(): open()");
		return NULL;
	}

	struct stat filestat;
	if (fstat(fd, &filestat) < 0) {
		perror("get_filedata(): fstat()");
		close(fd);
		return NULL;
	}

	struct filedata *filedata = malloc(sizeof(struct filedata) + filestat.st_size);
	if (filedata == NULL) {
		perror("get_filedata(): malloc()");
		close(fd);
		return NULL;
	}

	filedata->size = filestat.st_size;

	int readsize = read(fd, filedata->data, filedata->size);
	if (readsize != filedata->size) {
		perror("get_filedata(): read()");
		free(filedata);
		close(fd);
		return NULL;
	}

	close(fd);
	return filedata;
}

struct filedata * alloc_filedata(int size)
{
	struct filedata *retval = malloc(sizeof(struct filedata) + size);
	if (retval == NULL) {
		perror("alloc_filedata(): malloc()");
		return NULL;
	}
	retval->size = size;
	return retval;
}

int put_filedata(const char *filename, struct filedata *filedata)
{
	int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0644);
	if (fd < 0) {
		perror("put_filedata(): open()");
		return -1;
	}

	int writesize = write(fd, filedata->data, filedata->size);
	if (writesize != filedata->size) {
		perror("put_filedata(): write()");
		close(fd);
		return -1;
	}

	close(fd);
	return 0;
}
