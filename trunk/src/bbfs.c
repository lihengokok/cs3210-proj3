/*
  Big Brother File System

  The point of this FUSE filesystem is to provide an introduction to
  FUSE.  It was my first FUSE filesystem as I got to know the
  software; hopefully, the comments in this code will help people who
  follow later to get a gentler introduction.

  This might be called a no-op filesystem:  it doesn't impose
  filesystem semantics on top of any other existing structure.  It
  simply reports the requests that come in, and passes them to an
  underlying filesystem.  The information is saved in a logfile named
  bbfs.log, in the directory from which you run bbfs.

  gcc -Wall `pkg-config fuse --cflags --libs` -o bbfs bbfs.c
*/

#include "params.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/xattr.h>

#include <time.h>

#include "date_utils.h"
#include "log.h"
#include "btree.h"
#include <libexif/exif-data.h>
#include "auth.h"

struct stat* permissionsFile;
struct stat* permissionsFolder;
struct tnode *root = NULL;
static struct tnode *years = NULL;
char url[100] = "http://m.cip.gatech.edu/developer/imf/api/helper/coord/";

static void addDay(time_t time)
{
	struct tm* timeTm;
	struct tnode *yearNode, *monthNode, *dayNode;
	struct year *myYear;
	struct month *myMonth;
	struct day *myDay;
	timeTm = localtime(&time);
	myYear = malloc(sizeof(struct year));
	myYear->yearNum = timeTm->tm_year;
	myYear->months = NULL;
	myMonth = malloc(sizeof(struct month));
	myMonth->monthNum = timeTm->tm_mon;
	myMonth->days = NULL;
	myDay = malloc(sizeof(struct day));
	myDay->dayNum = timeTm->tm_mday;
	if((yearNode = tnode_search(years, myYear, yearsCompare)) == NULL)
	{
		//printf("Years\n");
		years = tnode_insert(years, myYear, yearsCompare);
		yearNode = tnode_search(years, myYear, yearsCompare);
		yearNode->data = myYear;
	} else free(myYear);
	if((monthNode = tnode_search(((struct year*)(yearNode->data))->months, myMonth, monthsCompare)) == NULL)
	{
		//printf("Months\n");
		((struct year*)(yearNode->data))->months = tnode_insert(((struct year*)(yearNode->data))->months, myMonth, monthsCompare);
		monthNode = ((struct year*)(yearNode->data))->months;
		monthNode = tnode_search(monthNode, myMonth, monthsCompare);
		monthNode->data = myMonth;
		//printf("Month node data addr: %i\n", (int)(monthNode->data));
	} else free(myMonth);
	if((dayNode = tnode_search(((struct month*)(monthNode->data))->days, myDay, daysCompare)) == NULL)
	{
		//printf("Days\n");
		((struct month*)(monthNode->data))->days = tnode_insert(((struct month*)(monthNode->data))->days, myDay, daysCompare);
		dayNode = ((struct month*)(monthNode->data))->days;
		dayNode = tnode_search(dayNode, myDay, daysCompare);
		dayNode->data = myDay;
	} else free(myDay);

	
}

//Note: we're also going to populate the time period dirs
//as we iterate over these photos
static void createNodeAndAdd(const char * path, const char * fileName, struct tnode **rootNode)
{
	struct t_snapshot* snap;
	char* fullPath;
	struct stat *statbuf;
	ExifData *ed;

	statbuf = malloc(sizeof(struct stat));
	snap = malloc(sizeof(struct t_snapshot));
	fullPath = malloc(strlen(path) + strlen(fileName) + 2);
	strcpy(fullPath, path);
	fullPath[strlen(path)] = '/';
	strcpy(fullPath + strlen(path) + 1, fileName);
	fullPath[strlen(path) + strlen(fileName) + 1] = '\0';
	snap->name = malloc(strlen(fileName) + 1);
	strcpy(snap->name, fileName);
	//snap->name[strlen(fileName)] = 
	lstat(fullPath, statbuf);
	if(S_ISDIR(statbuf->st_mode))
	{
		free(snap->name);
		free(snap);
		free(statbuf);
		return;
	}
	
	//Attempt to grab exif data
	//printf("Attempting exif read\n");
	ed = exif_data_new_from_file(fullPath);
	if (ed)
	{
		char *isGood;
		ExifEntry *entry;
		struct tm timeTm;
		int i;
		char latitude[15];
		char longitude[15];
		int flip = 0;
		
		//Date tag
		entry = exif_content_get_entry(ed->ifd[EXIF_IFD_0], EXIF_TAG_DATE_TIME);
		if(entry != NULL)
		{
			isGood = strptime(entry->data, "%Y:%m:%d %H:%M:%S", &timeTm);
			//printf("Inside exif\n");
			if (isGood != NULL)
			{
				//printf("Successful Exif read\n");
				//printf("Year: %i\n", timeTm.tm_year);
				snap->time = mktime(&timeTm);
			}
			else
			{
				snap->time = statbuf->st_mtime;
			}
		}
		
		entry = exif_content_get_entry(ed->ifd[EXIF_IFD_GPS], EXIF_TAG_GPS_LATITUDE_REF);
		if(entry != NULL)
		{
			if(strcmp("S", entry->data) == 0)
				flip = 1;
		}
		
		//Attempting geolaction read
		entry = exif_content_get_entry(ed->ifd[EXIF_IFD_GPS], EXIF_TAG_GPS_LATITUDE );
		if(entry != NULL)
		{
			//printf("Photo Name: %s\n", fileName);
			/*for(i = 0; i < 24; i++)
				printf("Lat: %i\n", *(entry->data + i));*/
			
			exifGpsDataToStr(entry->data, latitude, flip);
			//printf("Latitude: %s\n", latitude);
		}
		
		flip = 0;
		entry = exif_content_get_entry(ed->ifd[EXIF_IFD_GPS], EXIF_TAG_GPS_LONGITUDE_REF);
		if(entry != NULL)
		{
			if(strcmp("W", entry->data) == 0)
				flip = 1;
		}
		
		//Attempting geolaction read
		entry = exif_content_get_entry(ed->ifd[EXIF_IFD_GPS], EXIF_TAG_GPS_LONGITUDE );
		if(entry != NULL)
		{
			//printf("Photo Name: %s\n", fileName);
			/*for(i = 0; i < 24; i++)
				printf("Lat: %i\n", *(entry->data + i));*/
			
			exifGpsDataToStr(entry->data, longitude, flip);
			//printf("Latitude: %s\n", longitude);
			
			makeEasyPOST(latitude, longitude, fileName, url);
		}
		
	}
	else
	{
		snap->time = statbuf->st_mtime;
	}
	exif_data_unref(ed);
	
	//printf("%i\n", (int)snap->time);
	*rootNode = tnode_insert(*rootNode, snap, snapshotComp);
	addDay(snap->time);
	free(statbuf);
}

