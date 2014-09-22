#define _CRT_SECURE_NO_DEPRECATE
#include <iostream>

#include "4chan.hpp"

#include <regex>
#include <stdexcept>

#include <json/json.h>
#include <curl/curl.h>

using namespace fourchan;

static size_t writeData(char *contents, size_t size, size_t nmemb, void *userp) {
    ((std::string*)userp)->append(contents, size * nmemb);
    return size * nmemb;
}

static size_t saveData(char *contents, size_t size, size_t nmemb, void *userp) {
	size_t written = fwrite(contents, size, nmemb, (FILE*)userp);
    return written;
}

static std::string downloadFile(std::string url) {
	std::string data;
	CURL *curl;
	CURLcode res;

	curl = curl_easy_init();
	if(curl) {
		curl_easy_setopt(curl, CURLOPT_URL, (url).c_str());
		/* example.com is redirected, so we tell libcurl to follow redirection */ 
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeData);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);

		/* Perform the request, res will get the return code */ 
		res = curl_easy_perform(curl);
		/* Check for errors */ 
		if(res != CURLE_OK)
			std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;

		/* always cleanup */ 
		curl_easy_cleanup(curl);
	}

	return data;
}

static bool downloadFile(std::string url, std::string filename) {
	FILE *fp;
	CURL *curl;
	CURLcode res;

	fp = fopen(filename.c_str(), "wb");

	if(!fp) {
		std::cerr << "Couldn't open file " << filename << " for writing" << std::endl;
		return false;
	}

	curl = curl_easy_init();
	if(curl) {
		curl_easy_setopt(curl, CURLOPT_URL, (url).c_str());
		/* example.com is redirected, so we tell libcurl to follow redirection */ 
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, saveData);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

		/* Perform the request, res will get the return code */ 
		res = curl_easy_perform(curl);
		/* Check for errors */ 
		if(res != CURLE_OK) {
			std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
			return false;
		}

		/* always cleanup */ 
		curl_easy_cleanup(curl);

		fclose(fp);
	}

	return true;
}

Board::Board(std::string board) : board(board) {
}

Board::operator std::string() {
	return board;
}

Board *Board::fromUrl(std::string url) {
	std::regex boardRegex("^((https?)?://)?(boards\\.)?4chan\\.org/[^/]+(/thread/\\d+)?$");
	std::smatch matchObjects;
	if(std::regex_match((std::string)url, matchObjects, boardRegex)) {
		for(auto i = matchObjects.begin(); i < matchObjects.end(); ++i)
			std::cout << *i << std::endl;
	}
	return NULL;
}


Post::Post(std::string board,
			unsigned int no, 
			unsigned int resto, 
			bool sticky, 
			bool closed, 
			bool archived, 
			std::string now, 
			unsigned int time, 
			std::string name, 
			std::string trip, 
			std::string id, 
			std::string capcode, 
			std::string country, 
			std::string country_name, 
			std::string sub, 
			std::string com, 
			uint64_t tim, 
			std::string filename, 
			std::string ext, 
			unsigned int fsize, 
			std::string md5, 
			unsigned int w, 
			unsigned int h, 
			unsigned int tn_w, 
			unsigned int tn_h, 
			bool filedeleted, 
			bool spoiler, 
			unsigned int custom_spoiler, 
			unsigned int omitted_posts, 
			unsigned int omitted_images, 
			unsigned int replies, 
			unsigned int images, 
			bool bumplimit, 
			bool imagelimit, 
			unsigned int last_modified, 
			std::string tag, 
			std::string semantic_url) :

			board(board),
			no(no),
			resto(resto),
			sticky(sticky),
			closed(closed),
			archived(archived),
			now(now),
			time(time),
			name(name),
			trip(trip),
			id(id),
			capcode(capcode),
			country(country),
			country_name(country_name),
			sub(sub),
			com(com),
			tim(tim),
			filename(filename),
			ext(ext),
			fsize(fsize),
			md5(md5),
			w(w),
			h(h),
			tn_w(tn_w),
			tn_h(tn_h),
			filedeleted(filedeleted),
			spoiler(spoiler),
			custom_spoiler(custom_spoiler),
			omitted_posts(omitted_posts),
			omitted_images(omitted_images),
			replies(replies),
			images(images),
			bumplimit(bumplimit),
			imagelimit(imagelimit),
			last_modified(last_modified),
			tag(tag),
			semantic_url(semantic_url)
	 {
}


