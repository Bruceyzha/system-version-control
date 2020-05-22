#include "svc.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/*The initiazation of the system of version control,
 create the start branch "master", set the "master"
 branch as the current branch and store it to the svc
branch list, set all the parameter is null or 0, to 
avoid the wild pointer issue.*/ 

void *svc_init (void) {
    struct branch* new = malloc(sizeof(struct branch));
    new->commit = NULL;
    char* master = "master";
    strcpy(new->name,master);
    new->prev = NULL;
    struct state* helper = malloc(sizeof(struct state));
    helper->head = NULL;
    helper->branch_count = 1;
    helper->current_branch = new;
    helper->branch = new;
    helper->file = NULL;
    helper->commit = NULL;
    helper->commit_count = 0;
    return (void*)helper;
}

/*Free all the tracked files in this linked list*/

void freefile (struct file_name* file) {
    struct file_name* cursor = file;
    struct file_name* tmp;
    while(cursor != NULL){
        tmp = cursor->next;
        free(cursor);
        cursor = tmp;
    }
}

/*Free the single commit and the all
 information of the commit contained.*/

void freecommit (struct node* commit) {
        freefile(commit->name);
        free(commit->added);
        free(commit->moed);
        free(commit->removed);
        free(commit->commit_id);
        free(commit);
        commit=NULL;
}

/*Clean up everything from the svc in the heap.*/

void cleanup(void *helper) {
    struct state* new = (struct state*)helper;
    struct branch* cursor;
    cursor = new->branch;
    struct branch* tmp;
    for(int i = 0;i < new->commit_count ;i++){
        if(new->commit[i] == NULL){
            continue;
        }
        freecommit(new ->commit[i]);
        new->commit[i] = NULL;
    }
    while(cursor != NULL){
        tmp = cursor->prev;
        free(cursor);
        cursor = tmp;
    }
    freefile(new->file);
    free(new->commit);
    free(new);
    return;

}

/*Calculate the hash number of the file, use the 
provided algorithm.Firstly, according to the 
file_path of provide to find the file,if path is
wrong, function will return -1, if the file is
not exist, the function will return -2. Secondly,
find the file length and all content of the file,
 follow the algorithm to calculated it. */

int hash_file(void *helper, char *file_path) {
    if(file_path == NULL){
        return -1;
    }
    FILE* file = fopen(file_path,"r");
    if(file == NULL){
        return -2;
    }
    char *string;
    long lsize;
    fseek(file,0,SEEK_END);
    lsize = ftell(file);
    rewind(file);
    string = malloc(lsize);
    fread(string,1,lsize,file);
    size_t hash = 0;
/*There is type change to the unsigned char, to 
avoid the specific character not exist in the ASCii.*/
    for(size_t i = 0;i<strlen(file_path); i++){
        hash = hash + (unsigned char)file_path[i];
    }
   hash = hash%1000;
    for(size_t i = 0;i< lsize;i ++){
        hash = hash+ (unsigned char) string[i];
    }
    hash = hash%2000000000;
    free(string);
    return hash;
}

/*The function check_commit is that the function
check the uncommit change. It is for the later
function. The function will compare the current
tracked file with the last commit tracked file.
If there is no difference, the function will return 
0 and there is no uncommited change, if they have 
difference, that prove they have uncommit change 
and function will return 1.*/

