// JamBot.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "JamBot.h"
#include "OptiAlgo.h"
#include "InputChannelReader.h"
#include "DMXOutput.h"
#include "WavManipulation.h"
#include "Helpers.h"
#include "strsafe.h"
#include "gtk/gtk.h"
#include <iostream>
#include <fstream>
#include <string>


#define MAX_LOADSTRING 100

// Objects definition - name + id
#define IDC_STARTSYS_BUTTON 101
#define IDC_STOPSYS_BUTTON  102

// Thread-related global constants
#define MAX_THREADS 4
#define AUDIOINPUT_THREAD_ARR_ID 0
#define WAVGEN_THREAD_ARR_ID 1
#define OPTIALGO_THREAD_ARR_ID 2
#define AUDIOOUTPUT_THREAD_ARR_ID 3

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
HANDLE hThreadArray[MAX_THREADS];				// Array of threads
DWORD dwThreadArray[MAX_THREADS];				// Array of returned thread IDs
InputChannelReader inputChannelReader = InputChannelReader();
WavManipulation wavmanipulation = WavManipulation();
OptiAlgo optiAlgo = OptiAlgo();
DMXOutput lightsTest = DMXOutput();

// Functions to run components in threads
DWORD WINAPI AudioInputThread(LPVOID lpParam) { inputChannelReader = InputChannelReader(); Helpers::print_debug("START audio input.\n"); inputChannelReader.start(); return 0; }
DWORD WINAPI WavGenThread(LPVOID lpParam) { wavmanipulation = WavManipulation(); Helpers::print_debug("START wav manip.\n"); wavmanipulation.startSnip(); return 0; }
DWORD WINAPI OptiAlgoThread(LPVOID lpParam) { optiAlgo = OptiAlgo(); Helpers::print_debug("START opti algo.\n"); optiAlgo.start(); return 0; }
DWORD WINAPI AudioOutputThread(LPVOID lpParam) { lightsTest = DMXOutput(); Helpers::print_debug("START audio output.\n"); lightsTest.start(); return 0; }


// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
void CloseThread(int id);
void CloseAllThreads();
void ErrorHandler(LPTSTR lpszFunction);
const gchar *textInput;
GtkWidget *window;
GtkWidget *textEntry;

static void changeProgressBar(GtkWidget *widget, gpointer data)
{
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(data), 0.8);
}

static void testFunction(GtkWidget *widget) {
	//g_signal_new("pitch-data", G_TYPE_OBJECT, G_SIGNAL_RUN_FIRST, 0, NULL, NULL, NULL, G_TYPE_NONE, 0);
	g_signal_emit_by_name(window, "button_press_event");
}

static void openDialog(GtkWidget *button, gpointer window) {
	GtkWidget *dialog, *label;
	dialog = gtk_dialog_new_with_buttons("dialog", GTK_WINDOW(window), GTK_DIALOG_MODAL, GTK_STOCK_OPEN, GTK_RESPONSE_OK,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
	label = gtk_label_new("You clicked the button");
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label, 0, 0, 0);
	gtk_widget_show_all(dialog);
	gint response = gtk_dialog_run(GTK_DIALOG(dialog));
	if (response == GTK_RESPONSE_OK){
		g_print("The OK is pressed");
	}
	else {
		g_print("The cancel was pressed");
	}
	gtk_widget_destroy(dialog);
}

static void dialog_result(GtkWidget *dialog, gint resp, gpointer data){
	if (resp == GTK_RESPONSE_OK) {
		g_print("OK\n");
	}
	else {
		gtk_widget_destroy(dialog);
	}
}

static void openNoneModalDialog(GtkWidget *button, gpointer window) {
	GtkWidget *dialog, *label, *image, *hbox;
	dialog = gtk_dialog_new_with_buttons("Nonmodal dialog", GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK,
		GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
	label = gtk_label_new("The buttons was clicked");
	image = gtk_image_new_from_stock(GTK_STOCK_INFO, GTK_ICON_SIZE_DIALOG);

	hbox = gtk_hbox_new(0, 0);
	gtk_box_pack_start(GTK_BOX(hbox), image, 0, 0, 0);
	gtk_box_pack_start(GTK_BOX(hbox), label, 0, 0, 0);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, 0, 0, 0);
	gtk_widget_show_all(dialog);
	g_signal_connect(dialog, "response", G_CALLBACK(dialog_result), NULL);
}

