#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <curl/curl.h>
#include <memory.h>
#include <string.h>
#include <libintl.h>

#ifdef _LIBINTL_H
#include <locale.h>
# define _(x) gettext(x)
#else
# define _(x) x
#endif

#ifdef _WIN32
# define DATADIR "data"
# define LOCALEDIR "share/locale"
# ifndef snprintf
#  define snprintf _snprintf
# endif
#endif

#define APP_TITLE _("GtkTwitter")
#define APP_NAME _("gtktwitter")
#define TWITTER_UPDATE_URL "http://twitter.com/statuses/update.xml"
#define TWITTER_STATUS_URL "http://twitter.com/statuses/friends_timeline.xml"
#define ACCEPT_LETTER_URL  "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789;/?:@&=+$,-_.!~*'()%"
#define ACCEPT_LETTER_NAME "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_"

#define GET_CONTENT(x) (x->children ? x->children->content : NULL)

static GdkCursor *hand_cursor = NULL;
static GdkCursor *regular_cursor = NULL;

typedef struct _PIXBUF_CACHE {
	char* id;
	GdkPixbuf* pixbuf;
} PIXBUF_CACHE;

typedef struct _PROCESS_THREAD_INFO {
	GThreadFunc func;
	gboolean processing;
	gpointer data;
	gpointer retval;
} PROCESS_THREAD_INFO;

/**
 * curl callback
 */
static char* response_mime = NULL;	/* response content-type. ex: "text/html" */
static char* response_data = NULL;	/* response data from server. */
static size_t response_size = 0;	/* response size of data */

static size_t handle_returned_data(char* ptr, size_t size, size_t nmemb, void* stream) {
	if (!response_data)
		response_data = (char*)malloc(size*nmemb);
	else
		response_data = (char*)realloc(response_data, response_size+size*nmemb);
	if (response_data) {
		memcpy(response_data+response_size, ptr, size*nmemb);
		response_size += size*nmemb;
	}
	return size*nmemb;
}

static size_t handle_returned_header(void* ptr, size_t size, size_t nmemb, void* stream) {
	char* header = NULL;

	header = malloc(size*nmemb + 1);
	memcpy(header, ptr, size*nmemb);
	header[size*nmemb] = 0;
	if (strncmp(header, "Content-Type: ", 14) == 0) {
		char* stop = header + 14;
		stop = strpbrk(header + 14, "\r\n;");
		if (stop) *stop = 0;
		response_mime = strdup(header + 14);
	}
	free(header);
	return size*nmemb;
}

/**
 * string utilities
 */
static char* xml_decode_alloc(char* str) {
	char* buf = NULL;
	unsigned char* pbuf = NULL;
	int len = 0;

	if (!str) return NULL;
	len = strlen(str)*3;
	buf = malloc(len+1);
	memset(buf, 0, len+1);
	pbuf = (unsigned char*)buf;
	while(*str) {
		if (!memcmp(str, "&amp;", 5)) {
			strcat((char*)pbuf++, "&");
			str += 5;
		} else
		if (!memcmp(str, "&nbsp;", 6)) {
			strcat((char*)pbuf++, " ");
			str += 6;
		} else
		if (!memcmp(str, "&quot;", 6)) {
			strcat((char*)pbuf++, "\"");
			str += 6;
		} else
		if (!memcmp(str, "&nbsp;", 6)) {
			strcat((char*)pbuf++, " ");
			str += 6;
		} else
		if (!memcmp(str, "&lt;", 4)) {
			strcat((char*)pbuf++, "<");
			str += 4;
		} else
		if (!memcmp(str, "&gt;", 4)) {
			strcat((char*)pbuf++, ">");
			str += 4;
		} else
			*pbuf++ = *str++;
	}
	return buf;
}

static char* url_encode_alloc(char* str) {
	static const int force_encode_all = TRUE;
	const char* hex = "0123456789abcdef";

	char* buf = NULL;
	unsigned char* pbuf = NULL;
	int len = 0;

	if (!str) return NULL;
	len = strlen(str)*3;
	buf = malloc(len+1);
	memset(buf, 0, len+1);
	pbuf = (unsigned char*)buf;
	while(*str) {
		unsigned char c = (unsigned char)*str;
		if (c == ' ')
			*pbuf++ = '+';
		else if (c & 0x80 || force_encode_all) {
			*pbuf++ = '%';
			*pbuf++ = hex[c >> 4];
			*pbuf++ = hex[c & 0x0f];
		} else
			*pbuf++ = c;
		str++;
	}
	return buf;
}

/**
 * loading icon
 */