// Report errors to logfile and give -errno to caller
static int bb_error(char *str)
{
	int ret = -errno;
	
	log_msg("\tERROR %s: %s\n", str, strerror(errno));
	
	return ret;
}

// Check whether the given user is permitted to perform the given operation on the given 

//  All the paths I see are relative to the root of the mounted
//  filesystem.  In order to get to the underlying filesystem, I need to
//  have the mountpoint.  I'll save it away early on in main(), and then
//  whenever I need a path for something I'll call this to construct
//  it.
static void bb_fullpath(char fpath[PATH_MAX], const char *path)
{
	int index = findLast(path, '/');
	
	strcpy(fpath, BB_DATA->rootdir);
	strncat(fpath, path + index, PATH_MAX); // ridiculously long paths will
									// break here

	log_msg("\tbb_fullpath:  rootdir = \"%s\", path = \"%s\", fpath = \"%s\"\n",
		BB_DATA->rootdir, path, fpath);
}

///////////////////////////////////////////////////////////
//
// Prototypes for all these functions, and the C-style comments,
// come indirectly from /usr/include/fuse.h
//
/** Get file attributes.
 *
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored.  The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given.
 */
int bb_getattr(const char *path, struct stat *statbuf)
{
	int retstat = 0;
	//int index;
	char fpath[PATH_MAX];
	struct tm date;
	
	log_msg("\nbb_getattr(path=\"%s\", statbuf=0x%08x)\n",
	  path, statbuf);
	
	//Note, this is the scenario where it ends in a folder name
	//If it doesn't, we do something else sneaky
	date = pathToTmComplete(path);
	
	bb_fullpath(fpath, path);
	
	retstat = lstat(fpath, statbuf);
	if (retstat != 0)
	{
		if(isZero(&date))
		{
			retstat = bb_error("bb_getattr lstat");
		}
		else
		{
			*statbuf = *permissionsFolder;
			return 0;
		}
	}
		
	//index = findLast(path, '/');
	
	if(strlen(path + 1) > 12)
	{
		char store;
		char* isGood;
		char* isGood2;
		char* path2;
		struct tm beginTm;
		struct tm endTm;
		int dashIndex;
		path2 = path;
		dashIndex = findLast(path2 + 1, '-');
		if(dashIndex > 0)
		{
			(path2 + 1)[dashIndex] = '\0';
			
			isGood = strptime(path2 + 1, "%Y:%m:%d", &beginTm);
			log_msg("First part of path: %s\n", path2 + 1);
			isGood2 = strptime(path2 + 2 + dashIndex, "%Y:%m:%d", &endTm);
			log_msg("Second part of path: %s\n", path2 + 2 + dashIndex);
			beginTm.tm_sec = 0;
			beginTm.tm_min = 0;
			beginTm.tm_hour = 0;
			
			endTm.tm_sec = 0;
			endTm.tm_min = 0;
			endTm.tm_hour = 0;
			
			if(isGood != NULL && isGood2 != NULL)
			{
				*statbuf = *permissionsFolder;
				return 0;
			}
			(path2 + 1)[dashIndex] = '-';
		}
		
	}
	
	
	log_stat(statbuf);
	
	return retstat;
}

/** Read the target of a symbolic link
 *
 * The buffer should be filled with a null terminated string.  The
 * buffer size argument includes the space for the terminating
 * null character.  If the linkname is too long to fit in the
 * buffer, it should be truncated.  The return value should be 0
 * for success.
 */
// Note the system readlink() will truncate and lose the terminating
// null.  So, the size passed to to the system readlink() must be one
// less than the size passed to bb_readlink()
// bb_readlink() code by Bernardo F Costa (thanks!)
int bb_readlink(const char *path, char *link, size_t size)
{
	int retstat = 0;
	char fpath[PATH_MAX];
	
	log_msg("bb_readlink(path=\"%s\", link=\"%s\", size=%d)\n",
	  path, link, size);
	bb_fullpath(fpath, path);
	
	retstat = readlink(fpath, link, size - 1);
	if (retstat < 0)
	retstat = bb_error("bb_readlink readlink");
	else  {
	link[retstat] = '\0';
	retstat = 0;
	}
	
	return retstat;
}

/** Create a file node
 *
 * There is no create() operation, mknod() will be called for
 * creation of all non-directory, non-symlink nodes.
 */
