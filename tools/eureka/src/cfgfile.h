/*
 *  cfgfile.h
 *  AYM 1998-09-30
 */


int parse_config_file_default ();
int parse_config_file_user (const char *name);
int  parse_command_line_options (int argc, const char *const *argv, int pass);
void dump_parameters (FILE *fd);
void dump_command_line_options (FILE *fd);
extern const char config_file_magic[];