static GdkPixbuf* url2pixbuf(const char* url, GError** error) {
	GdkPixbuf* pixbuf = NULL;
	GdkPixbufLoader* loader = NULL;
	GdkPixbufFormat* format = NULL;
	GError* _error = NULL;
	CURL* curl = NULL;
	CURLcode res = CURLE_OK;

	/* initialize callback data */
	response_mime = NULL;
	response_data = NULL;
	response_size = 0;

	if (!strncmp(url, "file:///", 8) || g_file_test(url, G_FILE_TEST_EXISTS)) {
		gchar* newurl = g_filename_from_uri(url, NULL, NULL);
		pixbuf = gdk_pixbuf_new_from_file(newurl ? newurl : url, &_error);
	} else {
		curl = curl_easy_init();
		if (!curl) return NULL;
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, handle_returned_data);
		curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, handle_returned_header);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
		if (res == CURLE_OK) {
			if (response_mime) loader = gdk_pixbuf_loader_new_with_mime_type(response_mime, error);
			if (!loader) loader = gdk_pixbuf_loader_new();
			if (gdk_pixbuf_loader_write(loader, (const guchar*)response_data, response_size, &_error)) {
				pixbuf = gdk_pixbuf_loader_get_pixbuf(loader);
				format = gdk_pixbuf_loader_get_format(loader);
			}
			gdk_pixbuf_loader_close(loader, NULL);
		} else
			_error = g_error_new_literal(G_FILE_ERROR, res, curl_easy_strerror(res));
	}

	/* cleanup callback data */
	if (response_mime) free(response_mime);
	if (response_data) free(response_data);
	response_data = NULL;
	response_mime = NULL;
	response_size = 0;
	if (error && _error) *error = _error;
	return pixbuf;
}

/**
 * processing message funcs
 */
static gpointer process_thread(gpointer data) {
	PROCESS_THREAD_INFO* info = (PROCESS_THREAD_INFO*)data;

	info->retval = info->func(info->data);
	info->processing = FALSE;

	return info->retval;
}

static gpointer process_func(GThreadFunc func, gpointer data, GtkWidget* parent, const gchar* message) {
	GtkWidget* window = gtk_window_new(GTK_WINDOW_POPUP);
	GtkWidget* vbox;
	GtkWidget* image;
	GdkColor color;
	PROCESS_THREAD_INFO info;
	GError *error = NULL;
	GThread* thread;
	GdkCursor* cursor = gdk_cursor_new(GDK_WATCH);

	if (parent)
		parent = gtk_widget_get_toplevel(parent);

	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_widget_hide_on_delete(window);

	vbox = gtk_vbox_new(FALSE, 6);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
	gtk_container_add(GTK_CONTAINER(window), vbox);

	image = gtk_image_new_from_file(DATADIR"/loading.gif");
	if (image) gtk_container_add(GTK_CONTAINER(vbox), image);
	gtk_widget_show_all(vbox);

	if (message) {
		GtkWidget* label = gtk_label_new(message);
		gtk_container_add(GTK_CONTAINER(vbox), label);
	}

	gdk_color_parse("#F0F0F0", &color);
	gtk_widget_modify_bg(window, GTK_STATE_NORMAL, &color);

	gtk_widget_queue_resize(window);

	gtk_window_set_decorated(GTK_WINDOW(window), FALSE);

	if (parent) {
		gtk_window_set_transient_for(
				GTK_WINDOW(window),
				GTK_WINDOW(parent));
	}

	gtk_widget_show_all(window);

	if (parent) {
		gdk_window_set_cursor(parent->window, cursor);
	}
	gdk_window_set_cursor(window->window, cursor);
	gdk_flush();
	gdk_cursor_destroy(cursor);

	gdk_threads_leave();

	info.func = func;
	info.data = data;
	info.retval = NULL;
	info.processing = TRUE;
	thread = g_thread_create(
			process_thread,
			&info,
			TRUE,
			&error);
	while(info.processing) {
		gdk_threads_enter();
		while(gtk_events_pending())
			gtk_main_iteration_do(TRUE);
		gdk_threads_leave();
		g_thread_yield();
	}
	g_thread_join(thread);

	gdk_threads_enter();
	gtk_widget_hide(window);

	if (parent) {
		gdk_window_set_cursor(parent->window, NULL);
	}
	return info.retval;
}

/**
 * dialog message func
 */