int check_commit(void* helper,struct node* commit) {
    struct state* new = (struct state*)helper;

/*Variable add is current tracked file,
 cursor is last commit tracked file*/

    struct file_name* add = NULL;
    struct file_name* cursor = NULL;
    add = new ->file;
    int a = 0;

/*The varible a is for checking the change.
If "a" always equal to 0 until each for loop 
is done, that prove there is no change.*/

    while(add != NULL){
    cursor = commit->name;
        while(cursor != NULL){
            if(strcmp(add->name,cursor->name) == 0){
                if(hash_file(helper,add->name) != cursor->hash){
                    for(int i = 0;i<new->head->modnum;i++){
                        if(strcmp(add->name,new->head->moed[i].modi) == 0){
                            a = 1;
                            break;
                        }
                    }
                    if(a == 0){
                        return 1;
                    }
                    break;
                }
                else{
                    break;
                }
            }
            cursor = cursor->next;
        }
        if(cursor == NULL){
            a = 0;
            for(int i = 0;i<new->head->addnum; i++){
                if(strcmp(add->name,new->head->added[i].name)==0){
                    a = 1;
                    break;
                    }
                }
            if(a == 0){
                return 1;
            }
    }
    add = add->next;
    }
    struct file_name* remove = NULL;
    cursor = commit->name;
    while(cursor != NULL){
         remove = new->file;
        while(remove != NULL){
            if(strcmp(remove->name,cursor->name) == 0){
                break;
            }
            remove = remove->next;
        }
        if(remove == NULL){
            a = 0;
            for(int i = 0;i<new->head->rmnum;i++){
                if(strcmp(cursor->name,new->head->removed[i].name)== 0){
                    a = 1;
                    break;
                    }
                }
            if(a == 0){
                return 1;
            }
        }
        cursor = cursor->next;
    }
    return 0;
}

/*The function order is working for the function
 svc_commit. Because the Commit ID Algorithm need
increasing alphabetical order of tracked file, 
the function will change the oreder of the file*/

int order(char* name1,char* name2){
    int a = strlen(name1);
    int b = strlen(name2);
    int value = 0;
    if(a >= b){
        for(int i = 0; i<b ; i++){
            value = tolower((unsigned char)name1[i]) - tolower((unsigned char)name2[i]);
            if(value != 0){
                return value;
            }
        }
    }
    if(a < b){
        for(int i = 0;i<a ;i++){
            value = tolower((unsigned char)name1[i]) - tolower((unsigned char)name2[i]);
            if(value != 0){
                return value;
            }
        }
    }
    return 0;
}

/*The function svc_commit could calculate
 the commit_id. The function use the message, 
 the changed file name and Commit ID Algorithm 
 to generate a commit decimal id then change to 
 hex commit Id. The svc_commit also can create 
 a new commit and save all changed files and 
 tracked files into the commit.*/ 