// shouldn't that comment be "if" there is no.... ?
int bb_mknod(const char *path, mode_t mode, dev_t dev)
{
	int retstat = 0;
	char fpath[PATH_MAX];
	
	log_msg("\nbb_mknod(path=\"%s\", mode=0%3o, dev=%lld)\n",
	  path, mode, dev);
	bb_fullpath(fpath, path);
	
	// On Linux this could just be 'mknod(path, mode, rdev)' but this
	//  is more portable
	if (S_ISREG(mode)) {
		retstat = open(fpath, O_CREAT | O_EXCL | O_WRONLY, mode);
	if (retstat < 0)
		retstat = bb_error("bb_mknod open");
		else {
			retstat = close(retstat);
		if (retstat < 0)
		retstat = bb_error("bb_mknod close");
	}
	} else
	if (S_ISFIFO(mode)) {
		retstat = mkfifo(fpath, mode);
		if (retstat < 0)
		retstat = bb_error("bb_mknod mkfifo");
	} else {
		retstat = mknod(fpath, mode, dev);
		if (retstat < 0)
		retstat = bb_error("bb_mknod mknod");
	}
	
	return retstat;
}

/** Create a directory */
int bb_mkdir(const char *path, mode_t mode)
{
	int retstat = 0;
	char fpath[PATH_MAX];
	
	log_msg("\nbb_mkdir(path=\"%s\", mode=0%3o)\n",
		path, mode);
	bb_fullpath(fpath, path);
	
	retstat = mkdir(fpath, mode);
	if (retstat < 0)
	retstat = bb_error("bb_mkdir mkdir");
	
	return retstat;
}

/** Remove a file */
int bb_unlink(const char *path)
{
	int retstat = 0;
	char fpath[PATH_MAX];
	
	log_msg("bb_unlink(path=\"%s\")\n",
		path);
	bb_fullpath(fpath, path);
	
	retstat = unlink(fpath);
	if (retstat < 0)
	retstat = bb_error("bb_unlink unlink");
	
	return retstat;
}

/** Remove a directory */
int bb_rmdir(const char *path)
{
	int retstat = 0;
	char fpath[PATH_MAX];
	
	log_msg("bb_rmdir(path=\"%s\")\n",
		path);
	bb_fullpath(fpath, path);
	
	retstat = rmdir(fpath);
	if (retstat < 0)
	retstat = bb_error("bb_rmdir rmdir");
	
	return retstat;
}

/** Create a symbolic link */
// The parameters here are a little bit confusing, but do correspond
// to the symlink() system call.  The 'path' is where the link points,
// while the 'link' is the link itself.  So we need to leave the path
// unaltered, but insert the link into the mounted directory.
int bb_symlink(const char *path, const char *link)
{
	int retstat = 0;
	char flink[PATH_MAX];
	
	log_msg("\nbb_symlink(path=\"%s\", link=\"%s\")\n",
		path, link);
	bb_fullpath(flink, link);
	
	retstat = symlink(path, flink);
	if (retstat < 0)
	retstat = bb_error("bb_symlink symlink");
	
	return retstat;
}

/** Rename a file */
// both path and newpath are fs-relative
int bb_rename(const char *path, const char *newpath)
{
	int retstat = 0;
	char fpath[PATH_MAX];
	char fnewpath[PATH_MAX];
	
	log_msg("\nbb_rename(fpath=\"%s\", newpath=\"%s\")\n",
		path, newpath);
	bb_fullpath(fpath, path);
	bb_fullpath(fnewpath, newpath);
	
	retstat = rename(fpath, fnewpath);
	if (retstat < 0)
	retstat = bb_error("bb_rename rename");
	
	return retstat;
}

/** Create a hard link to a file */
int bb_link(const char *path, const char *newpath)
{
	int retstat = 0;
	char fpath[PATH_MAX], fnewpath[PATH_MAX];
	
	log_msg("\nbb_link(path=\"%s\", newpath=\"%s\")\n",
		path, newpath);
	bb_fullpath(fpath, path);
	bb_fullpath(fnewpath, newpath);
	
	retstat = link(fpath, fnewpath);
	if (retstat < 0)
	retstat = bb_error("bb_link link");
	
	return retstat;
}

/** Change the permission bits of a file */
int bb_chmod(const char *path, mode_t mode)
{
	int retstat = 0;
	char fpath[PATH_MAX];
	
	log_msg("\nbb_chmod(fpath=\"%s\", mode=0%03o)\n",
		path, mode);
	bb_fullpath(fpath, path);
	
	retstat = chmod(fpath, mode);
	if (retstat < 0)
	retstat = bb_error("bb_chmod chmod");
	
	return retstat;
}

/** Change the owner and group of a file */
int bb_chown(const char *path, uid_t uid, gid_t gid)
  
{
	int retstat = 0;
	char fpath[PATH_MAX];
	
	log_msg("\nbb_chown(path=\"%s\", uid=%d, gid=%d)\n",
		path, uid, gid);
	bb_fullpath(fpath, path);
	
	retstat = chown(fpath, uid, gid);
	if (retstat < 0)
	retstat = bb_error("bb_chown chown");
	
	return retstat;
}

/** Change the size of a file */
int bb_truncate(const char *path, off_t newsize)
{
	int retstat = 0;
	char fpath[PATH_MAX];
	
	log_msg("\nbb_truncate(path=\"%s\", newsize=%lld)\n",
		path, newsize);
	bb_fullpath(fpath, path);
	
	retstat = truncate(fpath, newsize);
	if (retstat < 0)
	bb_error("bb_truncate truncate");
	
	return retstat;
}

