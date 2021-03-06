/*
 * DO NOT EDIT THIS FILE - it is generated by Glade.
 */

#define OPEN_3D_OBJ 1
#define OPEN_2D_OBJ 2
#define OPEN_MAP    3
#define SAVE_MAP    4
#define OPEN_PARTICLES_OBJ 5
#define SAVE_PARTICLE_DEF 6
#define OPEN_EYE_CANDY_OBJ 7

extern char* selected_file;

#include <gtk/gtk.h>

#ifdef GTK2

extern char map_file_name[256];
extern char particle_file_name[256];
extern char map_folder[256];
extern char obj_2d_folder[256];
extern char obj_3d_folder[256];
extern char particles_folder[256];

extern GtkWidget * gtk_effect_win;
extern GtkWidget * gtk_effect_list_box;
extern GtkWidget * gtk_effect_hue_box;
extern GtkWidget * gtk_effect_saturation_box;
extern GtkWidget * gtk_effect_scale_box;
extern GtkWidget * gtk_effect_density_box;
extern GtkWidget * gtk_effect_base_height_box;
extern GtkWidget * gtk_effect_list;
extern GtkWidget * gtk_effect_hue;
extern GtkWidget * gtk_effect_saturation;
extern GtkWidget * gtk_effect_scale;
extern GtkWidget * gtk_effect_density;
extern GtkObject * gtk_effect_hue_obj;
extern GtkObject * gtk_effect_saturation_obj;
extern GtkObject * gtk_effect_scale_obj;
extern GtkObject * gtk_effect_density_obj;
extern GtkWidget * gtk_effect_base_height;
extern GtkFileFilter * e3d_filter;
extern GtkFileFilter * e2d_filter;
extern GtkFileFilter * map_filter;
extern GtkFileFilter * part_filter;

void init_filters();
void show_open_window(char * name, char * folder, GtkFileFilter * filter);
void show_save_window(char * name, char * folder, char * select, GtkFileFilter *filter);
void show_eye_candy_window();
#else
extern int continue_with;
extern GtkWidget* file_selector;
extern GtkWidget* effect_selector;
extern GtkWidget* create_fileselection (void);
#endif