void error_dialog(GtkWidget* widget, const char* message) {
	GtkWidget* dialog;
	dialog = gtk_message_dialog_new(
			GTK_WINDOW(gtk_widget_get_toplevel(widget)),
			(GtkDialogFlags)(GTK_DIALOG_MODAL),
			GTK_MESSAGE_WARNING,
			GTK_BUTTONS_CLOSE,
			message);
	gtk_window_set_title(GTK_WINDOW(dialog), APP_TITLE);
	gtk_widget_show(dialog);
	gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_transient_for(
			GTK_WINDOW(dialog),
			GTK_WINDOW(gtk_widget_get_toplevel(widget)));
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

static void insert_status_text(GtkTextBuffer* buffer, GtkTextIter* iter, const char* status) {
	char* ptr = (char*)status;
	char* last = ptr;
	while(*ptr) {
		if (!strncmp(ptr, "http://", 7) || !strncmp(ptr, "ftp://", 6)) {
			GtkTextTag *tag;
			int len;
			char* link;
			char* tmp;
			gchar* url;

			if (last != ptr)
				gtk_text_buffer_insert(buffer, iter, last, ptr-last);

			tmp = ptr;
			while(*tmp && strchr(ACCEPT_LETTER_URL, *tmp)) tmp++;
			len = (int)(tmp-ptr);
			link = malloc(len+1);
			memset(link, 0, len+1);
			strncpy(link, ptr, len);
			tag = gtk_text_buffer_create_tag(
					buffer,
					NULL, 
					"foreground",
					"blue", 
					"underline",
					PANGO_UNDERLINE_SINGLE, 
					NULL);
			url = g_strdup(link);
			g_object_set_data(G_OBJECT(tag), "url", (gpointer)url);
			gtk_text_buffer_insert_with_tags(buffer, iter, link, -1, tag, NULL);
			free(link);
			ptr = last = tmp;
		} else
		if (*ptr == '@' || !strncmp(ptr, "\xef\xbc\xa0", 3)) {
			GtkTextTag *tag;
			int len;
			char* link;
			char* tmp;
			gchar* url;

			if (last != ptr)
				gtk_text_buffer_insert(buffer, iter, last, ptr-last);

			tmp = ptr + (*ptr == '@' ? 1 : 3);
			while(*tmp && strchr(ACCEPT_LETTER_NAME, *tmp)) tmp++;
			len = (int)(tmp-ptr);
			link = malloc(len+1);
			memset(link, 0, len+1);
			strncpy(link, ptr, len);
			url = g_strdup_printf("http://twitter.com/%s", link + (*ptr == '@' ? 1 : 3));
			tag = gtk_text_buffer_create_tag(
					buffer,
					NULL, 
					"foreground",
					"blue", 
					"underline",
					PANGO_UNDERLINE_SINGLE, 
					NULL);
			g_object_set_data(G_OBJECT(tag), "url", (gpointer)url);
			gtk_text_buffer_insert_with_tags(buffer, iter, link, -1, tag, NULL);
			free(link);
			ptr = last = tmp;
		} else
			ptr++;
	}
	if (last != ptr)
		gtk_text_buffer_insert(buffer, iter, last, ptr-last);
}

/**
 * update friends statuses
 */
static gpointer update_friends_statuses_thread(gpointer data) {
	GtkWidget* window = (GtkWidget*)data;
	GtkTextBuffer* buffer = NULL;
	GtkTextTag* name_tag = NULL;
	GtkTextTag* date_tag = NULL;
	CURL* curl = NULL;
	CURLcode res = CURLE_OK;
	int status = 0;

	char url[2048];
	char auth[512];
	char* recv_data = NULL;
	char* mail = NULL;
	char* pass = NULL;
	int n;
	int length;
	gpointer result_str = NULL;

	xmlDocPtr doc = NULL;
	xmlNodeSetPtr nodes = NULL;
	xmlXPathContextPtr ctx = NULL;
	xmlXPathObjectPtr path = NULL;

	GtkTextIter iter;

	PIXBUF_CACHE* pixbuf_cache = NULL;

	/* making basic auth info */
	gdk_threads_enter();
	mail = (char*)g_object_get_data(G_OBJECT(window), "mail");
	pass = (char*)g_object_get_data(G_OBJECT(window), "pass");
	gdk_threads_leave();

	memset(url, 0, sizeof(url));
	strncpy(url, TWITTER_STATUS_URL, sizeof(url)-1);
	memset(auth, 0, sizeof(auth));
	snprintf(auth, sizeof(auth)-1, "%s:%s", mail, pass);

	/* initialize callback data */
	response_mime = NULL;
	response_data = NULL;
	response_size = 0;

	/* perform http */
	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_USERPWD, auth);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, handle_returned_data);
	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, handle_returned_header);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	res = curl_easy_perform(curl);
	res == CURLE_OK ? curl_easy_getinfo(curl, CURLINFO_HTTP_CODE, &status) : res;
	curl_easy_cleanup(curl);

	if (response_size == 0) {
		result_str = g_strdup("no server response");
		goto leave;
	}
	recv_data = malloc(response_size+1);
	memset(recv_data, 0, response_size+1);
	memcpy(recv_data, response_data, response_size);
	if (status != 200 || (response_mime && strcmp(response_mime, "application/xml"))) {
		/* failed to get xml */
		if (response_data) {
			char* message = xml_decode_alloc(recv_data);
			result_str = g_strdup(message);
			free(message);
		} else
			result_str = g_strdup("unknown server response");
		goto leave;
	}

	/* parse xml */
	doc = xmlParseDoc((xmlChar*)recv_data);
	if (!doc) {
		if (recv_data)
			result_str = g_strdup(recv_data);
		else
			result_str = g_strdup("unknown server response");
		goto leave;
	}

	/* create xpath query */
	ctx = xmlXPathNewContext(doc);
	if (!ctx) {
		result_str = g_strdup("unknown server response");
		goto leave;
	}
	path = xmlXPathEvalExpression((xmlChar*)"/statuses/status", ctx);
	if (!path) {
		result_str = g_strdup("unknown server response");
		goto leave;
	}
	nodes = path->nodesetval;

	gdk_threads_enter();
	buffer = (GtkTextBuffer*)g_object_get_data(G_OBJECT(window), "buffer");
	name_tag = (GtkTextTag*)g_object_get_data(G_OBJECT(buffer), "name_tag");
	date_tag = (GtkTextTag*)g_object_get_data(G_OBJECT(buffer), "date_tag");
	gtk_text_buffer_set_text(buffer, "", 0);
	gtk_text_buffer_get_iter_at_mark(buffer, &iter, gtk_text_buffer_get_insert(buffer));
	gdk_threads_leave();

	/* allocate pixbuf cache buffer */
	length = xmlXPathNodeSetGetLength(nodes);
	pixbuf_cache = malloc(length*sizeof(PIXBUF_CACHE));
	memset(pixbuf_cache, 0, length*sizeof(PIXBUF_CACHE));

	/* make the friends timelines */
	for(n = 0; n < length; n++) {
		char* id = NULL;
		char* icon = NULL;
		char* real = NULL;
		char* name = NULL;
		char* text = NULL;
		char* desc = NULL;
		char* date = NULL;
		GdkPixbuf* pixbuf = NULL;
		int cache;

		/* status nodes */
		xmlNodePtr status = nodes->nodeTab[n];
		if(status->type != XML_ATTRIBUTE_NODE && status->type != XML_ELEMENT_NODE && status->type != XML_CDATA_SECTION_NODE) continue;
		status = status->children;
		while(status) {
			if (!strcmp("created_at", (char*)status->name)) date = (char*)status->children->content;
			if (!strcmp("text", (char*)status->name)) {
				if (status->children) text = (char*)status->children->content;
			}
			/* user nodes */
			if (!strcmp("user", (char*)status->name)) {
				xmlNodePtr user = status->children;
				while(user) {
					if (!strcmp("id", (char*)user->name)) id = GET_CONTENT(user);
					if (!strcmp("name", (char*)user->name)) real = GET_CONTENT(user);
					if (!strcmp("screen_name", (char*)user->name)) name = GET_CONTENT(user);
					if (!strcmp("profile_image_url", (char*)user->name)) icon = GET_CONTENT(user);
					if (!strcmp("description", (char*)user->name)) desc = GET_CONTENT(user);
					user = user->next;
				}
			}
			status = status->next;
		}

		/**
		 * avoid to duplicate downloading of icon.
		 */
		for(cache = 0; cache < length; cache++) {
			if (!pixbuf_cache[cache].id) break;
			if (!strcmp(pixbuf_cache[cache].id, id)) {
				pixbuf = pixbuf_cache[cache].pixbuf;
				break;
			}
		}
		if (!pixbuf) {
			pixbuf = url2pixbuf((char*)icon, NULL);
			if (pixbuf) {
				pixbuf_cache[cache].id = id;
				pixbuf_cache[cache].pixbuf = pixbuf;
				if (desc) g_object_set_data(G_OBJECT(pixbuf), "description", g_strdup(desc));
			}
		}

		/**
		 * layout:
		 *
		 * [icon] [name:name_tag]
		 * [message]
		 * [date:date_tag]
		 *
		 */
		gdk_threads_enter();
		if (pixbuf) gtk_text_buffer_insert_pixbuf(buffer, &iter, pixbuf);
		gtk_text_buffer_insert(buffer, &iter, " ", -1);
		gtk_text_buffer_insert_with_tags(buffer, &iter, name, -1, name_tag, NULL);
		gtk_text_buffer_insert(buffer, &iter, "\n", -1);
		text = xml_decode_alloc(text);
		insert_status_text(buffer, &iter, text);
		//gtk_text_buffer_insert(buffer, &iter, text, -1);
		gtk_text_buffer_insert(buffer, &iter, "\n", -1);
		gtk_text_buffer_insert_with_tags(buffer, &iter, date, -1, date_tag, NULL);
		free(text);
		gtk_text_buffer_insert(buffer, &iter, "\n\n", -1);
		gdk_threads_leave();
	}
	free(pixbuf_cache);

	gdk_threads_enter();
	gtk_text_buffer_set_modified(buffer, FALSE) ;
	gtk_text_buffer_get_start_iter(buffer, &iter);
	gtk_text_buffer_place_cursor(buffer, &iter);
	gdk_threads_leave();