static void fileBrowse(GtkWidget *button, gpointer window) {
	GtkWidget *dialog;
	gchar *fileName;
	dialog = gtk_file_chooser_dialog_new("Choose a file", GTK_WINDOW(window), GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_OK,
		GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
	gtk_widget_show_all(dialog);
	gint resp = gtk_dialog_run(GTK_DIALOG(dialog));
	if (resp == GTK_RESPONSE_OK) {
		fileName = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		ifstream file(fileName);
		string line, text;
		if (file.is_open())
		{
			while (getline(file, line))
			{
				text += line;
				text += "\n";
			}
			file.close();
			GtkTextBuffer *buffer;
			buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textEntry));
			gtk_text_buffer_set_text(buffer, text.c_str(), -1);
		}
	}
	else {
		g_print("You Pressed the cancel button");
	}
	gtk_widget_destroy(dialog);
}


static void startJamming(GtkWidget *button) {
	hThreadArray[AUDIOOUTPUT_THREAD_ARR_ID] = CreateThread(
		NULL,
		0,
		AudioOutputThread,
		NULL,
		0,
		&dwThreadArray[AUDIOOUTPUT_THREAD_ARR_ID]);
	if (hThreadArray[AUDIOOUTPUT_THREAD_ARR_ID] == NULL)
	{
		ErrorHandler(TEXT("CreateThread"));
		CloseAllThreads();
		ExitProcess(3);
	}
	hThreadArray[OPTIALGO_THREAD_ARR_ID] = CreateThread(
		NULL,
		0,
		OptiAlgoThread,
		NULL,
		0,
		&dwThreadArray[OPTIALGO_THREAD_ARR_ID]);
	if (hThreadArray[OPTIALGO_THREAD_ARR_ID] == NULL)
	{
		ErrorHandler(TEXT("CreateThread"));
		CloseAllThreads();
		ExitProcess(3);
	}
	hThreadArray[WAVGEN_THREAD_ARR_ID] = CreateThread(
		NULL,
		0,
		WavGenThread,
		NULL,
		0,
		&dwThreadArray[WAVGEN_THREAD_ARR_ID]);
	if (hThreadArray[WAVGEN_THREAD_ARR_ID] == NULL)
	{
		ErrorHandler(TEXT("CreateThread"));
		CloseAllThreads();
		ExitProcess(3);
	}
	hThreadArray[AUDIOINPUT_THREAD_ARR_ID] = CreateThread(
		NULL,
		0,
		AudioInputThread,
		NULL,
		0,
		&dwThreadArray[AUDIOINPUT_THREAD_ARR_ID]);
	if (hThreadArray[AUDIOINPUT_THREAD_ARR_ID] == NULL)
	{
		ErrorHandler(TEXT("CreateThread"));
		CloseAllThreads();
		ExitProcess(3);
	}
}

static void startEmerson(GtkWidget *button) {
	wavmanipulation.startanalysis();
}

static gboolean delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	/* If you return FALSE in the "delete-event" signal handler,
	* GTK will emit the "destroy" signal. Returning TRUE means
	* you don't want the window to be destroyed.
	* This is useful for popping up 'are you sure you want to quit?'
	* type dialogs. */

	g_print("delete event occurred\n");

	/* Change TRUE to FALSE and the main window will be destroyed with
	* a "delete-event". */

	return TRUE;
}

/* Another callback */
static void destroy(GtkWidget *widget,
	gpointer   data)
{
	CloseAllThreads();
	gtk_main_quit();
}