/** Change the access and/or modification times of a file */
/* note -- I'll want to change this as soon as 2.6 is in debian testing */
int bb_utime(const char *path, struct utimbuf *ubuf)
{
	int retstat = 0;
	char fpath[PATH_MAX];
	
	log_msg("\nbb_utime(path=\"%s\", ubuf=0x%08x)\n",
		path, ubuf);
	bb_fullpath(fpath, path);
	
	retstat = utime(fpath, ubuf);
	if (retstat < 0)
	retstat = bb_error("bb_utime utime");
	
	return retstat;
}

/** File open operation
 *
 * No creation, or truncation flags (O_CREAT, O_EXCL, O_TRUNC)
 * will be passed to open().  Open should check if the operation
 * is permitted for the given flags.  Optionally open may also
 * return an arbitrary filehandle in the fuse_file_info structure,
 * which will be passed to all file operations.
 *
 * Changed in version 2.2
 */
int bb_open(const char *path, struct fuse_file_info *fi)
{
	int retstat = 0;
	int fd;
	char fpath[PATH_MAX];
	
	log_msg("\nbb_open(path\"%s\", fi=0x%08x)\n",
		path, fi);
	bb_fullpath(fpath, path);
	
	fd = open(fpath, fi->flags);
	if (fd < 0)
	retstat = bb_error("bb_open open");
	
	fi->fh = fd;
	log_fi(fi);
	
	return retstat;
}

/** Read data from an open file
 *
 * Read should return exactly the number of bytes requested except
 * on EOF or error, otherwise the rest of the data will be
 * substituted with zeroes.  An exception to this is when the
 * 'direct_io' mount option is specified, in which case the return
 * value of the read system call will reflect the return value of
 * this operation.
 *
 * Changed in version 2.2
 */
// I don't fully understand the documentation above -- it doesn't
// match the documentation for the read() system call which says it
// can return with anything up to the amount of data requested. nor
// with the fusexmp code which returns the amount of data also
// returned by read.
int bb_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	int retstat = 0;
	
	log_msg("\nbb_read(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n",
		path, buf, size, offset, fi);
	// no need to get fpath on this one, since I work from fi->fh not the path
	log_fi(fi);
	
	retstat = pread(fi->fh, buf, size, offset);
	if (retstat < 0)
	retstat = bb_error("bb_read read");
	
	return retstat;
}

/** Write data to an open file
 *
 * Write should return exactly the number of bytes requested
 * except on error.  An exception to this is when the 'direct_io'
 * mount option is specified (see read operation).
 *
 * Changed in version 2.2
 */
// As  with read(), the documentation above is inconsistent with the
// documentation for the write() system call.
int bb_write(const char *path, const char *buf, size_t size, off_t offset,
		 struct fuse_file_info *fi)
{
	int retstat = 0;
	
	log_msg("\nbb_write(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n",
		path, buf, size, offset, fi);
	// no need to get fpath on this one, since I work from fi->fh not the path
	log_fi(fi);
	
	retstat = pwrite(fi->fh, buf, size, offset);
	if (retstat < 0)
		retstat = bb_error("bb_write pwrite");
		
	
	//if (tnode_search(root, ) == NULL)
	//{
	
	//}
	
	return retstat;
}

/** Get file system statistics
 *
 * The 'f_frsize', 'f_favail', 'f_fsid' and 'f_flag' fields are ignored
 *
 * Replaced 'struct statfs' parameter with 'struct statvfs' in
 * version 2.5
 */
int bb_statfs(const char *path, struct statvfs *statv)
{
	int retstat = 0;
	char fpath[PATH_MAX];
	
	log_msg("\nbb_statfs(path=\"%s\", statv=0x%08x)\n",
		path, statv);
	bb_fullpath(fpath, path);
	
	// get stats for underlying filesystem
	retstat = statvfs(fpath, statv);
	if (retstat < 0)
	retstat = bb_error("bb_statfs statvfs");
	
	log_statvfs(statv);
	
	return retstat;
}

/** Possibly flush cached data
 *
 * BIG NOTE: This is not equivalent to fsync().  It's not a
 * request to sync dirty data.
 *
 * Flush is called on each close() of a file descriptor.  So if a
 * filesystem wants to return write errors in close() and the file
 * has cached dirty data, this is a good place to write back data
 * and return any errors.  Since many applications ignore close()
 * errors this is not always useful.
 *
 * NOTE: The flush() method may be called more than once for each
 * open().  This happens if more than one file descriptor refers
 * to an opened file due to dup(), dup2() or fork() calls.  It is
 * not possible to determine if a flush is final, so each flush
 * should be treated equally.  Multiple write-flush sequences are
 * relatively rare, so this shouldn't be a problem.
 *
 * Filesystems shouldn't assume that flush will always be called
 * after some writes, or that if will be called at all.
 *
 * Changed in version 2.2
 */
int bb_flush(const char *path, struct fuse_file_info *fi)
{
	int retstat = 0;
	
	log_msg("\nbb_flush(path=\"%s\", fi=0x%08x)\n", path, fi);
	// no need to get fpath on this one, since I work from fi->fh not the path
	log_fi(fi);
	
	return retstat;
}

/** Release an open file
 *
 * Release is called when there are no more references to an open
 * file: all file descriptors are closed and all memory mappings
 * are unmapped.
 *
 * For every open() call there will be exactly one release() call
 * with the same flags and file descriptor.  It is possible to
 * have a file opened more than once, in which case only the last
 * release will mean, that no more reads/writes will happen on the
 * file.  The return value of release is ignored.
 *
 * Changed in version 2.2
 */