char *svc_commit(void *helper, char *message) {
    if(message == NULL) {
        return NULL;
    }
    int id = 0;
    for(int i = 0;i < strlen(message) ; i++) {
        id=id+message[i];
    }
    id = id % 1000; 
    struct state* new = (struct state*) helper;

/* variable add is current tracked file, 
cursor is last commit tracked file.
Overall is all changed file*/

    struct file_name* add = NULL;
    struct file_name* cursor = NULL;
    add = new-> file;
//create a new commit.
    struct node* commit = NULL;
    commit = malloc(sizeof(struct node));
    int cont=0;
    struct commit_id *overall = NULL;
    strcpy(commit-> message,message);
    commit->added = NULL;
    commit->removed = NULL;
    commit->moed = NULL;
    commit->addnum = 0;
    commit->modnum = 0;
    commit->rmnum = 0;
//Check the first commit
    if(new->head == NULL){
        while(add != NULL){
            //check the manual deletion.
            if(hash_file(helper,add->name) == -2){
                struct file_name *tmp = add->next;
                svc_rm(helper,add->name);
                add = tmp;
                continue;
                }
//Save the added file into commit.
        commit->added = realloc(commit->added,sizeof(struct file)*(commit->addnum+1));
        strcpy(commit-> added[commit->addnum].name,add-> name);
        commit->addnum += 1;
        overall = realloc(overall,sizeof(struct commit_id)*(cont+1));
        strcpy(overall[cont].name,add->name);
        overall[cont].type = 1;
        cont++;
        add = add->next; 
        }
    }
    else{
//reset the cursor pointer.
    add = new->file;
    while(add != NULL){
        if(hash_file(helper,add->name) == -2){
                    struct file_name *tmp = add->next;
                    svc_rm(helper,add-> name);
                    add = tmp;
                    continue;
                }
    cursor = new->head->name;
        while(cursor != NULL){
            if(strcmp(add->name,cursor->name) == 0){
                if(hash_file(helper,add->name) != cursor->hash){
//Save the changed file into commit.
                    commit->moed = realloc(commit->moed,sizeof(struct modify)*(commit->modnum+1));
                    strcpy(commit->moed[commit->modnum].modi, add->name);
                    commit->moed[commit->modnum].old_hash = cursor->hash;
                    commit->moed[commit->modnum].new_hash = add->hash;
                    commit->modnum += 1;
                    overall = realloc(overall,sizeof(struct commit_id)*(cont+1));
                    strcpy(overall[cont].name,add->name);
                    overall[cont].type = 3;
                    cont ++;
                    break;
                }
                else{
                    break;
                }
            }
            cursor = cursor->next;
        }
        if(cursor == NULL){
//Save the added file into commit.
            commit->added = realloc(commit->added, sizeof(struct file)*(commit->addnum+1));
            strcpy(commit->added[commit->addnum].name, add->name);
            commit->addnum += 1;
            overall = realloc(overall, sizeof(struct commit_id)*(cont+1));
            strcpy(overall[cont].name, add->name);
            overall[cont].type = 1;
            cont++;
    }
    add = add->next;
    }
    struct file_name* remove = NULL;
    cursor = new->head->name;
    while(cursor != NULL){
         remove = new-> file;
        while(remove != NULL){
            if(strcmp(remove->name, cursor->name) == 0){
                break;
            }
            remove = remove->next;
        }
        if(remove == NULL){
//Save the removed file into commit.
            commit->removed = realloc(commit->removed, sizeof(struct file)*(commit->rmnum+1));
            strcpy(commit->removed[commit->rmnum].name, cursor->name);
            commit->rmnum += 1;
            overall = realloc(overall, sizeof(struct commit_id)*(cont+1));
            strcpy(overall[cont].name, cursor->name);
            overall[cont].type = 2;
            cont++;
        }
        cursor = cursor->next;
    }
    }
    if(overall == NULL){
        free(commit);
        return NULL;
    }
//create the ordername to temporary save the file name.
    char ordername[50];
    int ordertype;
//Change the order of the changed file for Algorithm.
    for(int i = 0;i< cont-1;i++){
        for(int j = i+1;j< cont;j++){
            if(order(overall[i].name, overall[j].name) > 0){
                strcpy(ordername, overall[i].name);
                strcpy(overall[i].name, overall[j].name);
                strcpy(overall[j].name, ordername);
                ordertype = overall[i].type;
                overall[i].type = overall[j].type;
                overall[j].type = ordertype;
            }
        }
    }
//Commit ID Algorithm
    for(int i = 0;i<cont;i++){
        if(overall[i].type == 1)id = id+376591;
        if(overall[i].type == 2)id = id+85973;
        if(overall[i].type == 3)id = id+9573681;
        for(int j = 0;j<strlen(overall[i].name); j++){
            id = (id*((unsigned char)overall[i].name[j]%37))%15485863+1;
                }
    }
    free(overall);
    cursor = new->file;
    char commit_id[7];
    sprintf(commit_id,"%06x",id);
    commit->commit_id = malloc(sizeof(char)*7);
//set the all information of the new commit.
    strcpy(commit->commit_id,commit_id);
    commit->file_count = 0;
    commit->name = NULL;
    commit->prev = NULL;
    commit->prev2 = NULL;
//Generate the traked files of new commit from the current tracked file.
    while(cursor != NULL){
        struct file_name* commit_file = malloc( sizeof(struct file_name));
        strcpy(commit_file->name, cursor->name);
        commit_file->hash = cursor->hash;
        commit_file->next = NULL;
        if(commit->name == NULL){
            commit->name = commit_file;
        }
        else{
            struct file_name* cursor2 = NULL;
    for(cursor2=commit->name;cursor2->next != NULL;cursor2=cursor2->next){}
        cursor2->next = commit_file;
        }
        commit->file_count += 1;
        cursor = cursor->next;
    }

/*Update the new statement of svc.
 New commit will be head commit, 
 current branch also have a new commit 
 and total commit number will be increased.*/

    commit->prev = new->head;
    new->head = commit;
    new->current_branch->commit = commit;
    new->commit = realloc(new->commit,sizeof(struct node*)*(new->commit_count+1));
    new->commit[new->commit_count] = commit;
    new->commit_count++;
    return commit->commit_id;
}

