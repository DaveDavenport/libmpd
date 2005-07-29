#include <stdio.h>
#include <gtk/gtk.h>
#include <libmpd/libmpdclient.h>
#include <libmpd/libmpd.h>
#include "egg-background-monitor.h"
#include "strfsong.h"

#define DEFAULT_TRAY_MARKUP "[<span size=\"small\">%name%</span>\n][<span size=\"large\">%title%</span>\n][%artist%][\n<span size=\"small\">%album% [(track %track%)]</span>]|%shortfile%|"


EggBackgroundMonitor *egm = NULL;
PangoLayout *tray_layout_tooltip = NULL;

GtkWidget *window = NULL;

MpdObj *mi = NULL;
int width, height;

int song_changed()
{
	gchar result[1024];
	int id;
	GString *string = g_string_new("");
	mpd_ob_status_update(mi);
	mpd_Song *song = mpd_ob_playlist_get_current_song(mi);
	strfsong(result, 1024, DEFAULT_TRAY_MARKUP, song);            	
	g_string_append(string, result);

	if(mpd_ob_status_get_total_song_time(mi) > 0)
	{
		g_string_append_printf(string, "\n<span size=\"small\">Time:\t%02i:%02i/%02i:%02i</span>",
				mpd_ob_status_get_elapsed_song_time(mi)/60, mpd_ob_status_get_elapsed_song_time(mi) %60,
				mpd_ob_status_get_total_song_time(mi)/60, mpd_ob_status_get_total_song_time(mi) %60);
	}
	else
	{
		g_string_append_printf(string, "\n<span size=\"small\">Time:\t%02i:%02i</span>",
				mpd_ob_status_get_elapsed_song_time(mi)/60, mpd_ob_status_get_elapsed_song_time(mi) %60);
	}

	for(id=0;id < string->len; id++)
	{
		if(string->str[id] == '&')
		{
			g_string_insert(string, id+1, "amp;");
			id++;
		}
	}
	pango_layout_set_markup(tray_layout_tooltip,string->str, string->len);	
	pango_layout_get_size(tray_layout_tooltip, &width, &height);
	gtk_window_resize(GTK_WINDOW(window), PANGO_PIXELS(width)+10, PANGO_PIXELS(height)+10+8);	
	g_string_free(string, TRUE);
	return TRUE;
}

int expose()
{
	int x,y;
	GdkPixbuf *bg = NULL;
	gdk_window_get_origin (window->window, &x, &y);
	bg = egg_background_monitor_get_region (egm, x+window->allocation.x, y+window->allocation.y, 
			PANGO_PIXELS(width)+10,PANGO_PIXELS(height)+10+8);
	gdk_draw_pixbuf(window->window, window->style->fg_gc[GTK_STATE_NORMAL],bg,
			0,0,0,0,-1,-1,GDK_RGB_DITHER_NONE,0,0);
	gdk_draw_rectangle(window->window,window->style->fg_gc[GTK_STATE_NORMAL],FALSE,
			0,0,PANGO_PIXELS(width)+7 , PANGO_PIXELS(height)+10+6);
	gdk_draw_rectangle(window->window,window->style->fg_gc[GTK_STATE_SELECTED],FALSE,
			1,1,PANGO_PIXELS(width)+7 , PANGO_PIXELS(height)+10+6);    	
	gtk_paint_layout (window->style, window->window, GTK_STATE_NORMAL, TRUE,
			NULL, window, "tooltip", 4, 3, tray_layout_tooltip);
	gtk_paint_layout (window->style, window->window, GTK_STATE_SELECTED, TRUE,
			NULL, window, "tooltip", 5, 4, tray_layout_tooltip);



	if(mpd_ob_status_get_total_song_time(mi) > 0)
	{

		int width2 = 0;
		gdk_draw_rectangle(window->window, window->style->fg_gc[GTK_STATE_NORMAL],FALSE,4,PANGO_PIXELS(height)+5, PANGO_PIXELS(width) ,8);
		gdk_draw_rectangle(window->window, window->style->fg_gc[GTK_STATE_SELECTED],FALSE,5,PANGO_PIXELS(height)+6, PANGO_PIXELS(width),8);
		width2 = (mpd_ob_status_get_elapsed_song_time(mi)/(float)mpd_ob_status_get_total_song_time(mi))*PANGO_PIXELS(width);
		gdk_draw_rectangle(window->window, window->style->mid_gc[GTK_STATE_NORMAL],TRUE,6,PANGO_PIXELS(height)+7, width2,7);
	}
	gdk_pixbuf_unref(bg);
	return TRUE;
}