int bb_release(const char *path, struct fuse_file_info *fi)
{
	int retstat = 0;
	
	log_msg("\nbb_release(path=\"%s\", fi=0x%08x)\n",
	  path, fi);
	log_fi(fi);

	// We need to close the file.  Had we allocated any resources
	// (buffers etc) we'd need to free them here as well.
	retstat = close(fi->fh);
	
	return retstat;
}

/** Synchronize file contents
 *
 * If the datasync parameter is non-zero, then only the user data
 * should be flushed, not the meta data.
 *
 * Changed in version 2.2
 */
int bb_fsync(const char *path, int datasync, struct fuse_file_info *fi)
{
	int retstat = 0;
	
	log_msg("\nbb_fsync(path=\"%s\", datasync=%d, fi=0x%08x)\n",
		path, datasync, fi);
	log_fi(fi);
	
	if (datasync)
	retstat = fdatasync(fi->fh);
	else
	retstat = fsync(fi->fh);
	
	if (retstat < 0)
	bb_error("bb_fsync fsync");
	
	return retstat;
}

/** Set extended attributes */
int bb_setxattr(const char *path, const char *name, const char *value, size_t size, int flags)
{
	int retstat = 0;
	char fpath[PATH_MAX];
	
	log_msg("\nbb_setxattr(path=\"%s\", name=\"%s\", value=\"%s\", size=%d, flags=0x%08x)\n",
		path, name, value, size, flags);
	bb_fullpath(fpath, path);
	
	retstat = lsetxattr(fpath, name, value, size, flags);
	if (retstat < 0)
	retstat = bb_error("bb_setxattr lsetxattr");
	
	return retstat;
}

/** Get extended attributes */
int bb_getxattr(const char *path, const char *name, char *value, size_t size)
{
	int retstat = 0;
	char fpath[PATH_MAX];
	
	log_msg("\nbb_getxattr(path = \"%s\", name = \"%s\", value = 0x%08x, size = %d)\n",
		path, name, value, size);
	bb_fullpath(fpath, path);
	
	retstat = lgetxattr(fpath, name, value, size);
	if (retstat < 0)
	retstat = bb_error("bb_getxattr lgetxattr");
	else
	log_msg("	value = \"%s\"\n", value);
	
	return retstat;
}

/** List extended attributes */
int bb_listxattr(const char *path, char *list, size_t size)
{
	int retstat = 0;
	char fpath[PATH_MAX];
	char *ptr;
	
	log_msg("bb_listxattr(path=\"%s\", list=0x%08x, size=%d)\n",
		path, list, size
		);
	bb_fullpath(fpath, path);
	
	retstat = llistxattr(fpath, list, size);
	if (retstat < 0)
	retstat = bb_error("bb_listxattr llistxattr");
	
	log_msg("	returned attributes (length %d):\n", retstat);
	for (ptr = list; ptr < list + retstat; ptr += strlen(ptr)+1)
	log_msg("	\"%s\"\n", ptr);
	
	return retstat;
}

/** Remove extended attributes */
int bb_removexattr(const char *path, const char *name)
{
	int retstat = 0;
	char fpath[PATH_MAX];
	
	log_msg("\nbb_removexattr(path=\"%s\", name=\"%s\")\n",
		path, name);
	bb_fullpath(fpath, path);
	
	retstat = lremovexattr(fpath, name);
	if (retstat < 0)
	retstat = bb_error("bb_removexattr lrmovexattr");
	
	return retstat;
}

/** Open directory
 *
 * This method should check if the open operation is permitted for
 * this  directory
 *
 * Introduced in version 2.3
 */
 
int bb_opendir(const char *path, struct fuse_file_info *fi)
{
	DIR *dp;
	int retstat = 0;
	char fpath[PATH_MAX];
	struct tm date;
	if(strcmp("/", path) == 0)
		return 0;
	if(strlen(path + 1) > 12)
	{
		char store;
		char* isGood;
		char* isGood2;
		char* path2;
		struct tm beginTm;
		struct tm endTm;
		int dashIndex;
		path2 = path;
		dashIndex = findLast(path2 + 1, '-');
		if(dashIndex > 0)
		{
			(path2 + 1)[dashIndex] = '\0';
			
			isGood = strptime(path2 + 1, "%Y:%m:%d", &beginTm);
			log_msg("First part of path: %s\n", path2 + 1);
			isGood2 = strptime(path2 + 2 + dashIndex, "%Y:%m:%d", &endTm);
			beginTm.tm_sec = 0;
			beginTm.tm_min = 0;
			beginTm.tm_hour = 0;
			
			endTm.tm_sec = 0;
			endTm.tm_min = 0;
			endTm.tm_hour = 0;
			if(isGood != NULL && isGood2 != NULL)
			{
				log_msg("Returning existence\n");
				return 0;
			}
			(path2 + 1)[dashIndex] = '-';
		}
		
	}
	//Note, this is the scenario where it ends in a folder name
	//If it doesn't, in this case, it fails
	date = pathToTmComplete(path);
	if(isZero(&date))
	{
		//retstat = bb_error("bb_getattr lstat");
		//return retstat;
	}
	else
	{
		return 0;
	}
	
	log_msg("\nbb_opendir(path=\"%s\", fi=0x%08x)\n", path, fi);
	bb_fullpath(fpath, path);
	
	dp = opendir(fpath);
	if (dp == NULL)
		retstat = bb_error("bb_opendir opendir");
	
	fi->fh = (intptr_t) dp;
	
	log_fi(fi);
	
	return retstat;
}