/* The function get_commit could 
find the commit node by commit id*/

void *get_commit(void *helper, char *commit_id) {
    struct state* new = (struct state*)helper;
    struct node* cursor = new->head;
//Travesal current branch to find the commit. 
    while(cursor != NULL){
        if(strcmp(cursor->commit_id,commit_id) == 0){
            return cursor;
        }
        cursor = cursor->prev;
    }
    return NULL;
}

/*The function get_prev_commits 
can return a list of previous 
commits from the provided commit, 
and the number of the previous. 
For merge commit, the list will 
collect the main branch first then 
the merged branch*/

char **get_prev_commits(void *helper, void *commit, int *n_prev) {
    struct state* new = (struct state*) helper;
    struct node* oldcommit = (struct node*) commit;
    struct node* cursor = new->head;
//Check whether commit exist. 
    if(cursor == NULL){
        *n_prev = 0;
        return NULL;
    }
/*set cursor is the last commit
 and compare the provided commit 
 with the last commit*/

   while(cursor->prev != NULL){
        cursor = cursor->prev;
    }
    if(commit == NULL||
    strcmp(oldcommit->commit_id,cursor->commit_id) == 0){
        *n_prev = 0;
        return NULL;
    }
    if(n_prev == NULL){
        return NULL;
    }
/*varible num is the number of the main branch
 prev commit. a is the number of same commit of two branch.*/

    int a = 0;
    int num = 0;
    cursor = oldcommit->prev;
    while(cursor != NULL){
        num = num + 1;
        cursor = cursor->prev;
    }
    int num2 = 0;
    cursor = oldcommit->prev2;
    while(cursor != NULL){
        num2 = num2+1;
        cursor = cursor->prev;
    }
    int total = num + num2;
//create the array of list of prev commit include the same commit.
    char **prev_commit = malloc(sizeof(char*)*total);
    if(num2 == 0){
    cursor = oldcommit->prev;
    for(int i = num-1;i>-1;i--){
        prev_commit[i] = cursor->commit_id;
        cursor = cursor->prev;
        }
        *n_prev = total;
        return prev_commit;
    }
//If the commit is merge commit, add the merged commit into the prev_commit.
    else{
        cursor = oldcommit->prev2;
        for(int i = total-1;i > total-num2-1;i--){
        prev_commit[i] = cursor->commit_id;
        cursor = cursor->prev;
        }
        cursor = oldcommit->prev;
        for(int i = total-num2-1; i>-1; i--){
            for(int j = total-1;j>total-num2-1; j--){
                if(strcmp(prev_commit[j], cursor->commit_id)){
                    a += 1;
                    break;
                }
            }
            if(a != 0){
                continue;
            }
            prev_commit[i] = cursor->commit_id;
            cursor = cursor->prev;
        }
//create a array of prev commit without same commit.
        char **real_commit = malloc(sizeof(char*)*(total-a));
        for(int i = total-a-1;i>-1;i--){
            real_commit[i] = prev_commit[i+a];
        }
        free(prev_commit);
        *n_prev = total-a;
        return real_commit;
    }
    
    return prev_commit;
}

/*Print the statement of the commit*/

void print_commit(void *helper, char *commit_id) {
    if(commit_id == NULL){
        printf("Invalid commit id\n");
        return;
    }
    struct state* new = (struct state*)helper;
    struct node* cursor = get_commit(helper,commit_id);
    if(cursor == NULL){
        printf("Invalid commit id\n");
        return;
    }
    printf("%s [%s]: %s\n",cursor->commit_id,new->current_branch->name,cursor->message);
    if(cursor->added != NULL){
        for(int i = 0;i<cursor->addnum;i++){
            printf("    + %s\n",cursor->added[i].name);
        }
    }
    if(cursor->removed != NULL){
        for(int i = 0;i<cursor->rmnum ; i++){
            printf("    - %s\n",cursor->removed[i].name);
        }
    } 
    if(cursor->moed != NULL){
        for(int i = 0;i<cursor->modnum; i++){
            printf("    / %s [%d --> %d]\n",cursor->moed[i].modi,cursor->moed[i].old_hash, cursor->moed[i].new_hash);
        }
    }
    printf("\n    Tracked files (%d):\n", cursor->file_count);
    struct file_name* tmp = cursor->name;
    while(tmp != NULL){
        printf("    [%10d] %s\n", tmp->hash, tmp->name);
        tmp = tmp->next;
    }
    return;  
}