leave:
	/* cleanup callback data */
	if (recv_data) free(recv_data);
	if (path) xmlXPathFreeObject(path);
	if (ctx) xmlXPathFreeContext(ctx);
	if (doc) xmlFreeDoc(doc);
	if (response_mime) free(response_mime);
	if (response_data) free(response_data);
	response_data = NULL;
	response_size = 0;

	return result_str;
}

static void update_friends_statuses(GtkWidget* widget, gpointer user_data) {
	gpointer result;
	GtkWidget* window = (GtkWidget*)user_data;
	GtkWidget* toolbox = (GtkWidget*)g_object_get_data(G_OBJECT(window), "toolbox");
	gtk_widget_set_sensitive(toolbox, FALSE);
	result = process_func(update_friends_statuses_thread, window, window, _("updating statuses..."));
	if (result) {
		/* show error message */
		error_dialog(widget, result);
		g_free(result);
	}
	gtk_widget_set_sensitive(toolbox, TRUE);
}

/**
 * post my status
 */
static gpointer post_status_thread(gpointer data) {
	GtkWidget* window = (GtkWidget*)data;
	GtkWidget* entry = NULL;
	CURL* curl = NULL;
	CURLcode res = CURLE_OK;
	int status = 0;

	char url[2048];
	char auth[512];
	char* message = NULL;
	char* mail = NULL;
	char* pass = NULL;
	gpointer result_str = NULL;

	gdk_threads_enter();
	mail = (char*)g_object_get_data(G_OBJECT(window), "mail");
	pass = (char*)g_object_get_data(G_OBJECT(window), "pass");
	entry = (GtkWidget*)g_object_get_data(G_OBJECT(window), "entry");
	message = (char*)gtk_entry_get_text(GTK_ENTRY(entry));
	gdk_threads_leave();
	if (!message || strlen(message) == 0) return NULL;

	/* making authenticate info */
	memset(url, 0, sizeof(url));
	strncpy(url, TWITTER_UPDATE_URL, sizeof(url)-1);
	message = url_encode_alloc(message);
	if (message) {
		strncat(url, "?status=", sizeof(url)-1);;
		strncat(url, message, sizeof(url)-1);
		free(message);
	}
	memset(auth, 0, sizeof(auth));
	snprintf(auth, sizeof(auth)-1, "%s:%s", mail, pass);

	/* initialize callback data */
	response_mime = NULL;
	response_data = NULL;
	response_size = 0;

	/* perform http */
	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_USERPWD, auth);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, handle_returned_data);
	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, handle_returned_header);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");
	curl_easy_setopt(curl, CURLOPT_POST, 1);
	res = curl_easy_perform(curl);
	res == CURLE_OK ? curl_easy_getinfo(curl, CURLINFO_HTTP_CODE, &status) : res;
	curl_easy_cleanup(curl);

	if (status != 200) {
		/* failed to the post */
		if (response_data) {
			char* message;
			char* recv_data = malloc(response_size+1);
			memset(recv_data, 0, response_size+1);
			memcpy(recv_data, response_data, response_size);
			message = xml_decode_alloc(recv_data);
			result_str = g_strdup(message);
			free(message);
		} else
			result_str = g_strdup("unknown server response");
		goto leave;
	} else {
		/* succeeded to the post */
		gdk_threads_enter();
		gtk_entry_set_text(GTK_ENTRY(entry), "");
		gdk_threads_leave();
	}

