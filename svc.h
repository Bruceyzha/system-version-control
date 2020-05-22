#ifndef svc_h
#define svc_h

#include <stdlib.h>

typedef struct resolution {
    // NOTE: DO NOT MODIFY THIS STRUCT
    char *file_name;
    char *resolved_file;
} resolution;

/*The struct file is for keep the path of file*/

struct file{
    char name[50];
};

/*The struct file name is for keeping file path,
the hash value of the file and the pointer of the next file*/

struct file_name{
    char name[50];
    int hash;
    struct file_name * next;
};

/*The struct commit_id is for keeping the changed file path
and the changed type, if type is 1, that is addition,
type is 2, that is remove, type is 3, that is modification.
This struct will be used for commit id calculation.*/

struct commit_id{
    char name[50];
    int type;
};

/*The struct modify is for recording the difference
of modification file, inlcude the file path and
 the hash value. The struct will be used for print
 commit*/

struct modify{
    char modi[50];
    int old_hash;
    int new_hash;
};

/*The struct node is for recording all detail of a
 commit. The variable name is keep all tracked file
  in this commit. The vaiable prev is for track the
   prev commit, and the variable prev2 is for track
 another prev commit after merge.The variable added,
moed,removed stored the changed file and the number
 of the file in this commit.*/

struct node{
    struct file_name* name;
    char* commit_id;
    char message[50];
    struct node *prev;
    struct node *prev2;
    struct file* added;
    int addnum;
    struct file* removed;
    int rmnum;
    struct modify* moed;
    int modnum;
    int file_count;
};

/*The struct branch store the information of a 
branch. Include the name of the branch, all the
 commit of this branch and previous branchs.*/

struct branch{
    char name[20];
    struct node* commit;
    struct branch* prev;
};

/*The struct state keep all information of the svc.
The variable head is a pointer to link to the all
the commit in current branch. Current_branch stores
the address of the current branch. Branch stores
all the branch in the svc. The commit is an array
 to store all the commit in the SVC.*/

struct state{
    struct node* head;
    struct branch* current_branch;
    struct branch* branch;
    int branch_count;
    struct file_name* file;
    struct node** commit;
    int commit_count;
};

/* new function*/

void freefile(struct file_name* file);

void freecommit(struct node* commit);

int check_commit(void* helper,struct node* commit);

int order(char* name1,char* name2);

void *svc_init(void);

void cleanup(void *helper);

int hash_file(void *helper, char *file_path);

char *svc_commit(void *helper, char *message);

void *get_commit(void *helper, char *commit_id);

char **get_prev_commits(void *helper, void *commit, int *n_prev);

void print_commit(void *helper, char *commit_id);

int svc_branch(void *helper, char *branch_name);

int svc_checkout(void *helper, char *branch_name);

char **list_branches(void *helper, int *n_branches);

int svc_add(void *helper, char *file_name);

int svc_rm(void *helper, char *file_name);

int svc_reset(void *helper, char *commit_id);

char *svc_merge(void *helper, char *branch_name, resolution *resolutions, int n_resolutions);

#endif