/* create a new branch in the function*/

int svc_branch(void *helper, char *branch_name) {
    // TODO: Implement
    if(branch_name==NULL){
        return -1;
    }
//check the branch name is correct.
    for(int i = 0;i<strlen(branch_name); i++){
        if ((branch_name[i]<'a')|| (branch_name[i]>'z')) {
            if ((branch_name[i]<'A')|| (branch_name[i]>'Z')) {
                if ((branch_name[i]<'0')|| (branch_name[i]>'9')) {
                    if ((branch_name[i] != '_')&& (branch_name[i]!='/')&& (branch_name[i]!='-')) {
                        return -1;
                    }
                }
            }
        }
    }
    struct state* new = (struct state*)helper;
    struct branch* cursor = new->branch;
    while(cursor != NULL){
        if(strcmp(cursor->name,branch_name) == 0){
            return -2;
        }
        cursor = cursor->prev;
    }
    int a = check_commit(helper,new->head);
    if(a == 1){
        return -3;
    }
/*initialise the new branch, and add it into the svc branch list*/
    struct branch* new_branch = malloc(sizeof(struct branch));
    new_branch->commit = new->head;
    strcpy(new_branch->name, branch_name);
    new_branch->prev = new->branch;
    new->branch = new_branch;
    new->branch_count += 1;
    return 0;
}

/*The function svc_checkout could 
change the current branch to the 
provided branch. The current tracked 
file will be the tracked file of head 
commit of new branch. Before checkout, 
uncommited change is not accepted.*/

int svc_checkout(void *helper, char *branch_name) {
    if(branch_name == NULL){
        return -1;
    }
    struct state* new = (struct state*)helper;
    struct branch* cursor = new->branch;
//Find the provided branch. 
    while(cursor != NULL){
        if(strcmp(cursor->name,branch_name) == 0){
            break;
        }
        cursor = cursor->prev;
    }
    if(cursor == NULL){
        return -1;
    }
    //Check the uncommited change.
    int a = check_commit(helper,new->head);
    if(a == 1){
        return -2;
    }
//change the current branch.
    new->current_branch = cursor;
    new->head = cursor->commit;
//free current tracked file in the svc.
    struct file_name* tmp = new->file;
    struct file_name* next = NULL;
    while(tmp != NULL){
        next = tmp->next;
        free(tmp);
        tmp = next;
    }
//initialize the current tracked file, and 
//generate the new tracked file from the head commit.
    new->file = NULL;
    if(new->head != NULL){
        tmp = new->head->name;
        while(tmp != NULL){
        struct file_name* commit_file = malloc( sizeof(struct file_name));
        strcpy(commit_file->name,tmp->name);
        commit_file->hash = hash_file(helper,commit_file->name);
        commit_file->next = NULL;
        if(new->file == NULL){
            new->file = commit_file;
        }
        else{
            struct file_name* cursor2=NULL;
    for(cursor2 = new->file;cursor2->next != NULL; cursor2 = cursor2->next){

    }
        cursor2->next = commit_file;
        }
        tmp = tmp->next;
    }
    }
    return 0;
}

/* The function list_branches could provide all the 
branch of svc and the number of the branch, and print their name.*/

char **list_branches(void *helper, int *n_branches) {
    if(n_branches == NULL){
        return NULL;
    }
    struct state* new = (struct state*)helper;
    *n_branches = new->branch_count;
//Create an array to save the branch list.
    char** list = malloc(sizeof(char*)*(*n_branches));
    struct branch* cursor = new->branch;
    int i = 0;
//Save the branch from back to top.
    for(i = (*n_branches-1);i>-1;i--){
        list[i] = cursor->name;
        cursor = cursor->prev;
    }
//print list from top to back.
    for(i = 0;i<(*n_branches);i++){
        printf("%s\n", list[i]);
    }
    return list;
}

/*The function svc_add can 
add a new file to the svc tracked file*/