leave:
	/* cleanup callback data */
	if (response_mime) free(response_mime);
	if (response_data) free(response_data);
	response_data = NULL;
	response_mime = NULL;
	response_size = 0;
	return result_str;
}

static void post_status(GtkWidget* widget, gpointer user_data) {
	gpointer result;
	GtkWidget* window = (GtkWidget*)user_data;
	GtkWidget* toolbox = (GtkWidget*)g_object_get_data(G_OBJECT(window), "toolbox");
	gtk_widget_set_sensitive(toolbox, FALSE);
	result = process_func(post_status_thread, window, window, _("posting status..."));
	if (!result)
		result = process_func(update_friends_statuses_thread, window, window, _("updating statuses..."));
	if (result) {
		/* show error message */
		error_dialog(widget, result);
		g_free(result);
	}
	gtk_widget_set_sensitive(toolbox, TRUE);
}

/**
 * enter key handler
 */
static gboolean on_entry_keyp_ress(GtkWidget *widget, GdkEventKey* event, gpointer user_data) {
	char* message = (char*)gtk_entry_get_text(GTK_ENTRY(widget));

	if (!message || strlen(message) == 0) return FALSE;
	if (event->keyval == GDK_Return)
		post_status(widget, user_data);
	return FALSE;
}

/**
 * login dialog func
 */
