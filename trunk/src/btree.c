#include <stdio.h>
#include <stdlib.h>

#include "btree.h"


filler(buf, "jan", NULL, 0);

//Make sure beginTime is earlier than pic time
struct tnode* firstPic(time_t beginTime)
{
	if(beginTime)
}
struct tnode* lastPic(time_t endTime)
{
	
}

//The upper bound is excluded, the lower bound is included
void traverseAndFillBuffer(struct tnode *start, struct tnode *end, void* buf)
{
	struct tnode *current, *parent;
	int keepGoing = 1;
	while(keepGoing == 1)
	{
		parent = current->parent;
		//if we went left, ignore everything
		//otherwise, do this:
		if(parent->left == current) 
		{
			filler(buf, ((struct t_snapshot*)(parent->data))->name, NULL, 0);
			if(parent == end)
				return;
			if(traverseInOrderFillBuffer(parent->right, end, buf) == 1)
				return;
		}
	}
}

int traverseInOrderFillBuffer(struct tnode *node, struct tnode *end, void* buf)
{
	if(node != NULL) 
	{
		if(traverseInOrderFillBuffer(node->left, end, buf) == 1)
			return 1;
		filler(buf, ((struct t_snapshot*)(node->data))->name, NULL, 0);
		if(node == end)
			return 1;
		return traverseInOrderFillBuffer(node->right, end, buf);
	}
}

int snapshotComp(void* a, void* b)
{
	struct t_snapshot *first, *second;
	first = a;
	second = b;
	return first->time - second->time;
}
void snapshotPrint(void* a)
{
	struct t_snapshot *snap;
	snap = a;
	printf("%s\n", snap->name);
}


/* insert a tnode into the binary tree */
struct tnode *tnode_insert(struct tnode *p, void* value, comparator compFunc) {
 struct tnode *tmp_one = NULL;
 struct tnode *tmp_two = NULL;

 if(p == NULL) {
  /* insert [new] tnode as root node */
  p = (struct tnode *)malloc(sizeof(struct tnode));
  p->data = value;
  p->left = p->right = p->parent = NULL;
 } else {
  tmp_one = p;
  /* Traverse the tree to get a pointer to the specific tnode */
  /* The child of this tnode will be the [new] tnode */
  while(tmp_one != NULL) {
   tmp_two = tmp_one;
   if(compFunc(tmp_one ->data, value) > 0)
    tmp_one = tmp_one->left;
   else
    tmp_one = tmp_one->right;
  }

  if(compFunc(tmp_two ->data, value) > 0) {
   /* insert [new] tnode as left child */
   tmp_two->left = (struct tnode *)malloc(sizeof(struct tnode));
   tmp_two->left->parent = tmp_two->left;
   tmp_two = tmp_two->left;
   tmp_two->data = value;
   tmp_two->left = tmp_two->right = NULL;
  } else {
   /* insert [new] tnode as left child */
   tmp_two->right = (struct tnode *)malloc(sizeof(struct tnode)); 
   tmp_two->right->parent = tmp_two->right;
   tmp_two = tmp_two->right;
   tmp_two->data = value;
   tmp_two->left = tmp_two->right = NULL;
  }
 }

 return(p);
}

/* print binary tree inorder */
void print_inorder(struct tnode *p, printer printFunc) {
 if(p != NULL) {
  print_inorder(p->left, printFunc);
  printFunc( p->data);
  print_inorder(p->right, printFunc);
 }
}

/* print binary tree preorder */
void print_preorder(struct tnode *p, printer printFunc) {
 if(p != NULL) {
  printFunc( p->data);
  print_preorder(p->left, printFunc);
  print_preorder(p->right, printFunc);
 }
}

/* print binary tree postorder */
void print_postorder(struct tnode *p, printer printFunc) {
 if(p != NULL) {
  print_postorder(p->left, printFunc);
  print_postorder(p->right, printFunc);
  printFunc( p->data);
 }
}

