#include "4chan.hpp"

#include <iostream>
#include <regex>
#include <string>
#include <thread>
#include <fstream>
#include <boost/filesystem.hpp>

#include <glibmm.h>
#include <gtkmm/builder.h>
#include <gtkmm/window.h>
#include <gtkmm/entry.h>
#include <gtkmm/button.h>
#include <gtkmm/filechooserbutton.h>
#include <gtkmm/listbox.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/progressbar.h>
#include <gtkmm/statusbar.h>

static Glib::RefPtr<Gtk::Builder> uiBuilder;
static Glib::RefPtr<Gtk::Application> app;

static Gtk::Window *window = NULL;

static Gtk::Entry *threadUrlEntry = NULL;
static Gtk::Button *threadUrlClipboardButton = NULL;

static Gtk::FileChooserButton *locationFileChooserButton = NULL;

static Gtk::Button *addButton = NULL;
static Gtk::Button *removeButton = NULL;

static Gtk::ListBox *threadsList = NULL;

static Gtk::SpinButton *intervalSpinButton = NULL;
static Gtk::Button *intervalButton = NULL;
static Gtk::Button *downloadButton = NULL;

static Gtk::Statusbar *statusBar = NULL;
static Gtk::Label *countdownLabel = NULL;


static std::vector<fourchan::Thread> threads;
static bool updating = false;
static bool downloading = false;
static int secondsToNextUpdate = 30;
static Glib::Thread *thread = NULL;
static std::string statusBarText;
static Glib::Dispatcher updateStatusbarDispatcher;
static Glib::Dispatcher updateCounterDispatcher;
static guint message;

static Glib::StaticMutex mutex = GLIBMM_STATIC_MUTEX_INIT; 

static void threadUrlClipboardButtonClicked() {
	Glib::RefPtr<Gtk::Clipboard> clipboard = Gtk::Clipboard::get();
	Glib::ustring text = clipboard->wait_for_text();
	if(std::regex_match((std::string)text, std::regex("^((https?)?://)?boards.4chan.org/[^/]+/thread/\\d+$")))
		threadUrlEntry->get_buffer()->set_text(text);
}

static void updateStatusBar() {
	{
		Glib::Mutex::Lock lock(mutex);
		if(statusBarText != "")
			statusBar->push(statusBarText, message);
		statusBarText = "";
	}
}

static void downloadThreads() {
	downloading = true;
	std::string folder;
	{ 
      Glib::Mutex::Lock lock(mutex);
      folder = locationFileChooserButton->get_filename();
    }
	if(folder == "") {
		thread = NULL;
		downloading = false;
		return;
	}
	if(!boost::filesystem::is_directory(boost::filesystem::path(folder)) && !boost::filesystem::create_directory(boost::filesystem::path(folder))) {
		{ 
			Glib::Mutex::Lock lock(mutex);
			statusBarText = "Couldn't open folder " + folder;
			std::cerr << statusBarText << std::endl;
	    }
		updateStatusbarDispatcher();
		thread = NULL;
		downloading = false;
		return;
	}
	std::cout << "Downloading " << threads.size() << " threads to " << folder << std::endl;
	for(auto i = threads.begin(); i != threads.end(); ++i) {
		{ 
			Glib::Mutex::Lock lock(mutex);
			statusBarText = "Fetching thread: " + i->url;
			std::cout << statusBarText << std::endl;
		}
		updateStatusbarDispatcher();
		if(!boost::filesystem::is_directory(boost::filesystem::path(folder + "/" + std::to_string(i->no))) && !boost::filesystem::create_directory(boost::filesystem::path(folder + "/" + std::to_string(i->no)))) {
			{
				Glib::Mutex::Lock lock(mutex);
				statusBarText = "Couldn't open folder " + folder + "/" + std::to_string(i->no);
				std::cerr << statusBarText << std::endl;
			}
			updateStatusbarDispatcher();
			continue;
		}
		try {
			i->fetchPosts();
		}
		catch(std::runtime_error e) {
			{
				Glib::Mutex::Lock lock(mutex);
				statusBarText = "Failed to fetch thread " + i->url;
				std::cerr << statusBarText << std::endl;
			}
			updateStatusbarDispatcher();
		}
		for(auto post = i->posts.begin(); post != i->posts.end(); ++post) {
			if(post->filename != "" && !boost::filesystem::exists(boost::filesystem::path(folder + std::string("/") + std::to_string(i->no) + "/" + std::to_string(post->tim) + post->ext))) {
				{ 
					Glib::Mutex::Lock lock(mutex);
					if(!downloading)
						return;
					statusBarText = "Fetching image " + std::to_string(post->tim) + post->ext;
					std::cout << statusBarText << std::endl;
				}
				updateStatusbarDispatcher();
				fourchan::Image image = post->getImage();

				image.writeToFile(folder + std::string("/") + std::to_string(i->no) + "/" + std::to_string(post->tim) + post->ext);
			}
		}
		{ 
			Glib::Mutex::Lock lock(mutex);
			statusBarText = "Finished fetching thread: " + i->url;
			std::cout << statusBarText << std::endl;
		}
		updateStatusbarDispatcher();
		if(!downloading)
			return;
	}
	thread = NULL;
	downloading = false;
}

