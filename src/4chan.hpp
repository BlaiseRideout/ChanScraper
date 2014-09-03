#ifndef FOURCHAN_H
#define FOURCHAN_H

#include <string>
#include <memory>
#include <vector>

namespace fourchan {
	class Board;
	class Post;
	class Thread;
	class Image;

	class Board {
		public:
			Board(std::string board);

			operator std::string();

			static Board *fromUrl(std::string url);

			std::string board;
	};

	class Post {
		public:
			Image getImage();
			Image getThumbnail();

			std::string board;
			unsigned int no;
			unsigned int resto;
			bool sticky;
			bool closed;
			bool archived;
			std::string now;
			unsigned int time;
			std::string name;
			std::string trip;
			std::string id;
			std::string capcode;
			std::string country;
			std::string country_name;
			std::string sub;
			std::string com;
			uint64_t tim;
			std::string filename;
			std::string ext;
			unsigned int fsize;
			std::string md5;
			unsigned int w;
			unsigned int h;
			unsigned int tn_w;
			unsigned int tn_h;
			bool filedeleted;
			bool spoiler;
			unsigned int custom_spoiler;
			unsigned int omitted_posts;
			unsigned int omitted_images;
			unsigned int replies;
			unsigned int images;
			bool bumplimit;
			bool imagelimit;
			unsigned int last_modified;
			std::string tag;
			std::string semantic_url;
		private:
			Post(std::string board,
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
					std::string semantic_url);
		friend class Thread;
	};

	class Thread {
		public:
			Thread(std::string url);
			Thread(std::string board, std::string no);
			Thread(std::string board, unsigned int no);

			void fetchPosts();

			std::string url;
			std::string board;
			int no;
			std::vector<Post> posts;
	};

	class Image {
		public:
			Image(std::string url, unsigned int width, unsigned int height);
			Image(std::string url, unsigned int width, unsigned int height, unsigned int filesize);

			bool writeToFile(std::string filename);

			std::string url;
			unsigned int width;
			unsigned int height;
			unsigned int filesize;
	};

}

#endif