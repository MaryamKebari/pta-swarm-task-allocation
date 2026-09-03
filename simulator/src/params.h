/**** Copyright 1998, Annie S. Wu, All Rights Reserved ****/

/* params.h
   14.10.15.AW	Revised for FLAIRS code.
   07.03.98.AW 	Created.
*/

/* prototypes */
int read_defaults();
int read_params(char *params_file);
int get_next_line(FILE *fp, char *aline);
int set_param(char *aline);
int read_default_opfiles(char *opfiles_file);
int read_opfiles(char *opfiles_file);
int get_file_pointer(char *extension);
int setup_output_files();
int file_on(char *extension);