/* returns the total number of tree nodes */
int tnode_count(struct tnode *p) {
 if(p == NULL)
  return 0;
 else {
  if(p->left == NULL && p->right == NULL)
   return 1;
  else
   return(1 + (tnode_count(p->left) + tnode_count(p->right)));
 }
}

/* exchange all left and right tnodes */
struct tnode *tnode_swap(struct tnode *p) {
 struct tnode *tmp_one = NULL; 
 struct tnode *tmp_two = NULL;

 if(p != NULL) { 
  tmp_one = tnode_swap(p->left);
  tmp_two = tnode_swap(p->right);
  p->right = tmp_one;
  p->left  = tmp_two;
 }

 return(p);
}

/* locate a value in the btree */
struct tnode *tnode_search(struct tnode *p, void* key, comparator compFunc) {
 struct tnode *temp;
 temp = p;

 while(temp != NULL) {
  if(compFunc(temp->data, key) == 0)
   return temp;
  else if(compFunc(temp->data, key) > 0)
   temp = temp->left;
  else
   temp = temp->right;
 }

 return NULL;
}

/* locate a minimum value in the btree */
struct tnode *tnode_searchmin(struct tnode *p) {
 if(p == NULL)
  return NULL;
 else
  if(p->left == NULL)
   return p;
  else
   return tnode_searchmin(p->left);
}

/* locate a maximum value in the btree */
struct tnode *tnode_searchmax(struct tnode *p) {
 if(p != NULL)
  while(p->right != NULL)
   p = p->right;

 return p;
}

/* destroy the binary tree */
void tnode_destroy(struct tnode *p) {
 if(p != NULL) {
  tnode_destroy(p->left);
  tnode_destroy(p->right);

  free(p);
 }
}

/*
int main(void) {
 int i;
 int demo_nr[] = {1, 3, 4, 7, 2, 9, 9, 0, 5, 6, 8, 7, 1, 2, 4};
 struct t_snapshot pics[5];
 
 pics[0].time = 14;
 pics[1].time = 9;
 pics[2].time = 2;
 pics[3].time = 3;
 pics[4].time = 15;
 
 for(i = 0; i < 5; i++)
 {
	 pics[i].name = malloc(10 * sizeof(char));
 }
 pics[0].name = "fred";
 pics[1].name = "shaggy";
 pics[2].name = "bob";
 pics[3].name = "waters";
 pics[4].name = "lasty";
 
 struct tnode *root = NULL; 
 struct tnode *searchval = NULL;
 int querry = 0;

 for(i = 0; i < 5; i++)
  root = tnode_insert(root, &(pics[i]), snapshotComp);

 printf("=-=-=\n");
 printf("Total number of tree nodes: %d\n", tnode_count(root));
 printf("inorder  : ");
 print_inorder(root, snapshotPrint);
 printf("\n");

 printf("preorder : ");
 print_preorder(root, snapshotPrint);
 printf("\n");

 printf("postorder: ");
 print_postorder(root, snapshotPrint);
 printf("\n");

 printf("=-=-=\n");
 printf("Enter integer, find: ");
 scanf("%d", &querry);
 searchval = tnode_search(root, &(pics[2]), snapshotComp);
 if(searchval == NULL)
  printf(" Not! found in btree\n");
 else
  printf(" * Found! in btree\n");

 searchval = NULL;
 printf("Searching for Minimum value\n");
 searchval = tnode_searchmin(root);
 if(searchval == NULL)
  printf(" * Minimum Not! found in btree ?\n");
 else
 {
  snapshotPrint(searchval->data);
  printf("\n");
 }



 printf("=-=-=\n");
 printf("Exchanging all tree nodes: left <-> right\n");
 root = tnode_swap(root);

 printf("inorder  : ");
 print_inorder(root,snapshotPrint);
 printf("\n");

 printf("preorder : ");
 print_preorder(root,snapshotPrint);
 printf("\n");

 printf("postorder: ");
 print_postorder(root,snapshotPrint);
 printf("\n");

 printf("=-=-=\n");
 printf("Destroying btree... bye!\n");
 tnode_destroy(root);

 return 0;
}

*/