/** Read directory
 *
 * This supersedes the old getdir() interface.  New applications
 * should use this.
 *
 * The filesystem may choose between two modes of operation:
 *
 * 1) The readdir implementation ignores the offset parameter, and
 * passes zero to the filler function's offset.  The filler
 * function will not return '1' (unless an error happens), so the
 * whole directory is read in a single readdir operation.  This
 * works just like the old getdir() method.
 *
 * 2) The readdir implementation keeps track of the offsets of the
 * directory entries.  It uses the offset parameter and always
 * passes non-zero offset to the filler function.  When the buffer
 * is full (or an error happens) the filler function will return
 * '1'.
 *
 * Introduced in version 2.3
 */
int bb_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
		   struct fuse_file_info *fi)
{
	int retstat = 0;
	DIR *dp;
	struct dirent *de;
	struct tm date;
	  
	if(strcmp("/", path) == 0)
	{
		fillYears(years, buf, filler);
		fillBuffer(1, 2000000000, root, buf, filler);
		return 0;
	}
	
	if(strlen(path + 1) > 12)
	{
		char store;
		char* isGood;
		char* isGood2;
		char* path2;
		struct tm beginTm;
		struct tm endTm;
		int dashIndex;
		path2 = path;
		dashIndex = findLast(path2 + 1, '-');
		if(dashIndex > 0)
		{
			(path2 + 1)[dashIndex] = '\0';
			
			isGood = strptime(path2 + 1, "%Y:%m:%d", &beginTm);

			isGood2 = strptime(path2 + 2 + dashIndex, "%Y:%m:%d", &endTm);
			
			log_msg("First part of path: %s\n", path2 + 1);
			log_msg("seconds after the minute (0 to 61): %d \n",endTm.tm_sec);
			log_msg("minutes after the hour (0 to 59): %d \n",endTm.tm_min);
			log_msg("hours since midnight (0 to 23): %d \n",endTm.tm_hour);
			log_msg("day of the month (1 to 31): %d \n",endTm.tm_mday);
			log_msg("months since January (0 to 11): %d \n",endTm.tm_mon);
			log_msg("years since 1900: %d \n",endTm.tm_year);
			log_msg("days since Sunday (0 to 6 Sunday=0): %d \n",endTm.tm_wday);
			log_msg("days since January 1 (0 to 365): %d \n",endTm.tm_yday);
			log_msg("Daylight Savings Time: %d \n",endTm.tm_isdst);
			
			log_msg("Second part of path: %s\n", path2 + 2 + dashIndex);
			beginTm.tm_sec = 0;
			beginTm.tm_min = 0;
			beginTm.tm_hour = 0;
			
			endTm.tm_sec = 0;
			endTm.tm_min = 0;
			endTm.tm_hour = 0;
			if(isGood != NULL && isGood2 != NULL)
			{
				time_t t1, t2;
				t1 = mktime(&beginTm);
				t2 = mktime(&endTm);
				log_msg("Performed search from %i to %i\n", t1, t2);
				fillBuffer(t1, t2, root, buf, filler);
				(path2 + 1)[dashIndex] = '-';
				return 0;
			}
			(path2 + 1)[dashIndex] = '-';
		}
		
	}
	
	
	//Note, this is the scenario where it ends in a folder name
	//If it doesn't, in this case, it fails
	date = pathToTmComplete(path);
	if(isZero(&date))
		retstat = bb_error("BB dir doesn't exist");
	else
	{
		time_t start, end;
		filler(buf, ".", NULL, 0);
		filler(buf, "..", NULL, 0);
		if(date.tm_mon == -1) //in this case, only a year was specified
		{
			struct year yearData;
			struct tnode* yearNode;
			yearData.yearNum = date.tm_year;
			yearNode = tnode_search(years, &yearData, yearsCompare);
			if(yearNode != NULL)
			{
				fillMonths(((struct year*)(yearNode->data))->months, buf, filler);
			}
			
			/*filler(buf, "jan", NULL, 0);
			filler(buf, "feb", NULL, 0);
			filler(buf, "mar", NULL, 0);
			filler(buf, "apr", NULL, 0);
			filler(buf, "may", NULL, 0);
			filler(buf, "jun", NULL, 0);
			filler(buf, "jul", NULL, 0);
			filler(buf, "aug", NULL, 0);
			filler(buf, "sep", NULL, 0);
			filler(buf, "oct", NULL, 0);
			filler(buf, "nov", NULL, 0);
			filler(buf, "dec", NULL, 0);*/
			date.tm_mon = 0;
			date.tm_mday = 1;
			date.tm_hour = 0;
			date.tm_min = 0;
			date.tm_sec = 0;
			start = mktime(&date);
			date.tm_year++;
			end = mktime(&date);
			log_msg("\nStart Time: %i\n", start);
			log_msg("\nEnd Time: %i\n", end);
			
			fillBuffer(start, end, root, buf, filler);
		}
		else if(date.tm_mday == -1)
		{
			struct year yearData;
			struct month monthData;
			struct tnode* yearNode;
			struct tnode* monthNode;
			//int i;
			//char asString[20];
			//int daysCount = daysInMonth(date);
			yearData.yearNum = date.tm_year;
			monthData.monthNum = date.tm_mon;
			yearNode = tnode_search(years, &yearData, yearsCompare);
			if(yearNode != NULL)
			{
				monthNode = tnode_search(((struct year*)(yearNode->data))->months, &monthData, monthsCompare);
				if(monthNode != NULL)
					fillDays(((struct month*)(monthNode->data))->days, buf, filler);
			}
			date.tm_mday = 1;
			date.tm_hour = 0;
			date.tm_min = 0;
			date.tm_sec = 0;
			start = mktime(&date);
			date.tm_mon++;
			end = mktime(&date);
			log_msg("\nStart Time: %i\n", start);
			log_msg("\nEnd Time: %i\n", end);
			
			fillBuffer(start, end, root, buf, filler);
			
		}
		else
		{
			date.tm_hour = 0;
			date.tm_min = 0;
			date.tm_sec = 0;
			start = mktime(&date);
			date.tm_mday++;
			end = mktime(&date);
			log_msg("\nStart Time: %i\n", start);
			log_msg("\nEnd Time: %i\n", end);
			
			fillBuffer(start, end, root, buf, filler);
		}
		return 0;
	}
	
