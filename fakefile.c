#include "fakefile.h"
#include "safe-fork.h"
#include "split.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*Name: Brooke Bailey
  TerpConnect Login ID: bbailey7
  University ID: 117543001
  Discussion Section: 0208
  
  The purpose of this program is to simulate
  make. The program reads fakefiles, looks up
  targets in a fakefile, prints action lines, etc.

  Honor Pledge: 
  I pledge on my honor that I have not given or received any
  unauthorized assistance on this assignment/examination.
  -Brooke Bailey*/



/*Prototypes in fakefile.h*/
int exists(const char filename[]);
int is_older(const char filename1[], const char filename2[]);
Fakefile *read_fakefile(const char filename[]);
int lookup_target(Fakefile *const fakefile, const char target_name[]);
void print_action(Fakefile *const fakefile, int rule_num);
void print_fakefile(Fakefile *const fakefile);
int num_dependencies(Fakefile *const fakefile, int rule_num);
char *get_dependency(Fakefile *const fakefile, int rule_num,
		     int dependency_num);
int do_action(Fakefile *const fakefile, int rule_num);
void clear_fakefile(Fakefile **const fakefile);


/*This function exists takes in a const char parameter. 
  The function returns an integer that determines if 
  the parameter exists in the filesystem. The function
  returns 1 if the parameter filename exists in the 
  filesystem. Otherwise, if the file does not
  exist in the filesystem or the filename is
  NULL, the function returns 0.*/

int exists(const char filename[]) {
  struct stat buf;
  if (filename != NULL) {
    errno = 0;
    if ((stat(filename, &buf) != 1) && errno != ENOENT) {
      return 1;
    } else {
      return 0;
    }
  }
  return 0;
}

/*This function is_older takes in two parameters.
  The goal of this function is to compare the time
  that each file was created or modified. First,
  the two files must exist. If either of them or both
  do not exist, then the function will return 0. 
  Additionally, neither one can be NULL or it will 
  also return 0.
  Otherwise, if they both exist and filename1 is 
  older than filename2 then the function returns
  1. If filename1 is newer than filename2,
  the function returns 0.*/

int is_older(const char filename1[], const char filename2[]) {
  if (filename1 != NULL && filename2 != NULL 
      && exists(filename1) && exists(filename2)) {
    struct stat stat_p1;
    struct stat stat_p2;

    stat(filename1, &stat_p1);
    stat(filename2, &stat_p2);
    /*This is checking which is older.
      If stat_pl.st_mtime < stat_[2.st_mtime,
      then filename1 is older than filename2.*/
    if (stat_p1.st_mtime < stat_p2.st_mtime) {
      return 1;
    }
  }
  return 0;
}

/*This helper function add_dependencies has two
  parameters. The goal of this function is to
  add dependencies to the rule input into the
  parameter. The first parameter is the Rule 
  that will have these dependencies.
  The second parameter is the name of the 
  dependency being added. 
  The function returns 0 if no memory could
  be malloced to create a new dependency.
  Otherwise, it adds the dependency to the
  rule and returns 1.*/

int add_dependencies(Rules *rule, char *dependency_name){
  Dependencies *curr = rule->deps;
  Dependencies *new_dep = malloc(sizeof(Dependencies));
  char *name;

  if (new_dep == NULL) {
      return 0;
  }
  /*Checks to make sure there are rules.
    If no rules have been added yet then
    curr = NULL.*/
  if (curr != NULL) {
    while (curr->next != NULL) {
      curr = curr->next;
    }
    curr->next = new_dep;
  } else {
    /*This starts the rules off*/
    rule->deps = new_dep;
    new_dep->next = NULL;
  }
  /*Giving the dependency a name*/
  name = malloc(strlen(dependency_name) + 1);
  strcpy(name, dependency_name);
  new_dep->dep_word = name;

  new_dep->d_index = rule->num_deps;
  new_dep->next = NULL;
  return 1;
}