int main(int argc, char **argv)
{
	gchar *fname = NULL;
	int xpos=-1, ypos=-1;
	/* make libmpd object */	
	mi = mpd_ob_new_default();

	/* check evn */
	if(g_getenv("MPD_PORT") != NULL)
	{
		mpd_ob_set_port(mi,atoi(g_getenv("MPD_PORT")));
	}

	if(g_getenv("MPD_HOST") != NULL)
	{
		mpd_ob_set_hostname(mi,(char *)g_getenv("MPD_HOST"));
	}

	fname = g_strdup_printf("%s/.mpdesklet.conf", g_getenv("HOME"));
	if(g_file_test(fname, G_FILE_TEST_EXISTS))
	{
		gchar *content = NULL;
		int length = 0;
		if(g_file_get_contents(fname,&content,&length,NULL))
		{
			gchar **tokens = NULL;
			tokens = g_strsplit(content,"\n", -1);
			if(tokens != NULL)
			{
				int i=0;
				do{
					if(!strncmp(tokens[i], "hostname=", 9))
					{
						g_print("hostname: %s\n", &tokens[i][9]);	
						mpd_ob_set_hostname(mi, &tokens[i][9]);
					}
					else if(!strncmp(tokens[i], "port=", 5))
					{
						g_print("port: %s\n", &tokens[i][5]);	
						mpd_ob_set_port(mi, atoi(&tokens[i][5]));
					}
					else if(!strncmp(tokens[i], "xpos=", 5))
                                	{
						g_print("xpos: %s\n", &tokens[i][5]);		
						xpos =  atoi(&tokens[i][5]);	
					}                                                	
					else if(!strncmp(tokens[i], "ypos=", 5))
					{
						g_print("ypos: %s\n", &tokens[i][5]);		
						ypos =  atoi(&tokens[i][5]);	
					}                                                						
					i++;
				}while(tokens[i] != NULL);
				g_strfreev(tokens);
			}
		}
	}
	g_free(fname);

	/* connect */
	mpd_ob_connect(mi);
	if(!mpd_ob_check_connected(mi))
	{
		printf("Failed to connect\n");
		return 0;
	}

	/* gtk init */
	gtk_init(&argc, &argv);
	/* create background monitor */
	egm = egg_background_monitor_new();

	/* create the window */
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	/* remove decoration */
	gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
	/* set it below */
	gtk_window_set_keep_below(GTK_WINDOW(window), TRUE);
	gtk_window_stick(GTK_WINDOW(window));
	gtk_window_set_skip_pager_hint(GTK_WINDOW(window), TRUE);
	gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window), TRUE);

	if(xpos > 0 && ypos > 0)
	{
		gtk_window_move(GTK_WINDOW(window), xpos,ypos);
	}

	
	/* make it paintable */
	gtk_widget_set_app_paintable(window, TRUE);

	tray_layout_tooltip = gtk_widget_create_pango_layout (window, NULL);

	song_changed();

	g_signal_connect(G_OBJECT(window), "expose-event", G_CALLBACK(expose), NULL);
	g_signal_connect(G_OBJECT(egm), "changed", G_CALLBACK(expose), NULL);
	g_signal_connect (G_OBJECT (window), "configure-event",
			(GCallback) expose,NULL);    	

	gtk_widget_show_all(window);

	g_timeout_add(500, song_changed, NULL);
	gtk_main();
	return 0;
}