static gboolean login_dialog(GtkWidget* window) {
	GtkWidget* dialog = NULL;
	GtkWidget* table = NULL;
	GtkWidget* label = NULL;
	GtkWidget* mail = NULL;
	GtkWidget* pass = NULL;
	gboolean ret = FALSE;

	/* login dialog */
	dialog = gtk_dialog_new();
	gtk_dialog_add_buttons(GTK_DIALOG(dialog),
			GTK_STOCK_OK, GTK_RESPONSE_OK,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

	gtk_window_set_title(GTK_WINDOW(dialog), _("GtkTwitter Login"));

	/* layout table */
	table = gtk_table_new(2, 2, FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(table), 6);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), table); 

	/* mail */
	label = gtk_label_new(_("_Mail:"));
	gtk_label_set_use_underline(GTK_LABEL(label), TRUE);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
	gtk_table_attach(
			GTK_TABLE(table),
			label,
			0, 1,                   4, 5,
			GTK_FILL,               GTK_FILL,
			0,                      0);
	mail = gtk_entry_new();
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), mail);
	gtk_table_attach(
			GTK_TABLE(table),
			mail,
			1, 2,                   4, 5,
			(GtkAttachOptions)(GTK_EXPAND|GTK_FILL), GTK_FILL,
			0,                      0);

	/* pass */
	label = gtk_label_new(_("_Password:"));
	gtk_label_set_use_underline(GTK_LABEL(label), TRUE);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
	gtk_table_attach(
			GTK_TABLE(table),
			label,
			0, 1,                   5, 6,
			GTK_FILL,               GTK_FILL,
			0,                      0);
	pass = gtk_entry_new();
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), pass);
	gtk_entry_set_visibility(GTK_ENTRY(pass), FALSE);
	gtk_table_attach(
			GTK_TABLE(table),
			pass,
			1, 2,                   5, 6,
			(GtkAttachOptions)(GTK_EXPAND|GTK_FILL), GTK_FILL,
			0,                      0);

	/* show modal dialog */
	gtk_widget_show_all(table);
	gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
	gtk_window_set_transient_for(
			GTK_WINDOW(dialog),
			GTK_WINDOW(window));
	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_widget_show_all(dialog);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
		/* set mail/pass value to window object */
		char* mail_text = (char*)gtk_entry_get_text(GTK_ENTRY(mail));
		char* pass_text = (char*)gtk_entry_get_text(GTK_ENTRY(pass));
		g_object_set_data(G_OBJECT(window), "mail", strdup(mail_text));
		g_object_set_data(G_OBJECT(window), "pass", strdup(pass_text));
		ret = TRUE;
	}

	gtk_widget_destroy(dialog);
	return ret;
}

static void textview_change_cursor(GtkWidget* textview, gint x, gint y) {
	static gboolean hovering_over_link = FALSE;
	GSList *tags = NULL;
	GtkWidget* toplevel;
	GtkTextBuffer *buffer;
	GtkTextIter iter;
	GtkTextTag* name_tag;
	//GtkTooltips* tooltips = NULL;
	gboolean hovering = FALSE;
	int len, n;

	toplevel = gtk_widget_get_toplevel(textview);
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
	gtk_text_view_get_iter_at_location(GTK_TEXT_VIEW(textview), &iter, x, y);

	//tooltips = (GtkTooltips*)g_object_get_data(G_OBJECT(toplevel), "tooltips");

	name_tag = (GtkTextTag*)g_object_get_data(G_OBJECT(buffer), "name_tag");
	if (gtk_text_iter_has_tag(&iter, name_tag)) {
		hovering = TRUE;
	} else {
		tags = gtk_text_iter_get_tags(&iter);
		if (tags) {
			len = g_slist_length(tags);
			for(n = 0; n < len; n++) {
				GtkTextTag* tag = (GtkTextTag*)g_slist_nth_data(tags, n);
				if (tag) {
					gpointer url = g_object_get_data(G_OBJECT(tag), "url");
					if (url) {
						hovering = TRUE;
						break;
					}
				}
			}
			g_slist_free(tags);
		}
	}
	if (hovering != hovering_over_link) {
		hovering_over_link = hovering;
		gdk_window_set_cursor(
				gtk_text_view_get_window(
					GTK_TEXT_VIEW(textview),
					GTK_TEXT_WINDOW_TEXT),
				hovering_over_link ? hand_cursor : regular_cursor);
	}
}