/*This helper function add_rule takes in two
  parameters. The goal of the function is to add 
  a new rule to the fakefile in the first
  parameter. The second parameter is a 
  char pointer that was read from a file
  by fgets(). The function simply returns
  the rule that was made by the function.
  This return will be used to run the 
  next function add_action. It will tell us
  which rule to add the action to.*/

Rules *add_rule(Fakefile *fakefile, char *line){
  Rules *curr = fakefile->rules;
  Rules *new_rule = malloc(sizeof(Rules));
  char **words = split(line);
  /*This index moves moves in array to get
    the dependencies.*/
  int index = 1;

  if (new_rule == NULL) {
      return NULL;
  }
  /*Checks to make sure there are rules.
    If no rules have been added yet then
    curr = NULL.*/
  if (curr != NULL) {

    while (curr->next != NULL) {
      curr = curr->next;
    }
    curr->next = new_rule;

  } else {
    /*This starts the rules off*/
    fakefile->rules = new_rule;
    new_rule->deps = NULL;
    new_rule->next = NULL;

  }

  new_rule->r_index = fakefile->num_rules;
  new_rule->num_deps = 0;
  /*Giving rule a name*/
  new_rule->tar_name = malloc(strlen(words[0]) + 1);
  strcpy(new_rule->tar_name, words[0]);
  free(words[0]);

  /*Filling in rule dependencies*/
  while (words[index]) {

    add_dependencies(new_rule, words[index]);
    free(words[index]);
    index++;
    (new_rule->num_deps)++;
  }
  free(words);

  return new_rule;		   
}

/*This helper function add_action takes
 in two parameters. The first parameter
 represents the Rule that the action 
 line is being added to. The second 
 parameter line is the line that is the 
 action line.*/

void add_action(Rules *rule, char *line){
  rule->action = malloc(strlen(line) + 1);

  if (rule->action == NULL) {
    return;
  }

  strcpy(rule->action, line);
}

/*This function read_fakefile takes in one
  parameter. The goal of this function is
  to open the file given in the parameter
  and read the components of a fakefile from
  it. It should then, store the components 
  in a fakefile that the function creates.
  If the parameter either does not exist in the
  filesystem or it is NULL, then the function
  returns NULL without allocating memory.
  Otherwise, it continues to create
  the fakefile and open the file.
  If it successfully reads the file then, 
  it returns the fakefile. 
  It may also return NULL if opening
  the file fails.*/

Fakefile *read_fakefile(const char filename[]) {
  Fakefile *fakefile;
  Rules *last_rule;
  int lastline_rule = 0;
  FILE *f;
  char line[1001];

  if (exists(filename) && filename != NULL) {
    fakefile = malloc(sizeof(Fakefile));
    fakefile->num_rules = 0;
    fakefile->rules = NULL;
    /*Opens stream and checks it's not null*/
    f = fopen(filename, "r");
    if (f == NULL) {
      return NULL;
    }

    /*Reading from FILE*/
    while (fgets(line, 1001, f)) {
      if (line[0] == '\n'|| line[0] == ' ') {
	continue;
      } else if (line[0] == '\t' && lastline_rule) {
	add_action(last_rule, line);
	lastline_rule = 0;
      } else if (line[0] != '\t' && lastline_rule == 0) {
	last_rule = add_rule(fakefile, line);
	fakefile->num_rules++;
	lastline_rule = 1;
      } else {
	return NULL;
      }
    }
    fclose(f);
    return fakefile;
  } 
  return NULL;
}

/*This function lookup_target takes
  in two parameters. The goal of the
  function is to find a rule in the
  fakefile parameter with the name
  entered as the second parameter. 
  It simply compares the name of the
  current rule to the name given in 
  the parameter. If the fakefile has a 
  rule with the same name as the 
  name given in the parameter, then it
  returns the rule index of that rule.
  If either parameter is NULL or if
  there is no rule in the fakefile
  that has the parameter name as its
  name, then the function returns -1.
*/