	log_msg("\nbb_readdir(path=\"%s\", buf=0x%08x, filler=0x%08x, offset=%lld, fi=0x%08x)\n",
		path, buf, filler, offset, fi);

		
	// once again, no need for fullpath -- but note that I need to cast fi->fh
	dp = (DIR *) (uintptr_t) fi->fh;

	// Every directory contains at least two entries: . and ..  If my
	// first call to the system readdir() returns NULL I've got an
	// error; near as I can tell, that's the only condition under
	// which I can get an error from readdir()
	de = readdir(dp);
	if (de == 0) {
		retstat = bb_error("bb_readdir readdir");
	return retstat;
	}

	// This will copy the entire directory into the buffer.  The loop exits
	// when either the system readdir() returns NULL, or filler()
	// returns something non-zero.  The first case just means I've
	// read the whole directory; the second means the buffer is full.
	do {
	log_msg("calling filler with name %s\n", de->d_name);
	if (filler(buf, de->d_name, NULL, 0) != 0) {
		log_msg("\tERROR bb_readdir filler:  buffer full");
		return -ENOMEM;
	}
	} while ((de = readdir(dp)) != NULL);
	
	log_fi(fi);
	
	return retstat;
}

/** Release directory
 *
 * Introduced in version 2.3
 */
int bb_releasedir(const char *path, struct fuse_file_info *fi)
{
	int retstat = 0;
	
	log_msg("\nbb_releasedir(path=\"%s\", fi=0x%08x)\n",
		path, fi);
	log_fi(fi);
	
	closedir((DIR *) (uintptr_t) fi->fh);
	
	return retstat;
}

/** Synchronize directory contents
 *
 * If the datasync parameter is non-zero, then only the user data
 * should be flushed, not the meta data
 *
 * Introduced in version 2.3
 */
// when exactly is this called?  when a user calls fsync and it
// happens to be a directory? ???
int bb_fsyncdir(const char *path, int datasync, struct fuse_file_info *fi)
{
	int retstat = 0;
	
	log_msg("\nbb_fsyncdir(path=\"%s\", datasync=%d, fi=0x%08x)\n",
		path, datasync, fi);
	log_fi(fi);
	
	return retstat;
}

/**
 * Initialize filesystem
 *
 * The return value will passed in the private_data field of
 * fuse_context to all file operations and as a parameter to the
 * destroy() method.
 *
 * Introduced in version 2.3
 * Changed in version 2.6
 */
// Undocumented but extraordinarily useful fact:  the fuse_context is
// set up before this function is called, and
// fuse_get_context()->private_data returns the user_data passed to
// fuse_main().  Really seems like either it should be a third
// parameter coming in here, or else the fact should be documented
// (and this might as well return void, as it did in older versions of
// FUSE).
void *bb_init(struct fuse_conn_info *conn)
{
	
	log_msg("\nbb_init()\n");
	
	return BB_DATA;
}

/**
 * Clean up filesystem
 *
 * Called on filesystem exit.
 *
 * Introduced in version 2.3
 */
void bb_destroy(void *userdata)
{
	log_msg("\nbb_destroy(userdata=0x%08x)\n", userdata);
}

/**
 * Check file access permissions
 *
 * This will be called for the access() system call.  If the
 * 'default_permissions' mount option is given, this method is not
 * called.
 *
 * This method is not called under Linux kernel versions 2.4.x
 *
 * Introduced in version 2.5
 */
int bb_access(const char *path, int mask)
{
	int retstat = 0;
	char fpath[PATH_MAX];
	
	log_msg("\nbb_access(path=\"%s\", mask=0%o)\n",
		path, mask);
	bb_fullpath(fpath, path);
	
	retstat = access(fpath, mask);
	
	if (retstat < 0)
		retstat = bb_error("bb_access access");
	
	return retstat;
}

/**
 * Create and open a file
 *
 * If the file does not exist, first create it with the specified
 * mode, and then open it.
 *
 * If this method is not implemented or under Linux kernel
 * versions earlier than 2.6.15, the mknod() and open() methods
 * will be called instead.
 *
 * Introduced in version 2.5
 */
int bb_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	int retstat = 0;
	char fpath[PATH_MAX];
	int fd;
	
	log_msg("\nbb_create(path=\"%s\", mode=0%03o, fi=0x%08x)\n",
		path, mode, fi);
	bb_fullpath(fpath, path);
	
	fd = creat(fpath, mode);
	if (fd < 0)
		retstat = bb_error("bb_create creat");
	
	
	fi->fh = fd;
	
	log_fi(fi);
	
	createNodeAndAdd(BB_DATA->rootdir, path + 1, &root);
	
	return retstat;
}

/**
 * Change the size of an open file
 *
 * This method is called instead of the truncate() method if the
 * truncation was invoked from an ftruncate() system call.
 *
 * If this method is not implemented or under Linux kernel
 * versions earlier than 2.6.15, the truncate() method will be
 * called instead.
 *
 * Introduced in version 2.5
 */
int bb_ftruncate(const char *path, off_t offset, struct fuse_file_info *fi)
{
	int retstat = 0;
	
	log_msg("\nbb_ftruncate(path=\"%s\", offset=%lld, fi=0x%08x)\n",
		path, offset, fi);
	log_fi(fi);
	
	retstat = ftruncate(fi->fh, offset);
	if (retstat < 0)
	retstat = bb_error("bb_ftruncate ftruncate");
	
	return retstat;
}

