#ifndef _CONF_H_
#define _CONF_H_

#include <gctypes.h>

typedef enum CF_TYPE{
    CFT_INT=0,
    CFT_STRING,
    CFT_BOOLEAN,
    CFT_ARRAY,
    CFT_ACTION,
    CFT_ACTION_ARG,
}CF_TYPE;

typedef struct CONF_ITEM {
    char *name;
    int short_opt;
    char *help;
    int (*action)(struct CONF_ITEM *self); /* if defined, the option is 
											  available only on the command line */
	bool modified;
    CF_TYPE type;
    union {
	struct {
	    int val;
	    int default_val;
	}dt_int;
	struct {
	    int bool;
	    int default_bool;
	}dt_bool;
	struct {
	    char str[255];
	    char *default_str;
	}dt_str;
	struct {
	    int size;
	    int *array;
	    int *default_array;
	}dt_array;
    }data;
}CONF_ITEM;

#define CF_BOOL(t) t->data.dt_bool.bool
#define CF_VAL(t) t->data.dt_int.val
#define CF_STR(t) t->data.dt_str.str
#define CF_ARRAY(t) t->data.dt_array.array
#define CF_ARRAY_SIZE(t) t->data.dt_array.size


CONF_ITEM* cf_get_item_by_name(char *name);
void cf_create_bool_item(char *name,char *help,char short_opt,bool def);
void cf_create_action_item(char *name,char *help,char short_opt,int (*action)(struct CONF_ITEM *self));
void cf_create_action_arg_item(char *name,char *help,char short_opt,int (*action)(struct CONF_ITEM *self));
void cf_create_string_item(char *name,char *help,char short_opt,char *def);
void cf_create_int_item(char *name,char *help,char short_opt,int def);
void cf_create_array_item(char *name,char *help,char short_opt,int size,int *def);
void cf_init(void);
bool cf_save_file(char *filename,int flags);
bool cf_open_file(char *filename);
void cf_init_cmd_line(void);
int cf_get_non_opt_index(int argc, char *argv[]);
char* cf_parse_cmd_line(int argc, char *argv[]);
void cf_print_help(void);

#endif
