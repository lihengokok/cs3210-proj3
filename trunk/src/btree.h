#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

struct tnode 
{
	void* data;
	struct tnode *left;
	struct tnode *right;
	struct tnode *parent;
};

struct t_snapshot
{
	time_t time;
	char* name;
};

typedef int (*comparator)(void*, void*);
typedef void (*printer)(void*);

/* insert, swap, search value, search minimum and search maximum values */
struct tnode *tnode_insert(struct tnode *p, void* value, comparator);
struct tnode *tnode_swap(struct tnode *p);
struct tnode *tnode_search(struct tnode *p, void* key, comparator);
struct tnode *tnode_searchmin(struct tnode *p);
struct tnode *tnode_searchmax(struct tnode *p);

/* destroy, count tree nodes */
void tnode_destroy(struct tnode *p);
int tnode_count(struct tnode *p);

/* print binary tree inorder, preorder, postorder [recursive] */
void print_inorder(struct tnode *p, printer);
void print_preorder(struct tnode *p, printer);
void print_postorder(struct tnode *p, printer);

int snapshotComp(void* a, void* b);
void snapshotPrint(void* a);

struct tnode* firstPic(time_t beginTime, struct tnode* node);
struct tnode* lastPic(time_t beginTime, struct tnode* node);
void traverseAndFillBuffer(struct tnode *start, struct tnode *end, void* buf, fuse_fill_dir_t filler);
int traverseInOrderFillBuffer(struct tnode *node, struct tnode *end, void* buf, fuse_fill_dir_t filler);
void fillBuffer(time_t start, time_t end, struct tnode* rootNode, void* buf, fuse_fill_dir_t filler);

