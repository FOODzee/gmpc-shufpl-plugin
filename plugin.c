/* gmpc-shufpl (GMPC plugin)
 * Copyright (C) 2006-2009 Qball Cow <qball@sarine.nl>
 * Project homepage: http://gmpcwiki.sarine.nl/
 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <gtk/gtk.h>
#include <gmpc/plugin.h>
#include <glade/glade.h>
#include <libmpd/libmpd-internal.h>
#include <gmpc/misc.h>

void shufpl_set_enabled(int enabled);
int shufpl_get_enabled(void);

int shufpl_tool_menu(GtkWidget *menu);

extern GtkTreeModel *playlist;
/* Allow gmpc to check the version the plugin is compiled against */
int plugin_api_version = PLUGIN_API_VERSION;

/** 
 * Define the plugin structure
 */
gmpcPlugin plugin = {
	/* name */
	.name = "Playlist shuffler",
	/* version */
	.version = {0,20,0},
	/* type */
	.plugin_type = GMPC_PLUGIN_NO_GUI,
	/** enable/disable */
	.get_enabled = shufpl_get_enabled,
	.set_enabled = shufpl_set_enabled,

    .tool_menu_integration = shufpl_tool_menu
};

GList *items = NULL;
GtkListStore *tag_list = NULL;
typedef struct{
    GtkWidget *combo;
    GtkWidget *box;
    GtkWidget *button;
}Item;





int shufpl_get_enabled(void)
{
	return cfg_get_single_value_as_int_with_default(config, "shufpl", "enable", TRUE);
}
void shufpl_set_enabled(int enabled)
{
	cfg_set_single_value_as_int(config, "shufpl", "enable", enabled);
	pl3_tool_menu_update(); 
}
/********************************************************
 * This code is copy-pasted from playlistsort-plugin,
 * which I tottally use for my own
 ********************************************************/
int playlistsort_sort(gpointer aa, gpointer bb, gpointer data)
{
    MpdData_real *a = *(MpdData_real **)aa;
    MpdData_real *b = *(MpdData_real **)bb;
    int retv =0;
    int i;
	int *field = ((gint *)(data));
    /** Filter */
	if(!a && !b) return 0;
	if(!a) return -1;
	if(!b) return 1;

    for(i=0;field[i] != -1;i++)
    {
        int do_int = 0;
        char *sa= NULL, *sb = NULL;
        switch(field[i])
        {
            
            case MPD_TAG_ITEM_TITLE:
                sa = a->song->title;
                sb = b->song->title;
                break;
            case MPD_TAG_ITEM_ARTIST:
                sa = a->song->artist;
                sb = b->song->artist;
                break;
            case MPD_TAG_ITEM_ALBUM:
                sa = a->song->album;
                sb = b->song->album;
                break;
            case MPD_TAG_ITEM_GENRE:
                sa = a->song->genre;
                sb = b->song->genre;
                break;
            case MPD_TAG_ITEM_TRACK:

                sa = a->song->track;
                sb = b->song->track;
                do_int = 1;
                break;
            case MPD_TAG_ITEM_NAME:
                sa = a->song->name;
                sb = b->song->name;
                break;
            case MPD_TAG_ITEM_COMPOSER:
                sa = a->song->composer;
                sb = b->song->composer;
                break;
            case MPD_TAG_ITEM_PERFORMER:
                sa = a->song->performer;
                sb = b->song->performer;
                break;
            case MPD_TAG_ITEM_COMMENT:
                sa = a->song->comment;
                sb = b->song->comment;
                break;
            case MPD_TAG_ITEM_DISC:
                sa = a->song->disc;
                sb = b->song->disc;
                break;
            case MPD_TAG_ITEM_DATE:
                sa = a->song->date;
                sb = b->song->date;
                break;
            case MPD_TAG_ITEM_FILENAME:
                sa = a->song->file;
                sb = b->song->file;
                break;
            default:
                g_assert(FALSE);
                return 0;
        }
        if(sa && sb){

            if(do_int == 0)
            {
                gchar *aa,*bb;
                aa = g_utf8_strdown(sa, -1);
                bb = g_utf8_strdown(sb, -1);
                retv = g_utf8_collate(aa,bb);
                g_free(aa);
                g_free(bb);
            }else{
                gint64 aa,bb;
                aa = g_ascii_strtoll(sa, NULL, 10);
                bb = g_ascii_strtoll(sb, NULL, 10);
                retv = (gint)aa-bb;
            }
        }
        else if(sa == sb) retv = 0;
        else if (sa == NULL) retv  = -1;
        else retv = 1;

        if(retv != 0)
        {
            return retv;
        }
    }
    return 0;
}

/***************************************************************
 * This function is differ from it's original to meet my needs
 ***************************************************************/