int lookup_target(Fakefile *const fakefile, const char target_name[]) {
  Rules *curr = fakefile->rules;
  if (fakefile != NULL && target_name != NULL) {
    while (curr != NULL && strcmp(curr->tar_name, target_name) != 0) {
      curr = curr->next;
    }

    if (curr != NULL) {
      return curr->r_index;
    } else {
      return -1;
    }
  }
  return -1;
}

/*This function print_action takes in
 two parameters. The goal is to print
 the action line of the rule with index
 rule_num (second parameter). It does
 this by searching through the fakefile
 (first parameter) and comparing the
 rule_num to the index of that rule.
 if the rule's index matched the rule_num,
 then the action line is printed.
 Otherwise, if fakefile is NULL or
 rule_num is invalid, then it does
 nothing and simply returns.
*/

void print_action(Fakefile *const fakefile, int rule_num) {
  Rules *curr = fakefile->rules;
  char **action;
  int index = 0;


  if (fakefile != NULL && rule_num >= 0 && rule_num < fakefile->num_rules) {

    /*Finds the rule*/
    while (curr->next != NULL && curr->r_index != rule_num) {
      curr = curr->next;
    }
    /*Setup to print action*/
    action = split(curr->action);
    while (action[index]) {
      printf("%s", action[index]);
      /*Makes sure it's not the last word in action*/
      if (action[index+1] != NULL){
	printf(" ");
      } 
      free(action[index]);
      index++;
    }
    /* Freeing split*/
    free(action);
    printf("\n");
  }
  return;
}

/*This function print_fakefile takes
  in one parameter. The goal of this 
  function is to simply print all
  of the components of the fakefile.
  If the fakefile is not NULL, it
  prints all of the components of the 
  fakefile. Otherwise, it simply 
  returns. This function calls on
  print_action() to print the action.*/

void print_fakefile(Fakefile *const fakefile) {
  Rules *curr_rule = fakefile->rules;
  Dependencies *curr_dep;

  if (fakefile != NULL) {
    /*Loops through the rules.*/
    while (curr_rule != NULL) {
      /*Prints the current rule name.*/
      printf("%s: ", curr_rule->tar_name);
      curr_dep = curr_rule->deps;
      /*Loops through dependencies.*/
      while (curr_dep != NULL) {
	/*Prints the curr_dep name.*/
	printf("%s", curr_dep->dep_word);
	/*Adds a space between each dependency
	  as long as it is not the last one.*/
	if (curr_dep->next != NULL) {
	  printf(" ");
	} else {
	  printf("\n");
	}
	curr_dep = curr_dep->next;
      }
      /*Prints action.*/
      printf("\t");
      print_action(fakefile,curr_rule->r_index);
      /*Prints a new line as long as it is not
	the last rule.*/
      if (curr_rule->r_index < fakefile->num_rules-1) {
	printf("\n");
      }
      curr_rule = curr_rule->next;
    }
  }
  return;
}

/*The function num_dependencies takes in
  two parameters. The goal of the function
  is to get the number of dependencies in
  the rule at rule index rule_num
  (second parameter). The way this function
  does that is by looping through the 
  fakefile's (first parameter) rules.
  If the fakefile is not NULL and
  the rule_num valid, then it returns the 
  rule's number of dependencies. 
  Otherwise, it returns -1.*/

int num_dependencies(Fakefile *const fakefile, int rule_num) {
  Rules *curr = fakefile->rules;

  if (fakefile != NULL && 
      (rule_num < fakefile->num_rules && rule_num >=0)) {

    /*Finds the rule*/
    while (curr->next != NULL && curr->r_index != rule_num) {
      curr = curr->next;
    }

    return curr->num_deps;
  } 
  return -1;
}

/*The function get_dependency takes in
  three parameters. The goal of the function
  is to get the name of the dependency in
  the rule at rule index rule_num
  (second parameter) and index at 
  dependency index dependency_num
. (third parameter). The way this function
  does that is by looping through the 
  fakefile's (first parameter) rules and
  then looping through those rules' 
  dependencies. If the fakefile is not NULL,
  the rule_num valid, and the dependency_num
  is valid then it returns the 
  dependencies' name at that index. 
  Otherwise, it returns NULL.*/