int svc_add(void *helper, char *file_name) {
    if(file_name == NULL){
        return -1;
    }
    struct state* new = (struct state*) helper;
    struct file_name* cursor = NULL;
    cursor = new-> file;
    int hash_value = hash_file(helper, file_name);
    while(cursor != NULL){
//check the file already exist in the tracked 
//files list by comparing the name. 
        if(strncmp(cursor->name,file_name, strlen(file_name)) == 0){
            if(hash_value != cursor->hash){
                cursor->hash = hash_value;
                return hash_value;
            }
            return -2;
        }
        cursor = cursor->next;
    }
//Check wether the file exist
    if(hash_value == -2){
        return -3;
    }
    struct file_name *new_file = malloc(sizeof(struct file_name));
    strcpy(new_file->name,file_name);
    new_file->hash = hash_value;
//check if the file is the first file of the current tracked file
    if(new->file == NULL){
        new_file->next = NULL;
        new->file = new_file;
    }
    else{
    struct file_name* tmp = NULL;
    for(tmp = new->file;tmp->next != NULL;tmp = tmp->next);
    new_file->next = NULL;
    tmp->next = new_file;
    }
    return hash_value;
}

/*The function svc_rm can 
remove a file to the svc tracked file*/

int svc_rm(void *helper, char *file_name) {
    if(file_name == NULL){
        return -1;
    }
    struct state* new = (struct state*)helper;
    struct file_name* cursor = NULL;
    struct file_name* tmp = NULL;
    cursor = new->file;
    while(cursor != NULL){
        if(strcmp(cursor->name,file_name) == 0){
            if(tmp == NULL){
                new->file = cursor->next;
            }
            else{
            tmp->next = cursor-> next;
            }
            int value = cursor-> hash;
            free(cursor);
            return value;
        }
        tmp = cursor;
        cursor = cursor-> next;
    }
    return -2;
}

/*The reset function will reset the commit
to prev commit of current branch and remove
the commit that after the reset commit. and
change the content of the tracked file to
 the reset version*/

int svc_reset(void *helper, char *commit_id) {
    if(commit_id == NULL){
        return -1;
    }
    struct state* new = (struct state*)helper;
    struct node* cursor = new->head;
    while(cursor != NULL){
        if(strcmp(cursor->commit_id,commit_id) == 0){
            break;
        }
        cursor = cursor->prev;
    }
    if(cursor == NULL){
        return -2;
    }
/*The variable detach is the cursor of the commit.
Branch cursor is for travesal all branch, samecommit 
is an array of commit exist in different branches*/
    struct node* detach = new->current_branch->commit;
    struct branch* branch_cursor;
    struct node* branch_commit;
    struct node** samecommit = NULL;
    int samecont = 0;
    int check = 0;
//Find all detach commit(after the reset commit) 
//and save it is the samecommit array.
    while(detach != cursor){
        branch_cursor = new->branch;
        while(branch_cursor != NULL){
            branch_commit = branch_cursor->commit;
            while(branch_commit != NULL){
                if(strcmp(branch_commit->commit_id,detach->commit_id) == 0){
                    for(int i = 0;i<samecont;i++){
                        if(strcmp(branch_commit->commit_id,samecommit[i]->commit_id)){
                            check = 1;
                            break;
                        }
                    }
                    if(check == 0){
                        samecommit = realloc(samecommit,sizeof(struct node)* (samecont+1));
                        samecommit[samecont] = detach;
                        samecont++;
                    }
                }
                branch_commit = branch_commit->prev;
            }
            branch_cursor = branch_cursor->prev;
        }
        detach = detach->prev;
    }
//Free the detach commit and delete them from the svc commit list.
    for(int i = 0;i<samecont; i++){
        for(int j = 0;j<new->commit_count; j++){
            if(samecommit[i] == new->commit[i]){
                freecommit(new->commit[i]);
                new->commit[i] = NULL;
            }
        }
    }
    free(samecommit);
//Change the svc head commit is provided commit.
    new->head = cursor;
    new->current_branch->commit = cursor;
    struct file_name* tmp = new->file;
    struct file_name* next = NULL;
    while(tmp != NULL){
        next = tmp->next;
        free(tmp);
        tmp = next;
    }
    new->file = NULL;
    tmp = new->head->name;
    while(tmp != NULL){
        struct file_name* commit_file = malloc( sizeof(struct file_name));
        strcpy(commit_file->name,tmp->name);
        commit_file->hash = tmp->hash;
        commit_file->next = NULL;
        if(new->file == NULL){
            new->file=commit_file;
        }
        else{
            struct file_name* cursor2 = NULL;
    for(cursor2 = new->file;cursor2->next != NULL;cursor2 = cursor2->next){

    }
        cursor2->next = commit_file;
        }
        tmp = tmp->next;
    }
    
    return 0;
}