void playlistsort_start_field()
{
  GList *node, *list=NULL;	
  GtkTreeIter iter; 
  int i;
  int fields[4];
  GList *a;
  MpdData *data2,*data = mpd_playlist_get_changes(connection, -1);

  fields[0] = MPD_TAG_ITEM_ALBUM;
  fields[1] = MPD_TAG_ITEM_DISC;
  fields[2] = MPD_TAG_ITEM_TRACK;
  fields[3] = -1;
  data2 = misc_sort_mpddata(data,(GCompareDataFunc)playlistsort_sort,(gpointer)fields);

  i=0;
  for(data = mpd_data_get_first(data); data;data = mpd_data_get_next(data)){
    mpd_playlist_move_id(connection, data->song->id, i);
    i++;
  }

  mpd_playlist_queue_commit(connection);
}

/********************************************************
 * Shuffling by track
 ********************************************************/
void shufpl_bytrack()
{
  mpd_playlist_shuffle(connection);
}

/********************************************************
 * Shuffling by album
 ********************************************************/
gint id = 0;

void shufpl_submit_album(gpointer data, gpointer user_data){
  mpd_playlist_move_id(connection, ((mpd_Song *)data)->id, id++);
}

void shufpl_commit_albums(gpointer data, gpointer user_data){
  g_ptr_array_foreach((GPtrArray *)data, shufpl_submit_album, NULL);
}

void shufpl_album_free(gpointer data){
  g_ptr_array_free((GPtrArray*) data, TRUE);
}

void shufpl_byalbum(){
  // Before shuffling we need to sort queue by album-disk-track
  // 'cos after that order of songs in album will be correct.

    playlistsort_start_field();

  /// P.S. Of course, I tried to sort each album just before commiting
  /// but for some unknown reason that just don't work.
  /// May be you can figure out why and correct that?

  MpdData * data =  mpd_playlist_get_changes(connection, -1);    // List with songs in current playlist
  GData * albums_list = NULL;                                    // Stores pointers to all founded albums, accessible by album_names
  g_datalist_init(&albums_list);
  GPtrArray * albums_arr = g_ptr_array_new();                    // Stores pointers to all founded albums, accessible by indexes
  GPtrArray * shuffled_albums_arr = g_ptr_array_new();           // Stores result of shuffling albums from albums_arr
  GPtrArray * album;                                             // Stores tracks of a album
  gchar * album_name = NULL;

  // Getting list of albums in current playlist
  for(data = mpd_data_get_first(data); data;data = mpd_data_get_next(data)){
    g_free(album_name);
    album_name = g_utf8_strdown(data->song->album, -1);
    if (!(album = g_datalist_get_data(&albums_list, album_name))) // if we have no such album in list yet
    {
      album = g_ptr_array_new();
      g_ptr_array_add(album, data->song);
      g_ptr_array_add(albums_arr, album);
      g_datalist_set_data(&albums_list, album_name, album);
    } else {
      g_ptr_array_add(album, data->song);
    }
  }

  // Current album should be on top of queue
  mpd_Song * song = mpd_playlist_get_current_song(connection);
  if (song)
    album = g_datalist_get_data(&albums_list, g_utf8_strdown(song->album, -1));
  if(album){
    g_ptr_array_add(shuffled_albums_arr, album);
    g_ptr_array_remove_fast(albums_arr, album);
  }

  // Other albums should be shuffled
  gint i, from, len = albums_arr->len;
  for (i = 0; i<len; i++){
    from = g_random_int_range(0, albums_arr->len);
    g_ptr_array_add(shuffled_albums_arr, g_ptr_array_index(albums_arr, from));
    g_ptr_array_remove_index_fast(albums_arr, from);
  }

  // Submitting tracks
  id = 0;
  g_ptr_array_foreach(shuffled_albums_arr, shufpl_commit_albums, NULL);
  mpd_playlist_queue_commit(connection);

  // Freeing memory
  g_ptr_array_set_free_func(shuffled_albums_arr, shufpl_album_free);
  g_ptr_array_free(shuffled_albums_arr, TRUE);
  g_datalist_clear(&albums_list);
  mpd_data_free(data);
}

/// TODO in v. 0.3.0:
/********************************************************
 * Shuffling by artist
 ********************************************************/
void shufpl_byartist(){}

int shufpl_tool_menu(GtkWidget *menu)
{
    GtkWidget *item;
    GtkWidget *submenu;
    if(!shufpl_get_enabled()) return;

    submenu = gtk_menu_new();
       GtkWidget *track, *album, *artist;
       track = gtk_menu_item_new_with_label("by track");
       album = gtk_menu_item_new_with_label("by album");
       //artist = gtk_menu_item_new_with_label("by artist");
       gtk_menu_append(GTK_MENU(submenu), track);
       gtk_menu_append(GTK_MENU(submenu), album);
       //gtk_menu_append(GTK_MENU(submenu), artist); 
       g_signal_connect(G_OBJECT(track), "activate", G_CALLBACK(shufpl_bytrack), NULL);    
       g_signal_connect(G_OBJECT(album), "activate", G_CALLBACK(shufpl_byalbum), NULL);    
       //g_signal_connect(G_OBJECT(artist), "activate", G_CALLBACK(shufpl_byartist), NULL);   

    item = gtk_menu_item_new_with_label("Shuffle playlist");
    gtk_menu_item_set_submenu(item, submenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    return 1;
}