static gboolean textview_event_after(GtkWidget* textview, GdkEvent* ev) {
	GtkTextIter start, end, iter;
	GtkTextBuffer *buffer;
	GtkTextTag* name_tag;
	GdkEventButton *event;
	GSList *tags = NULL;
	gint x, y;
	int len, n;
	gchar* url = NULL;

	if (ev->type != GDK_BUTTON_RELEASE) return FALSE;
	event = (GdkEventButton*)ev;
	if (event->button != 1) return FALSE;

	gdk_threads_enter();
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
	gtk_text_buffer_get_selection_bounds(buffer, &start, &end);
	if (gtk_text_iter_get_offset(&start) != gtk_text_iter_get_offset(&end)) {
		gdk_threads_leave();
		return FALSE;
	}
	gtk_text_view_window_to_buffer_coords(
			GTK_TEXT_VIEW(textview), 
			GTK_TEXT_WINDOW_WIDGET,
			(gint)event->x, (gint)event->y, &x, &y);
	gtk_text_view_get_iter_at_location(GTK_TEXT_VIEW(textview), &iter, x, y);

	name_tag = (GtkTextTag*)g_object_get_data(G_OBJECT(buffer), "name_tag");
	if (gtk_text_iter_has_tag(&iter, name_tag)) {
		GtkTextIter* link_start;
		GtkTextIter* link_end;
		gchar* user_id;
		link_start = gtk_text_iter_copy(&iter);
		link_end = gtk_text_iter_copy(&iter);
		gtk_text_iter_backward_to_tag_toggle(link_start, NULL);
		gtk_text_iter_forward_to_tag_toggle(link_end, NULL);
		user_id = gtk_text_buffer_get_text(buffer, link_start, link_end, TRUE);
		url = g_strdup_printf("http://twitter.com/%s", user_id);
	} else {
		tags = gtk_text_iter_get_tags(&iter);
		if (tags) {
			len = g_slist_length(tags);
			for(n = 0; n < len; n++) {
				GtkTextTag* tag = (GtkTextTag*)g_slist_nth_data(tags, n);
				if (tag) {
					gpointer tag_data = g_object_get_data(G_OBJECT(tag), "url");
					if (tag_data)  {
						url = g_strdup(tag_data);
						break;
					}
				}
			}
			g_slist_free(tags);
		}
	}

	if (url) {
		GtkWidget* toplevel = gtk_widget_get_toplevel(textview);
#ifdef _WIN32
		ShellExecute(NULL, "open", url, NULL, NULL, SW_SHOW);
#else
		gchar* command = g_strdup_printf("firefox \"%s\"", url);
		g_spawn_command_line_async(command, NULL);
		g_free(command);
#endif
		g_free(url);
		gtk_widget_queue_draw(toplevel);
	}
	gdk_threads_leave();
	return FALSE;
}

static gboolean textview_motion(GtkWidget* textview, GdkEventMotion* event) {
	gint x, y;
	gdk_threads_enter();
	gtk_text_view_window_to_buffer_coords(
			GTK_TEXT_VIEW(textview),
			GTK_TEXT_WINDOW_WIDGET,
			(gint)event->x, (gint)event->y, &x, &y);
	textview_change_cursor(textview, x, y);
	gdk_window_get_pointer(textview->window, NULL, NULL, NULL);
	gdk_threads_leave();
	return FALSE;
}

static gboolean textview_visibility(GtkWidget* textview, GdkEventVisibility* event) {
	gint wx, wy, x, y;
	gdk_threads_enter();
	gdk_window_get_pointer(textview->window, &wx, &wy, NULL);
	gtk_text_view_window_to_buffer_coords(
			GTK_TEXT_VIEW(textview),
			GTK_TEXT_WINDOW_WIDGET,
			wx, wy, &x, &y);
	textview_change_cursor(textview, x, y);
	gdk_window_get_pointer(textview->window, NULL, NULL, NULL);
	gdk_threads_leave();
	return FALSE;
}

static void buffer_delete_range(GtkTextBuffer* buffer, GtkTextIter* start, GtkTextIter* end, gpointer user_data) {
	GtkTextIter* iter = gtk_text_iter_copy(end);
	while(TRUE) {
		GSList* tags = NULL;
		GtkTextTag* tag;
		int len, n;
		if (!gtk_text_iter_backward_char(iter)) break;
		if (!gtk_text_iter_in_range(iter, start, end)) break;
		tags = gtk_text_iter_get_tags(iter);
		if (!tags) continue;
		len = g_slist_length(tags);
		for(n = 0; n < len; n++) {
			gpointer tag_data;

			tag = (GtkTextTag*)g_slist_nth_data(tags, n);
			if (!tag) continue;

			tag_data = g_object_get_data(G_OBJECT(tag), "url");
			if (tag_data) g_free(tag_data);
			g_object_set_data(G_OBJECT(tag), "url", NULL);

			tag_data = g_object_get_data(G_OBJECT(tag), "description");
			if (tag_data) g_free(tag_data);
			g_object_set_data(G_OBJECT(tag), "description", NULL);
		}
		g_slist_free(tags);
	}
	gtk_text_iter_free(iter);
}

/**
 * main entry
 */
int main(int argc, char* argv[]) {
	/* widgets */
	GtkWidget* window = NULL;
	GtkWidget* vbox = NULL;
	GtkWidget* hbox = NULL;
	GtkWidget* swin = NULL;
	GtkWidget* textview = NULL;
	GtkWidget* image = NULL;
	GtkWidget* button = NULL;
	GtkWidget* entry = NULL;
	//GtkTooltips* tooltips = NULL;

	GtkTextBuffer* buffer = NULL;
	GtkTextTag* name_tag = NULL;
	GtkTextTag* date_tag = NULL;

#ifdef _LIBINTL_H
	setlocale(LC_CTYPE, "");

#ifdef LOCALE_SISO639LANGNAME
	if (getenv("LANG") == NULL) {
		char szLang[256] = {0};
		if (GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SISO639LANGNAME, szLang, sizeof(szLang))) {
			char szEnv[256] = {0};
			sprintf(szEnv, "LANG=%s", szLang);
			putenv(szEnv);
		}
	}