char *get_dependency(Fakefile *const fakefile, int rule_num,
		     int dependency_num){
  if (fakefile != NULL && 
      (rule_num < fakefile->num_rules && rule_num >=0)) {
    Rules *curr_rule = fakefile->rules;
    Dependencies *curr_dep;
    /*Finds rule*/
    while (curr_rule->next != NULL && curr_rule->r_index != rule_num) {
      curr_rule = curr_rule->next;
    }
    if (dependency_num < 0 || dependency_num > (curr_rule->num_deps -1)) {
      return NULL;
    }
    curr_dep = curr_rule->deps;
    /*Finds dependency name*/
    while (curr_dep->next != NULL && curr_dep->d_index != dependency_num) {
      curr_dep = curr_dep->next;
    }
    return curr_dep->dep_word;
  }
  return NULL;
}
/*This function do_action takes in
  two parameters. The goal is to perform
  the command in the action line of the
  rule at rule index rule_num(second parameter).
  If fakefile is not NULL and the rule_num is valid
  then the function performs the command and
  returns the exit status. Otherwise, if 
  the fakefile(first parameter) is NULL, the
  rule_num is invalid, or the command did not
  exit normally, then it returns -1.
*/
int do_action(Fakefile *const fakefile, int rule_num){
  pid_t id;
  Rules *curr = fakefile->rules;
  char **action;
  int status = 0;
  if (fakefile != NULL && rule_num < fakefile->num_rules && rule_num >= 0) {
    /*Finds rule*/
    while (curr->next != NULL && curr->r_index != rule_num) {
      curr = curr->next;
    }

    action = split(curr->action);
    id = safe_fork();

    if (id > 0) {
      wait(&status);
      if (WIFEXITED(status)) {
	return WEXITSTATUS(status);
      } else {
	return -1;
      }
    } else if (id == 0){

      execvp(action[0], action);

    } else {
      return -1;
    }
  }
  return -1;
}
/*This recursive helper function removedeps takes in
  two parameters.The goal of this function is to 
  loop through the file freeing all the memory 
  allocated for making dependencies.
  The first parameter is the fakefile we remove
  from and the curr_dep is the current
  dependency we are on. It does not return anything.
*/
void removedeps(Fakefile **const fakefile, Dependencies *curr_dep) {
  if (curr_dep == NULL) {
    return;
  }

  removedeps(fakefile, curr_dep->next);
  free(curr_dep->dep_word);
  curr_dep->next = NULL;
  free(curr_dep);
  return;
}

/*This recursive helper function removerules takes in
  two parameters.The goal of this function is to 
  loop through the file freeing all the memory 
  allocated for making rules.
  The first parameter is the fakefile we remove
  from and the curr_rule is the current
  rule we are on. It returns 1 when it
  is finished freeing the current rule.
  It first removes all of the dependencies of the 
  current rule and then the actual rules.
*/
int removerules(Fakefile **const fakefile, Rules *curr_rule) {
  /*Base Case*/
  if (curr_rule == NULL) {
    return 1;
  } 

  removedeps(fakefile, curr_rule->deps);
  removerules(fakefile, curr_rule->next);
  free(curr_rule->action);
  free(curr_rule->tar_name);
  curr_rule->next = NULL;
  free(curr_rule);
  return 1;
}
/*This function clear_fakefile take in one
  parameter. The goal of the function is 
  to deallocate all of the memory associated 
  with the fakefile(parameter). 
  As long as the fakefile parameter is
  not NULL if removes eveyrthing from the
  fakefile.*/
void clear_fakefile(Fakefile **const fakefile) {
  if (fakefile != NULL && *fakefile != NULL) {
    removerules(fakefile,(*fakefile)->rules);
    (*fakefile)->rules = NULL;
    free(*fakefile);
    *fakefile = NULL;
  }
}