int gtkStart(int argc, char* argv[])
{
	GtkWidget *window;
	GtkWidget *windowBox, *songProgressBox, *songControlBox, *songLyricsBox, *songInputBox, *jambox;
	GtkWidget *playButton, *progressBar, *progressBarTest, *showModalDialog, *showNonmodalDialog, *fileSelectDialog, *emersonButton;
	GtkWidget *startJambot;

	gtk_init(&argc, &argv);

	/*=========================== Window ===========================*/
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	g_signal_connect(window, "delete-event", G_CALLBACK(delete_event), NULL);
	g_signal_connect(window, "destroy", G_CALLBACK(destroy), NULL);

	gtk_container_set_border_width(GTK_CONTAINER(window), 100);
	gtk_window_set_title(GTK_WINDOW(window), "JamBot");

	g_signal_connect(window, "button_press_event", G_CALLBACK(changeProgressBar), NULL);

	/*=========================== Widget boxes ===========================*/
	windowBox = gtk_hbox_new(false, 0);
	//gtk_widget_set_size_request(windowBox, 200, 30);

	songLyricsBox = gtk_vbox_new(false, 0);
	//gtk_widget_set_size_request(songLyricsBox, 300, 10);
	gtk_box_pack_start(GTK_BOX(windowBox), songLyricsBox, false, false, 5);

	songInputBox = gtk_vbox_new(false, 0);
	gtk_box_pack_start(GTK_BOX(windowBox), songInputBox, false, false, 5);

	jambox = gtk_hbox_new(false, 0);
	//gtk_widget_set_size_request(jambox, 150, 30);
	gtk_box_pack_start(GTK_BOX(windowBox), jambox, false, false, 5);

	songControlBox = gtk_hbox_new(true, 0);
	//gtk_widget_set_size_request(songControlBox, 150, 30);
	gtk_box_pack_start(GTK_BOX(songInputBox), songControlBox, false, false, 5);

	songProgressBox = gtk_hbox_new(false, 0);
	//gtk_widget_set_size_request(songProgressBox, 100, 30);
	gtk_box_pack_start(GTK_BOX(songInputBox), songProgressBox, false, false, 5);

	/*=========================== Progress bar ===========================*/
	progressBar = gtk_progress_bar_new();
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progressBar), 0.2);
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progressBar), "progress bar");
	gtk_box_pack_start(GTK_BOX(songProgressBox), progressBar, false, false, 5);

	/*=========================== Button ===========================*/
	GtkWidget *playArrow;
	playArrow = gtk_arrow_new(GTK_ARROW_RIGHT, GTK_SHADOW_NONE);

	playButton = gtk_button_new();
	gtk_container_add(GTK_CONTAINER(playButton), playArrow);
	gtk_box_pack_start(GTK_BOX(songControlBox), playButton, false, false, 5);
	gtk_widget_show(playArrow);
	gtk_widget_show(playButton);

	playButton = gtk_button_new_with_label("Pause");
	gtk_box_pack_start(GTK_BOX(songControlBox), playButton, false, false, 5);
	gtk_widget_show(playButton);

	playButton = gtk_button_new_with_label("Stop");
	gtk_box_pack_start(GTK_BOX(songControlBox), playButton, false, false, 5);
	gtk_widget_show(playButton);

	fileSelectDialog = gtk_button_new_with_label("File Select");
	gtk_box_pack_start(GTK_BOX(songLyricsBox), fileSelectDialog, false, false, 5);
	g_signal_connect(GTK_OBJECT(fileSelectDialog), "clicked", G_CALLBACK(fileBrowse), window);

	progressBarTest = gtk_button_new_with_label("Test");
	gtk_box_pack_start(GTK_BOX(songProgressBox), progressBarTest, false, false, 5);
	g_signal_connect(GTK_OBJECT(progressBarTest), "clicked", G_CALLBACK(testFunction), (gpointer)progressBar);

	showModalDialog = gtk_button_new_with_label("Show modal dialog");
	gtk_box_pack_start(GTK_BOX(songProgressBox), showModalDialog, false, false, 5);
	g_signal_connect(GTK_OBJECT(showModalDialog), "clicked", G_CALLBACK(openDialog), window);

	showNonmodalDialog = gtk_button_new_with_label("Show nonmodal dialog");
	gtk_box_pack_start(GTK_BOX(songProgressBox), showNonmodalDialog, false, false, 5);
	g_signal_connect(GTK_OBJECT(showNonmodalDialog), "clicked", G_CALLBACK(openNoneModalDialog), window);

	startJambot = gtk_button_new_with_label("Start system as normal");
	gtk_box_pack_start(GTK_BOX(jambox), startJambot, false, false, 5);
	g_signal_connect(GTK_OBJECT(startJambot), "clicked", G_CALLBACK(startJamming), window);

	emersonButton = gtk_button_new_with_label("Emerson Button");
	gtk_box_pack_start(GTK_BOX(jambox), emersonButton, false, false, 5);
	g_signal_connect(GTK_OBJECT(emersonButton), "clicked", G_CALLBACK(startEmerson), window);

	/*=========================== Text entry ===========================*/
	textEntry = gtk_text_view_new();
	GtkTextBuffer *buffer;
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textEntry));
	gtk_text_buffer_set_text(buffer, "Hello this is some text\n hello", -1);
	//gtk_text_view_set_border_window_size(GTK_TEXT_VIEW(textEntry), GTK_TEXT_WINDOW_LEFT, 10);
	gtk_box_pack_start(GTK_BOX(songLyricsBox), textEntry, false, false, 5);

	/*=========================== File Browser ===========================*/
	/*fileBrowser = gtk_file_chooser_dialog_new("File selection", GTK_WINDOW(window), GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_OK,
	GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
	gtk_file_selection_set_filename(GTK_FILE_SELECTION(fileBrowser), "penguin.png");
	gtk_box_pack_start(GTK_BOX(windowBox), fileBrowser, false, false, 5);*/

	/*=========================== the rest ===========================*/
	gtk_container_add(GTK_CONTAINER(window), windowBox);

	gtk_widget_show_all(window);
	gtk_main();

	return 0;
}

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPTSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	gtkStart(0, NULL);
	return 0;
}