#endif

	bindtextdomain(APP_NAME, LOCALEDIR);
	bind_textdomain_codeset(APP_NAME, "utf-8");
	textdomain(APP_NAME);
#endif

	g_thread_init(NULL);
	gdk_threads_init();
	gdk_threads_enter();

	gtk_init(&argc, &argv);

	/* main window */
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), APP_TITLE);
	g_signal_connect(G_OBJECT(window), "delete-event", gtk_main_quit, window);

	/* link cursor */
	hand_cursor = gdk_cursor_new(GDK_HAND2);
	regular_cursor = gdk_cursor_new(GDK_XTERM);

	/* tooltips */
	//tooltips = gtk_tooltips_new();
	//gtk_tooltips_disable(tooltips);
	//tooltips = (GtkTooltips*)g_object_get_data(G_OBJECT(window), "tooltips");

	/* virtical container box */
	vbox = gtk_vbox_new(FALSE, 6);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
	gtk_container_add(GTK_CONTAINER(window), vbox);

	/* title logo */
	image = gtk_image_new_from_pixbuf(gdk_pixbuf_new_from_file(DATADIR"/twitter.png", NULL));
	gtk_box_pack_start(GTK_BOX(vbox), image, FALSE, TRUE, 0);

	/* status viewer on scrolled window */
	textview = gtk_text_view_new();
	gtk_text_view_set_editable(GTK_TEXT_VIEW(textview), FALSE);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(textview), FALSE);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textview), GTK_WRAP_CHAR);
	g_signal_connect(textview, "motion-notify-event", G_CALLBACK(textview_motion), NULL);
	g_signal_connect(textview, "visibility-notify-event", G_CALLBACK(textview_visibility), NULL);
	g_signal_connect(textview, "event-after", G_CALLBACK(textview_event_after), NULL);

	swin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(
			GTK_SCROLLED_WINDOW(swin),
			GTK_POLICY_NEVER, 
			GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(swin), textview);
	gtk_container_add(GTK_CONTAINER(vbox), swin);

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
	g_signal_connect(G_OBJECT(buffer), "delete-range", G_CALLBACK(buffer_delete_range), NULL);
	g_object_set_data(G_OBJECT(window), "buffer", buffer);

	/* tags for string attributes */
	name_tag = gtk_text_buffer_create_tag(
			buffer,
			"b",
			"scale",
			PANGO_SCALE_LARGE,
			"underline",
			PANGO_UNDERLINE_SINGLE,
			"weight",
			PANGO_WEIGHT_BOLD,
			"foreground",
			"#0000FF",
			NULL);
	g_object_set_data(G_OBJECT(buffer), "name_tag", name_tag);

	date_tag = gtk_text_buffer_create_tag(
			buffer,
			"small",
			"scale",
			PANGO_SCALE_X_SMALL,
			"style",
			PANGO_STYLE_ITALIC,
			"foreground",
			"#005500",
			NULL);
	g_object_set_data(G_OBJECT(buffer), "date_tag", date_tag);

	/* horizontal container box for buttons and entry */
	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	g_object_set_data(G_OBJECT(window), "toolbox", hbox);

	/* update button */
	button = gtk_button_new();
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(update_friends_statuses), window);
	image = gtk_image_new_from_pixbuf(gdk_pixbuf_new_from_file(DATADIR"/reload.png", NULL));
	//gtk_button_set_image(GTK_BUTTON(button), image);
	gtk_container_add(GTK_CONTAINER(button), image);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, TRUE, 0);

	/* post button */
	button = gtk_button_new();
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(post_status), window);
	image = gtk_image_new_from_pixbuf(gdk_pixbuf_new_from_file(DATADIR"/post.png", NULL));
	//gtk_button_set_image(GTK_BUTTON(button), image);
	gtk_container_add(GTK_CONTAINER(button), image);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, TRUE, 0);

	/* text entry */
	entry = gtk_entry_new();
	g_object_set_data(G_OBJECT(window), "entry", entry);
	g_signal_connect(G_OBJECT(entry), "key-press-event", G_CALLBACK(on_entry_keyp_ress), window);
	gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);

	/* request initial window size */
	gtk_widget_set_size_request(window, 300, 400);
	gtk_widget_show_all(vbox);
	gtk_widget_show(window);

	/* show login dialog, and update friends statuses */
	if (login_dialog(window)) {
		update_friends_statuses(window, window);
		gtk_main();
	}

	gdk_threads_leave();

	return 0;
}