Image Post::getImage() {
	Image img("http://i.4cdn.org/" + this->board + "/" + std::to_string(this->tim) + this->ext, this->w, this->h, this->fsize);
	return img;
}

Image Post::getThumbnail() {
	Image img("http://t.4cdn.org/" + this->board + "/" + std::to_string(this->tim) + "s" + this->ext, this->tn_w, this->tn_h);
	return img;
}

Thread::Thread(std::string url) : url(url) {
	if(!std::regex_match(this->url, std::regex("^((https?)?://)?(boards\\.4chan\\.org||a\\.4cdn\\.org)/[^/]+/thread/\\d+$")))
		throw std::runtime_error("fourchan::Thread::Thread(std::string url): url not a valid 4chan thread URL: " + this->url);

	std::regex boardRegex(".*(4chan\\.org||4cdn\\.org)/([^/]+)/thread/(\\d+)(.json)?$");
	std::smatch matchObjects;
	if(std::regex_match(this->url, matchObjects, boardRegex)) {
		this->board = matchObjects[2];
		this->no = std::stoi(matchObjects[3]);
	}

}

Thread::Thread(std::string board, std::string no) : Thread("http://4chan.org/" + board + "/thread/" + no) {
}

Thread::Thread(std::string board, unsigned int no) : Thread(board, std::to_string(no)) {
}

void Thread::fetchPosts() {
	this->posts.clear();

	std::string data = downloadFile("http://a.4cdn.org/" + this->board + "/thread/" + std::to_string(no) + ".json");
	
	Json::Value root;
	Json::Reader reader;

	if(!reader.parse(data, root) || !root.isObject())
		throw std::runtime_error("Failed to parse thread: http://a.4cdn.org/" + this->board + "/thread/" + std::to_string(no) + ".json" + reader.getFormattedErrorMessages());

	Json::Value posts = root["posts"];

	if(!posts.isArray())
		throw std::runtime_error("Failed to parse thread: http://a.4cdn.org/" + this->board + "/thread/" + std::to_string(no) + ".json" + reader.getFormattedErrorMessages());

	for(unsigned int i = 0; i < posts.size(); ++i) {
		Json::Value post = posts[i];


		if(!post.isObject()) {
			std::cerr << "Post object not an object: " << this->url << post.toStyledString() << std::endl;

			continue;
		}

		this->posts.push_back(Post(this->board,
							post.get("no", 0).asUInt(),
							post.get("resto", 0).asUInt(),
							post.get("sticky", 0).asUInt() == 1,
							post.get("closed", 0).asUInt() == 1,
							post.get("archived", 0).asUInt() == 1,
							post.get("now", "").asString(),
							post.get("time", 0).asUInt(),
							post.get("name", "").asString(),
							post.get("trip", "").asString(),
							post.get("id", "").asString(),
							post.get("capcode", "").asString(),
							post.get("country", "").asString(),
							post.get("country_name", "").asString(),
							post.get("sub", "").asString(),
							post.get("com", "").asString(),
							post.get("tim", 0).asUInt64(),
							post.get("filename", "").asString(),
							post.get("ext", "").asString(),
							post.get("fsize", 0).asUInt(),
							post.get("md5", "").asString(),
							post.get("w", 0).asUInt(),
							post.get("h", 0).asUInt(),
							post.get("tn_w", 0).asUInt(),
							post.get("tn_h", 0).asUInt(),
							post.get("filedeleted", 0).asUInt() == 1,
							post.get("spoiler", 0).asUInt() == 1,
							post.get("custom_spoiler", 0).asUInt(),
							post.get("omitted_posts", 0).asUInt(),
							post.get("omitted_images", 0).asUInt(),
							post.get("replies", 0).asUInt(),
							post.get("images", 0).asUInt(),
							post.get("bumplimit", 0).asUInt() == 1,
							post.get("imagelimit", 0).asUInt() == 1,
							post.get("last_modified", 0).asUInt(),
							post.get("tag", "").asString(),
							post.get("semantic_url", "").asString()));
		
	}
}

Image::Image(std::string url, unsigned int width, unsigned int height) : url(url), width(width), height(height), filesize(width * height * 3) {
}

Image::Image(std::string url, unsigned int width, unsigned int height, unsigned int filesize) : url(url), width(width), height(height), filesize(filesize) {
}

bool Image::writeToFile(std::string filename) {
	return downloadFile(this->url, filename);
}