/* The function merge can merge the 
provide branch into the current branch, 
and merge the tracked file. If there are 
conflictingfiles, it will appear in the 
resolutions array. Change the content of 
conflicting files to the content of the 
resolution_file.*/

char *svc_merge(void *helper, char *branch_name, struct resolution *resolutions, int n_resolutions) {
    if(branch_name == NULL){
        printf("Invalid branch name\n");
        return NULL;
    }
    struct state* new = (struct state*)helper;
    struct branch* branch_cursor = new->branch;
//Check the provided branch with the current branch.
    while(branch_cursor != NULL){
        if(strcmp(branch_cursor->name,branch_name) == 0){
            if(branch_cursor == new->current_branch){
                printf("Cannot merge a branch with itself\n");
                return NULL;
            }
            break;
        }
        branch_cursor = branch_cursor->prev;
    }
    if(branch_cursor == NULL){
        printf("Branch not found\n");
        return NULL;
    }
//Check the uncommited change.
    if(check_commit(helper,new->head) != 0){
        printf("Changes must be committed\n");
        return NULL;
    }
    struct file_name* file_cursor = branch_cursor->commit->name;
    struct file_name* file_cursor2;

/*Travesal the tracked file or two branch, 
if there are the same file and the file has 
been changed, travesal the resolution and copy 
the resolution content to the file content then 
add it to the new tracked files.*/

    while(file_cursor != NULL){
        file_cursor2 = new->file;
        while(file_cursor2 != NULL){
            if(strcmp(file_cursor->name,file_cursor2->name) == 0){
                if(hash_file(helper,file_cursor->name) != hash_file(helper,file_cursor2->name)){
                    for(int i = 0;i<n_resolutions; i++){
                        if(strcmp(file_cursor->name,resolutions[i].file_name) == 0){
                            FILE* file = fopen(resolutions[i].file_name,"r");
                            fseek(file,0,SEEK_END);
                            long size = ftell(file);
                            rewind(file);
                            char* buffer = malloc(size);
                            fread(buffer,size,1,file);
                            fclose(file);
                            FILE* new_file = fopen(file_cursor->name,"w");
                            fwrite(buffer,1,size,file);
                            fclose(new_file);
                            file_cursor->hash = hash_file(helper, file_cursor->name);
                        }
                    }
                }
                break;
            }
            file_cursor2 = file_cursor2->next;
        }
//If there are new files, direcly add it into the tracked files.
        if(file_cursor2 == NULL){
            struct file_name* new_file = malloc(sizeof(struct file_name));
            strcpy(new_file->name, file_cursor->name);
            new_file->hash = file_cursor->hash;
            new_file->next = NULL;
            struct file_name* last = NULL;
            for(last=new->file;last->next != NULL;last = last->next);
            last->next = new_file;
        }
        file_cursor = file_cursor->next;
    }
    int length = strlen(branch_name) + strlen("Merged branch ");
    char* message = (char*)malloc(sizeof(char)*length+1);
    sprintf(message, "Merged branch %s", branch_name);
//Have a new commit with the merge commit.
    char* commit_id = svc_commit(helper,message);
    new->head->prev2 = branch_cursor->commit;
    printf("Merge successful\n");
    free(message);
    return commit_id;
}




/*First time write over 1000 lines code, 
nearly finish the almost function,except 
the binary files read and wirte, save the 
file content for the reset. Over half month 
work, really appreciate all the tutor and 
staff help me on the ed, I can not finish 
the assignment without you, thank you.*/
