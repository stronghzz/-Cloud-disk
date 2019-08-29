#include <boost/filesystem.hpp>
#include <fstream>
#include "httplib.h"
#define SERVER_BASE_DIR "www"
#define SERVER_ADDR "0.0.0.0"
#define SERVER_PORT 8000
#define SERVER_BACKUP_DIR SERVER_BASE_DIR"/list"
using namespace httplib;
namespace bf=boost::filesystem;
class CloudServer
{
	private:
		Server srv;
	public:
		CloudServer()
		{
			bf::path base_path(SERVER_BASE_DIR);
			if(!bf::exists(base_path))
			{
				bf::create_directory(base_path);
			}
			bf::path list_path(SERVER_BACKUP_DIR);
			if (!bf::exists(list_path))
			{
				bf::create_directory(list_path);
			}

		}
		bool Start()
		{
			srv.set_base_dir(SERVER_BASE_DIR);
			srv.Get("/(list(/){0,1}){0,1}",GetFileList);
			srv.Get("/list/(.*)",GetFileData);
			srv.Put("/list/(.*)",PutFileData);
			srv.listen(SERVER_ADDR,SERVER_PORT);
			return true;
		}
	private:
		static void PutFileData(const Request &req,Response &rsp)
		{
			std::cout << "backup file" << req.path << "\n";
			if(!req.has_header("Range"))
			{
				rsp.status = 400;
				return;
			}
			std::string range = req.get_header_value("Range");
			int64_t range_start;
			if(RangeParse(range,range_start)==false)
			{
				rsp.status = 400;
				return;
			}
			std::string real = SERVER_BASE_DIR + req.path;
			std::ofstream file(real,std::ios::binary | std::ios::trunc);
			if (!file.is_open())
			{
				std::cerr << "open file" << real << "error\n";
				rsp.status = 500;
				return;
			}
			file.seekp(range_start,std::ios::beg);
			file.write(&req.body[0],req.body.size());
			if (!file.good())
			{
				std::cerr << "file write body error\n";
				rsp.status = 500;
				return;
			}
			file.close();
			return;
		}
		static bool RangeParse(std::string &range,int64_t &start)
		{
			size_t pos1 = range.find("=");
			size_t pos2 = range.find("-");
			if (pos1 == std::string::npos||pos2 == std::string::npos)
			{
				std::cerr << "range:["<<range<<"] format error\n";
				return false;
			}
			std::stringstream rs;
			rs << range.substr(pos1+1,pos2-pos1-1);
			rs >> start;
			return true;
		}
		static void GetFileList(const Request &req,Response &rsp)
		{	
			bf::path list(SERVER_BACKUP_DIR);
			bf::directory_iterator item_begin(list);
			bf::directory_iterator item_end;
			std::string body;
			body = "<html><body><ol><hr />";
			for(;item_begin != item_end;++item_begin)
			{
				if(bf::is_directory(item_begin->status()))//是否是目录
				{
					continue;
				}
				std::string file = item_begin->path().filename().string();
				std::string uri = "/list/" + file;
				body += "<h4><li>";
				body += "<a href='";
				body += uri;
				body += "'>";
				body += file;
				body += "</a>";
				body += "</li></h4>";

			}
			body += "<hr /></ol></body></html>";
			std::cerr << "hello" << std::endl;
			rsp.set_content(&body[0],"text/html");
			return;

		}
		static void GetFileData(const Request &req,Response &rsp)
		{
			std::string file = SERVER_BASE_DIR + req.path;
			if (!bf::exists(file))
			{
				rsp.status = 404;
				return;
			}
			std::ifstream ifile(file,std::ios::binary);
			if (!ifile.is_open())
			{
				std::cerr << "open file" << file << "error\n";
				rsp.status = 500;
				return;

			}
			std::string body;
			int64_t fsize = bf::file_size(file);
			body.resize(fsize);
			ifile.read(&body[0],fsize);
			if (!ifile.good())
			{
				std::cerr << "read file" << file << "body error\n";
				rsp.status = 500;
				return;
			}
			rsp.set_content(body,"text/plain");//给第二次会覆盖
		}
};


int main()
{
	CloudServer srv;
	srv.Start();
	return 0;
}