/**
 * Get attributes from an open file
 *
 * This method is called instead of the getattr() method if the
 * file information is available.
 *
 * Currently this is only called after the create() method if that
 * is implemented (see above).  Later it may be called for
 * invocations of fstat() too.
 *
 * Introduced in version 2.5
 */
// Since it's currently only called after bb_create(), and bb_create()
// opens the file, I ought to be able to just use the fd and ignore
// the path...
int bb_fgetattr(const char *path, struct stat *statbuf, struct fuse_file_info *fi)
{
	int retstat = 0;
	
	log_msg("\nbb_fgetattr(path=\"%s\", statbuf=0x%08x, fi=0x%08x)\n",
		path, statbuf, fi);
	log_fi(fi);
	
	retstat = fstat(fi->fh, statbuf);
	if (retstat < 0)
	retstat = bb_error("bb_fgetattr fstat");
	
	log_stat(statbuf);
	
	return retstat;
}

struct fuse_operations bb_oper = {
  .getattr = bb_getattr,
  .readlink = bb_readlink,
  // no .getdir -- that's deprecated
  .getdir = NULL,
  .mknod = bb_mknod,
  .mkdir = bb_mkdir,
  .unlink = bb_unlink,
  .rmdir = bb_rmdir,
  .symlink = bb_symlink,
  .rename = bb_rename,
  .link = bb_link,
  .chmod = bb_chmod,
  .chown = bb_chown,
  .truncate = bb_truncate,
  .utime = bb_utime,
  .open = bb_open,
  .read = bb_read,
  .write = bb_write,
  /** Just a placeholder, don't set */ // huh???
  .statfs = bb_statfs,
  .flush = bb_flush,
  .release = bb_release,
  .fsync = bb_fsync,
  .setxattr = bb_setxattr,
  .getxattr = bb_getxattr,
  .listxattr = bb_listxattr,
  .removexattr = bb_removexattr,
  .opendir = bb_opendir,
  .readdir = bb_readdir,
  .releasedir = bb_releasedir,
  .fsyncdir = bb_fsyncdir,
  .init = bb_init,
  .destroy = bb_destroy,
  .access = bb_access,
  .create = bb_create,
  .ftruncate = bb_ftruncate,
  .fgetattr = bb_fgetattr
};

void bb_usage()
{
	fprintf(stderr, "usage:  bbfs rootDir mountPoint\n");
	abort();
}

int main(int argc, char *argv[])
{
	int i;
	int fuse_stat;
	struct bb_state *bb_data;
	DIR* mydir;
	struct dirent *entry;

	root = NULL;

	// bbfs doesn't do any access checking on its own (the comment
	// blocks in fuse.h mention some of the functions that need
	// accesses checked -- but note there are other functions, like
	// chown(), that also need checking!).  Since running bbfs as root
	// will therefore open Metrodome-sized holes in the system
	// security, we'll check if root is trying to mount the filesystem
	// and refuse if it is.  The somewhat smaller hole of an ordinary
	// user doing it with the allow_other flag is still there because
	// I don't want to parse the options string.
	if ((getuid() == 0) || (geteuid() == 0)) {
	fprintf(stderr, "Running BBFS as root opens unnacceptable security holes\n");
	return 1;
	}

	bb_data = calloc(sizeof(struct bb_state), 1);
	if (bb_data == NULL) {
		perror("main calloc");
		abort();
	}
	
	bb_data->logfile = log_open();
	
	// libfuse is able to do most of the command line parsing; all I
	// need to do is to extract the rootdir; this will be the first
	// non-option passed in.  I'm using the GNU non-standard extension
	// and having realpath malloc the space for the path
	// the string.
	for (i = 1; (i < argc) && (argv[i][0] == '-'); i++)
	if (argv[i][1] == 'o') i++; // -o takes a parameter; need to
					// skip it too.  This doesn't
					// handle "squashed" parameters
	
	if ((argc - i) != 2) bb_usage();
	
	bb_data->rootdir = realpath(argv[i], NULL);
	
	
	permissionsFile = malloc(sizeof(struct stat));
	permissionsFolder = malloc(sizeof(struct stat));
	lstat(bb_data->rootdir, permissionsFolder);
	*permissionsFile = (*permissionsFolder);
	permissionsFile->st_ino = 0;
    permissionsFile->st_mode = 0100664;
	permissionsFile->st_nlink = 1;
	permissionsFolder->st_ino = 0;
    permissionsFolder->st_mode = 040444;
	permissionsFolder->st_nlink = 2; // fix this
	
	//set up the tree
	mydir = opendir(bb_data->rootdir);
    entry = NULL;
    while((entry = readdir(mydir))) /* If we get EOF, the expression is 0 and
                                     * the loop stops. */
    {
		createNodeAndAdd(bb_data->rootdir, entry->d_name, &root);
        //printf("%s\n", entry->d_name);
    }
    
    //print out the year 2011
    //printf("Years data addr: %i\n", (int)(((struct year*)(years->data))->months));
    //logMonths(((struct year*)(years->data))->months);
    
    
	printf("\n");
    //print all the nodes out to see the tree
    //print_inorder(root, snapshotPrint);
    
    closedir(mydir);

	argv[i] = argv[i+1];
	argc--;

	fprintf(stderr, "about to call fuse_main\n");
	fuse_stat = fuse_main(argc, argv, &bb_oper, bb_data);
	fprintf(stderr, "fuse_main returned %d\n", fuse_stat);
	
	return fuse_stat;
}