#pragma once

#include "../include/WebServ.hpp"

class Request;

class Server
{
    class ServerException : public std::runtime_error
    {
        public:
            explicit ServerException(const std::string &message)
                : std::runtime_error(message) {}
    };

    public:
        Server(std::vector<InfoServer> &serverInfo);
        ~Server();
        void start();
        void handleRequest(int client_fd);
        std::string getContentType(const std::string &file_path);
        void sendResponse(int client_fd, int status_code, const std::string &status_message, const std::string &file_path);
        void execute_cgi(const std::string &script_path);

    private:
        int         createServerSocket(int port);
        std::string getPathIndex(std::string full_file_path, InfoServer& currentServerInfo);
        bool        autoIndexOn(InfoServer& currentServerInfo, std::string file_path);
        void        handleAutoIndex(int client_fd, std::string file_path);
        std::string findRedirect(InfoServer& currentServerInfo, std::string file_path);
        void        handleRedirect(int client_fd, std::string redirect);
        void        processGet(int client_fd, Request &request, InfoServer& currentServerInfo);
        void        processPost(int client_fd, Request &request, InfoServer& currentServerInfo);
        void        processDelete(int client_fd, Request &request, InfoServer& currentServerInfo);
        void        handleFileUpload(int client_fd, Request &request);
        bool        sendAll(int fd_client, const std::string &data);
        void        sendErrorResponse(int fd_client, int code, const std::string &message);
        template <typename T>
        std::string toString(const T &value);
        int         stringToInt(const std::string &str);
        void    processCGI(int client_fd, Request request, InfoServer& currentServerInfo);




        struct sockaddr_in          _address;
        std::vector<InfoServer>     &_serverInfo;
        int                         _server_fd;
        std::vector<pollfd>         _fds;
        std::map<int, InfoServer*>  _serverMap;

};