void CloseThread(int id)
{
	CloseHandle(hThreadArray[id]);
}
void CloseAllThreads()
{
	DWORD result;

	Helpers::print_debug("Stopping audio input...\n");
	inputChannelReader.stop();
	result = WaitForSingleObject(hThreadArray[AUDIOINPUT_THREAD_ARR_ID], 10000);
	if (result == WAIT_OBJECT_0) { Helpers::print_debug("STOP audio input.\n"); }
	else if (result == WAIT_FAILED) { ErrorHandler(TEXT("WaitForSingleObject")); }
	else { Helpers::print_debug("FAILED stopping audio input.\n"); }

	Helpers::print_debug("Stopping optimization algorithm...\n");
	optiAlgo.stop();
	result = WaitForSingleObject(hThreadArray[OPTIALGO_THREAD_ARR_ID], 3000);
	if (result == WAIT_OBJECT_0) { Helpers::print_debug("STOP opti algo.\n"); }
	else if (result == WAIT_FAILED) { ErrorHandler(TEXT("WaitForSingleObject")); }
	else { Helpers::print_debug("FAILED stopping optimization algorithm.\n"); }

	//Helpers::print_debug("Stopping wav manip...\n");
	////wavmanipulation.stop();
	//result = WaitForSingleObject(hThreadArray[WAVGEN_THREAD_ARR_ID], 500);
	//if (result == WAIT_OBJECT_0) { Helpers::print_debug("STOP wav manip.\n"); }
	//else if (result == WAIT_FAILED) { ErrorHandler(TEXT("WaitForSingleObject")); }
	//else { Helpers::print_debug("FAILED stopping wav manip.\n"); }

	Helpers::print_debug("Stopping audio output...\n");
	lightsTest.stop();
	result = WaitForSingleObject(hThreadArray[AUDIOOUTPUT_THREAD_ARR_ID], 3000);
	if (result == WAIT_OBJECT_0) { Helpers::print_debug("STOP audio output.\n"); }
	else if (result == WAIT_FAILED) { ErrorHandler(TEXT("WaitForSingleObject")); }
	else { Helpers::print_debug("FAILED stopping audio output.\n"); }
}

void ErrorHandler(LPTSTR lpszFunction)
{
	// Retrieve the system error message for the last-error code.

	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	// Display the error message.

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
		(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
	StringCchPrintf((LPTSTR)lpDisplayBuf,
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s failed with error %d: %s"),
		lpszFunction, dw, lpMsgBuf);
	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

	// Free error-handling buffer allocations.

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
}