static void downloadButtonClicked() {
	if(updating)
		secondsToNextUpdate = 1;
	else {
		if(thread == NULL)
			thread = Glib::Thread::create(sigc::ptr_fun(downloadThreads));
	}
}

static void addButtonClicked() {
	Glib::ustring text = threadUrlEntry->get_buffer()->get_text();
	if(std::regex_match((std::string)text, std::regex("^((https?)?://)?boards.4chan.org/[^/]+/thread/\\d+$"))) {
		for(auto thread = threads.begin(); thread != threads.end(); ++thread) {
			if(thread->url == text)
				return;
		}
		threads.push_back(fourchan::Thread(text));

		Gtk::ListBoxRow *row = new Gtk::ListBoxRow();
		row->add_label(text);
		threadsList->append(*row);
		threadsList->show_all();
	}
}

static void removeButtonClicked() {
	if(threadsList->get_selected_row() != NULL) {
		threads.erase(threads.begin() + threadsList->get_selected_row()->get_index());
		
		threadsList->remove(*threadsList->get_selected_row());
		threadsList->show_all();
	}
}

static void updateCounter() {
	{
		Glib::Mutex::Lock lock(mutex);
		countdownLabel->set_label(std::to_string(secondsToNextUpdate));
	}
}

static void autoUpdate() {
	while(1) {
		{ 
	      Glib::Mutex::Lock lock(mutex);
	      if(!updating)
	      	break;
	    }

		if(secondsToNextUpdate <= 0) {
			downloadThreads();
			{ 
		      Glib::Mutex::Lock lock(mutex);
		      secondsToNextUpdate = intervalSpinButton->get_value_as_int();
		    }
		}
		else
			secondsToNextUpdate--;
		updateCounterDispatcher();
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
	countdownLabel->set_label("");
	thread = NULL;
}

static void intervalButtonClicked() {
	updating = !updating;
	if(updating) {
		secondsToNextUpdate = intervalSpinButton->get_value_as_int();
		thread = Glib::Thread::create(sigc::ptr_fun(autoUpdate));
		intervalButton->set_label("Stop");
	}
	else {
		intervalButton->set_label("Start");
	}
}

static bool windowClosed(GdkEventAny *event) {
	updating = false;
	downloading = false;

	if(thread != NULL && thread->joinable())
		thread->join();

	return false;
}

static void initWindow() {
	if(window) {
		window->set_default_size(500, 500);
		window->signal_delete_event().connect(sigc::ptr_fun(windowClosed));

		uiBuilder->get_widget("threadUrlEntry", threadUrlEntry);

		uiBuilder->get_widget("threadUrlClipboardButton", threadUrlClipboardButton);
		if(threadUrlClipboardButton)
			threadUrlClipboardButton->signal_clicked().connect(sigc::ptr_fun(threadUrlClipboardButtonClicked));

		uiBuilder->get_widget("locationFileChooserButton", locationFileChooserButton);
		/*if(locationFileChooserButton)
			locationFileChooserButton->signal_state_changed().connect(sigc::ptr_fun(locationFileChooserButtonChanged));*/

		uiBuilder->get_widget("addButton", addButton);
		if(addButton)
			addButton->signal_clicked().connect(sigc::ptr_fun(addButtonClicked));

		uiBuilder->get_widget("removeButton", removeButton);
		if(removeButton)
			removeButton->signal_clicked().connect(sigc::ptr_fun(removeButtonClicked));


		uiBuilder->get_widget("threadsList", threadsList);


		uiBuilder->get_widget("intervalSpinButton", intervalSpinButton);

		uiBuilder->get_widget("intervalButton", intervalButton);
		if(intervalButton)
			intervalButton->signal_clicked().connect(sigc::ptr_fun(intervalButtonClicked));


		uiBuilder->get_widget("downloadButton", downloadButton);
		if(downloadButton)
			downloadButton->signal_clicked().connect(sigc::ptr_fun(downloadButtonClicked));

		uiBuilder->get_widget("statusBar", statusBar);

		uiBuilder->get_widget("countdownLabel", countdownLabel);
	}
    message = statusBar->get_context_id("Download Threads");
    updateStatusbarDispatcher.connect(sigc::ptr_fun(updateStatusBar));
    updateCounterDispatcher.connect(sigc::ptr_fun(updateCounter));
}

int main(int argc, char **argv) {
	app = Gtk::Application::create(argc, argv, "com.blaiseritchie.chanscraper");

	uiBuilder = Gtk::Builder::create();
	try {
		uiBuilder->add_from_file("gtkbuilder.glade");
	}
	catch(const Glib::FileError& ex) {
		std::cerr << "FileError: " << ex.what() << std::endl;
		return 1;
	}
	catch(const Glib::MarkupError& ex) {
		std::cerr << "MarkupError: " << ex.what() << std::endl;
		return 1;
	}
	catch(const Gtk::BuilderError& ex) {
		std::cerr << "BuilderError: " << ex.what() << std::endl;
		return 1;
	}

	uiBuilder->get_widget("mainWindow", window);
	initWindow();

	int ret = app->run(*window);

	return ret;
}