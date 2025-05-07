#include "../include/Server.hpp"

Server::Server(std::vector<InfoServer> &serverInfo): _serverInfo(serverInfo), _server_fd(0)
{
    for (size_t i = 0; i < this->_serverInfo.size(); i++)
    {
        int port = this->_serverInfo[i].server_port["default"];
        int server_fd = this->createServerSocket(port);

        pollfd server_pollfd;
        memset(&server_pollfd, 0, sizeof(pollfd));
        server_pollfd.fd = server_fd;
        server_pollfd.events = POLLIN | POLLOUT;
        this->_fds.push_back(server_pollfd);
        this->_serverMap[server_fd] = &this->_serverInfo[i];
    }
}

Server::~Server()
{

}

int Server::createServerSocket(int port)
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        std::cerr << "Erreur lors de la creation du socket" << std::endl;
        throw ServerException("echec de la creation du socket");
    }
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        close(server_fd);
        throw ServerException("Failed to set SO_REUSEADDR");
    }
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) == -1)
    {
        close(server_fd);
        throw ServerException("Failed to set SO_REUSEPORT");
    }



    if (fcntl(server_fd, F_SETFL, O_NONBLOCK) == -1)
    {
        std::cerr << "Erreur lors de la configuration du socket en mode non-bloquant" << std::endl;
        close(server_fd);
        throw ServerException("Echec de la configuration du socket en mode non-bloquant");
    }

    this->_address.sin_family = AF_INET;
    this->_address.sin_addr.s_addr = INADDR_ANY;
    this->_address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&this->_address, sizeof(this->_address)) == -1)
    {
        std::cerr << "Erreur lors du bind du socket" << std::endl;
        close(server_fd);
        throw ServerException("Echec du bind du socket");
    }
    if (listen(server_fd, 10) == -1)
    {
        std::cerr << "Erreur lors de l'appel à listen" << std::endl;
        close(server_fd);
        throw ServerException("Echec de l'appel à listen");
    }
    std::cout << "Serveur en ecoute...\n";
    return server_fd;
}

void Server::start()
{
    int sock_opt;
    socklen_t optlen = sizeof(sock_opt);
    std::map<int, time_t> last_activity;

    while (!gServerStop)
    {
        int ret = poll(this->_fds.data(), this->_fds.size(), TIMEOUT);
        if (ret == -1)
            break ;
        
        time_t now = time(NULL);
        
        for (size_t i = 0; i < this->_fds.size(); i++)
        {
            int fd = this->_fds[i].fd;
            if (last_activity.find(fd) != last_activity.end() && (now - last_activity[fd]) * 1000 > TIMEOUT)
            {
                close(fd);
                this->_fds.erase(this->_fds.begin() + i);
                last_activity.erase(fd);
                i--;
                continue;
            }

            if (this->_fds[i].revents & POLLIN || this->_fds[i].revents & POLLOUT)
            {
                if (this->_serverMap.find(this->_fds[i].fd) != this->_serverMap.end() && 
                        getsockopt(this->_fds[i].fd, SOL_SOCKET, SO_ACCEPTCONN, &sock_opt, &optlen) != -1 &&
                            sock_opt == 1)
                {
                    int client_fd = accept(this->_fds[i].fd, NULL, NULL);
                    if (client_fd < 0)
                    {
                        std::cerr << "Erreur lors de l'acceptation de la connexion" << std::endl;
                        continue;
                    }

                    int flag = 1;
                    setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
                    setsockopt(client_fd, IPPROTO_TCP, TCP_QUICKACK, &flag, sizeof(flag));
                    
                    struct linger sl;
                    sl.l_onoff = 1;
                    sl.l_linger = 0;
                    setsockopt(client_fd, SOL_SOCKET, SO_LINGER, &sl, sizeof(sl));

                    this->_serverMap[client_fd] = this->_serverMap[this->_fds[i].fd];
                    
                    pollfd client_pollfd;
                    memset(&client_pollfd, 0, sizeof(client_pollfd));
                    client_pollfd.fd = client_fd;
                    client_pollfd.events = POLLIN;
                    this->_fds.push_back(client_pollfd);

                    last_activity[client_fd] = now;
                }
                else
                {
                    if (this->_fds[i].revents & POLLIN)
                    {
                        try
                        {
                            this->handleRequest(this->_fds[i].fd);
                            last_activity[fd] = now;
                        }
                        catch (const std::exception &e)
                        {
                            std::cerr << "Erreur lors du traitement de la requete : " << e.what() << std::endl;
                            close(fd);
                            this->_fds.erase(this->_fds.begin() + i);
                            last_activity.erase(fd);
                            i--;
                            continue ;
                        }
                    }
                    if (this->_fds[i].revents & POLLOUT)
                    {
                        try
                        {
                            this->handleRequest(this->_fds[i].fd);
                            last_activity[fd] = now;
                        }
                        catch (const std::exception &e)
                        {
                            std::cerr << "Erreur lors de l'envoi de la reponse : " << e.what() << std::endl;
                            close(fd);
                            this->_fds.erase(this->_fds.begin() + i);
                            last_activity.erase(fd);
                            i--;
                            continue;
                        }
                    }
                    usleep(1000);
                    close(fd);
                    this->_fds.erase(this->_fds.begin() + i);
                    last_activity.erase(fd);
                    i--;
                }
            }
        }
    }
    for (size_t i = 0; i < this->_fds.size(); i++)
        close(this->_fds[i].fd);
    this->_fds.clear();
}

void Server::handleRequest(int client_fd)
{
    InfoServer& currentServerInfo = *(this->_serverMap[client_fd]);
    Request request(currentServerInfo);
    try
    {
        int returnReadRequest = request.readRequest(client_fd);
        if (returnReadRequest == -1)
            return this->sendErrorResponse(client_fd, 500, "Internal Server Error");
        else if (returnReadRequest == 0)
            return this->sendErrorResponse(client_fd, 400, "Bad Request");

        int returnParseRequest = request.parseRequest(client_fd);
        if (returnParseRequest == -1)
            return this->sendErrorResponse(client_fd, 500, "Internal Server Error");
        else if (returnParseRequest == 0)
            return this->sendErrorResponse(client_fd, 400, "Bad Request");

        /*
        std::cout << "\n\n----- BEGIN REQUETE HTTP DANS SERVER.CPP handleRequest() -----"<< std::endl;
        std::cout << "Methode\t: " << request.getMethod() << std::endl;
        std::cout << "Url\t: " << request.getUrl() << std::endl;
        std::cout << "Path\t: " << request.getPath() << std::endl;
        std::cout << "Body\t: " << request.getBody() << std::endl;
        std::cout << "Version HTTP\t: " << request.getHttpVersion() << std::endl;
        std::cout << "----- END REQUETE HTTP -----\n\n"<< std::endl;
        */
        
        if (request.getPath().find("/cgi-bin") == 0 && request.getMethodAllow() == true)
            return this->processCGI(client_fd, request, currentServerInfo);
        else if (request.getMethod() == "GET" && request.getMethodAllow() == true)
            this->processGet(client_fd, request, currentServerInfo);
        else if (request.getMethod() == "POST" && request.getMethodAllow() == true)
            this->processPost(client_fd, request, currentServerInfo);
        else if (request.getMethod() == "DELETE" && request.getMethodAllow() == true)
            this->processDelete(client_fd, request, currentServerInfo);
    }
    catch (const std::exception &e)
    {
        if (request.getMethodAllow() == false)
        {
            std::cerr << "Error : this method is not allowed." << std::endl;
            this->sendResponse(client_fd, 405, "Method Not Allowed", this->_serverMap[client_fd]->error_path["405"]);
        }
        else
        {
            std::cerr << "Error Internal Servor Error" << std::endl;
            this->sendResponse(client_fd, 500, "Internal Server Error", this->_serverMap[client_fd]->error_path["500"]);
        }
    }
}

std::string Server::getContentType(const std::string &file_path)
{
    size_t find_dot = file_path.find_last_of('.');
    if (find_dot == std::string::npos)
        return "application/octet-stream";
    std::string ext = file_path.substr(find_dot);
    if (ext == ".html" || ext == ".htm")
        return "text/html";
    if (ext == ".css")
        return "text/css";
    if (ext == ".js")
        return "application/javascript";
    if (ext == ".jpg" || ext == ".jpeg")
        return "image/jpeg";
    if (ext == ".png")
        return "image/png";
    if (ext == ".gif")
        return "image/gif";
    if (ext == ".svg")
        return "image/svg+xml";
    if (ext == ".ico")
        return "image/x-icon";
    if (ext == ".json")
        return "application/json";
    if (ext == ".pdf")
        return "application/pdf";
    if (ext == ".txt")
        return "text/plain";
    return "application/octet-stream";
}

void Server::sendResponse(int client_fd, int status_code, const std::string &status_message, const std::string &file_path)
{
    FdGuard guard(client_fd);

    std::ifstream file(file_path.c_str(), std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Erreur: Impossible d'ouvrir " << file_path << std::endl;
        return this->sendResponse(client_fd, 500, "Internal Servor Error", this->_serverMap[client_fd]->error_path["500"]);
    }

    file.seekg(0, std::ios::end);
    long file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::string header = "HTTP/1.1 " + this->toString(status_code) + " " + status_message + "\r\n";
    header += "Content-Length: " + this->toString(file_size) + "\r\n";
    header += "Content-Type: " + this->getContentType(file_path) + "\r\n";
    header += "Connection: close\r\n\r\n";

    if (!this->sendAll(client_fd, header))
        return;

    char buffer[1024];
    while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0)
    {
        std::string chunk(buffer, file.gcount());
        if (!this->sendAll(client_fd, chunk))
            return;
    }
    file.close();
}


bool Server::sendAll(int fd_client, const std::string &data)
{
    const char *p = data.c_str();
    size_t remaining = data.size();
    
    while (remaining > 0)
    {
        ssize_t sent = send(fd_client, p, remaining, 0);
        if (sent == -1)
        {
            close(fd_client);
            return false;
        }
        else if (sent == 0)
        {
            close(fd_client);
            return true;
        }
        remaining -= sent;
        p += sent;
    }
    return true;
}

void Server::sendErrorResponse(int fd_client, int code, const std::string &message)
{
    std::string body = this->toString(code) + " " + message + "\r\n";
    std::string header = "HTTP/1.1 " + body +
                       "Content-Length: " + this->toString(body.size()) + "\r\n"
                       "Content-Type: text/plain\r\n"
                       "Connection: close\r\n\r\n";
    if(!this->sendAll(fd_client, header + body))
        return ;
    close(fd_client);
}

template <typename T>
std::string Server::toString(const T &value)
{
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

std::string    Server::getPathIndex(std::string full_file_path, InfoServer& currentServerInfo)
{
    std::string requestedPath;

    if (full_file_path[full_file_path.size() - 1] == '/')
        requestedPath = full_file_path.erase(full_file_path.size() - 1);
    else
        requestedPath = full_file_path;
    requestedPath = requestedPath.substr(1);
    for (std::vector<LocationConfig>::const_iterator it = currentServerInfo.locations.begin(); it != currentServerInfo.locations.end(); ++it)
    {
        std::string path = it->path;
        if (path[0] == '/')
            path = path.substr(1);
        if (path == requestedPath)
        {
            for (std::map<std::string, std::string>::const_iterator opt = it->options.begin(); opt != it->options.end(); ++opt)
            {
                if (opt->first == "index")
                    full_file_path = path + "/" + opt->second;
            }
        }
    }
    return full_file_path;
}

bool Server::autoIndexOn(InfoServer& currentServerInfo, std::string file_path)
{
    if (file_path[file_path.size() - 1] != '/')
        file_path += "/";
    bool autoIndex;
    for (std::vector<LocationConfig>::const_iterator it = currentServerInfo.locations.begin(); it != currentServerInfo.locations.end(); ++it)
    {
        std::string path = it->path;
        if (path[path.size() - 1] != '/')
            path += "/";
        if (path == file_path)
        {
            for (std::map<std::string, std::string>::const_iterator opt = it->options.begin(); opt != it->options.end(); ++opt)
            {
                if (opt->first == "autoindex")
                {
                    if (opt->second == "on")
                        autoIndex = true;
                    else
                        autoIndex = false;
                    return autoIndex;
                }
            }
        }
    }
    return false;
}

void Server::handleAutoIndex(int client_fd, std::string file_path)
{
    char pwd[1024];
    if (getcwd(pwd, sizeof(pwd)) == NULL)
    {
        std::cerr << "Error getting current directory\n";
        this->sendResponse(client_fd, 500, "Internal Server Error", this->_serverMap[client_fd]->error_path["500"]);
        return;
    }
    std::string res = std::string(pwd) + file_path;
    const char* dirName = res.c_str();
    DIR* dir = opendir(dirName);
    if (dir == NULL) {
        this->sendResponse(client_fd, 404, "Not Found", this->_serverMap[client_fd]->error_path["404"]);
        return;
    }

    std::ofstream htmlFile("html/autoindex.html");
    if (!htmlFile.is_open())
    {
        std::cerr << "Failed to create HTML file\n";
        closedir(dir);
        this->sendResponse(client_fd, 500, "Internal Server Error", this->_serverMap[client_fd]->error_path["500"]);
        return;
    }

    htmlFile << "<!DOCTYPE html>\n";
    htmlFile << "<html>\n<head>\n<title>Index of " << file_path << "</title>\n</head>\n<body>\n";
    htmlFile << "<h1>Index of " << file_path << "</h1>\n<ul>\n";

    struct dirent* entry = NULL;
    std::string entryName;
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_name[0] != '.')
        {
                if (file_path[file_path.size() - 1] != '/')
                    entryName = file_path + "/" + std::string(entry->d_name);
                else
                    entryName = std::string(entry->d_name);
            htmlFile << "<li><a href=\"" << entryName << "\">" << entry->d_name << "</a></li>\n";
        }
    }

    htmlFile << "</ul>\n</body>\n</html>\n";
    htmlFile.close();
    closedir(dir);
    this->sendResponse(client_fd, 200, "Success", "html/autoindex.html");
}





std::string Server::findRedirect(InfoServer& currentServerInfo, std::string file_path)
{
    if (file_path[file_path.size() - 1] != '/')
        file_path += "/";
    std::string redirect = "";
    for (std::vector<LocationConfig>::const_iterator it = currentServerInfo.locations.begin(); it != currentServerInfo.locations.end(); ++it)
    {
        std::string path = it->path;
        if (path[path.size() - 1] != '/')
            path += "/";
        
        if (path == file_path)
        {
            for (std::map<std::string, std::string>::const_iterator opt = it->options.begin(); opt != it->options.end(); ++opt)
            {
                if (opt->first == "return")
                {
                    redirect = opt->second;
                    return redirect;
                }
            }
        }
    }
    return redirect;
}

void Server::handleRedirect(int client_fd, std::string redirect)
{
    size_t findSpace = redirect.find(" ");
    if (findSpace != std::string::npos)
    {
        std::string codeStr = redirect.substr(0, findSpace);
        std::stringstream ss(codeStr);
        int code;
        ss >> code;
        std::string path = redirect.substr(findSpace + 1);
        if (path[0] == '/')
            path = path.substr(1);
        this->sendResponse(client_fd, code, "Redirect", path);
    }
    else
        this->sendResponse(client_fd, 404, "Not Found", this->_serverMap[client_fd]->error_path["404"]);
}




void    Server::processGet(int client_fd, Request &request, InfoServer& currentServerInfo)
{
    std::string file_path = request.getPath();
    std::string redirect = findRedirect(currentServerInfo, file_path);
    if (!redirect.empty())
    {
        return handleRedirect(client_fd, redirect);
    }
    if (autoIndexOn(currentServerInfo, file_path))
        return handleAutoIndex(client_fd, file_path);
    std::string full_file_path;
    if (file_path.compare("/") == 0)
        full_file_path = "html/index.html";
    else
        full_file_path = file_path;
    full_file_path = this->getPathIndex(full_file_path, currentServerInfo);
    if (full_file_path[0] == '/')
        full_file_path = full_file_path.substr(1);
    if (access(full_file_path.c_str(), F_OK) != 0)
        return this->sendResponse(client_fd, 404, "Not Found", this->_serverMap[client_fd]->error_path["404"]);
    if (access(full_file_path.c_str(), R_OK) != 0)
        return this->sendResponse(client_fd, 403, "Forbidden", this->_serverMap[client_fd]->error_path["403"]);
    this->sendResponse(client_fd, 200, "OK", full_file_path);
}

int Server::stringToInt(const std::string &str)
{
    std::istringstream iss(str);
    int value;
    iss >> value;
    if (iss.fail())
        std::cerr << "Error : Invalid number format: " << str << std::endl;
    return value;
}

void Server::processPost(int client_fd, Request &request, InfoServer& currentServerInfo)
{
    (void)currentServerInfo;
    try
    {
        std::string body = request.getBody();
        int size = stringToInt(request.getHeaders()["Content-Length"]);
        if (size >= this->_serverMap[client_fd]->client_max_body_size["default"])
            return this->sendResponse(client_fd, 413, "File to big", this->_serverMap[client_fd]->error_path["413"]);
        if (request.getHeaders()["Content-Type"].find("multipart/form-data") != std::string::npos)
            return this->handleFileUpload(client_fd, request);
        if (body.empty())
            return this->sendResponse(client_fd, 400, "Bad Request", this->_serverMap[client_fd]->error_path["400"]);
        std::ofstream outfile("html/uploaded_data.txt", std::ios::app);
        if (!outfile.is_open())
            return this->sendResponse(client_fd, 500, "Internal Server Error", this->_serverMap[client_fd]->error_path["500"]);
        outfile << body << std::endl;
        outfile.close();

        this->sendResponse(client_fd, 200, "OK", this->_serverMap[client_fd]->error_path["200"]);
        close(client_fd);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Erreur lors du traitement de la requete POST : " << e.what() << std::endl;
        this->sendResponse(client_fd, 500, "Internal Server Error", this->_serverMap[client_fd]->error_path["500"]);
    }
}

void Server::processDelete(int client_fd, Request &request, InfoServer& currentServerInfo)
{
    (void)currentServerInfo;
    std::string file_path = request.getPath().substr(1);
    
    if (access(file_path.c_str(), F_OK) != 0)
    {
        std::cerr << "Erreur : Le fichier [" << file_path << "] n'existe pas." << std::endl;
        return this->sendResponse(client_fd, 404, "Not Found", this->_serverMap[client_fd]->error_path["404"]);
    }
    if (access(file_path.c_str(), W_OK) != 0)
    {
        std::cerr << "Erreur : Le fichier [" << file_path << "]" << std::endl;
        return this->sendResponse(client_fd, 403, "Forbidden", this->_serverMap[client_fd]->error_path["403"]);
    }
    if (std::remove(file_path.c_str()) == 0)
    {
        std::cout << "Fichier supprimer avec succes : " << file_path << std::endl;
        this->sendResponse(client_fd, 200, "OK", this->_serverMap[client_fd]->error_path["200"]);
    }
    else
    {
        std::cerr << "Erreur lors de la suppression du fichier : " << file_path << std::endl;
        this->sendResponse(client_fd, 403, "Forbidden", this->_serverMap[client_fd]->error_path["403"]);
    }
}

void Server::handleFileUpload(int client_fd, Request &request)
{
    std::string content_type = request.getHeaders()["Content-Type"];
    std::string boundary_prefix = "boundary=";
    size_t boundary_pos = content_type.find(boundary_prefix);
    
    if (boundary_pos == std::string::npos)
    {
        std::cerr << "Error: Boundary not found in Content-Type header" << std::endl;
        std::cerr << "Content-Type: " << content_type << std::endl;
        return this->sendResponse(client_fd, 400, "Bad Request", this->_serverMap[client_fd]->error_path["400"]);
    }

    std::string body = request.getBody();
    size_t start = body.find("\r\n\r\n");
    if (start == std::string::npos)
        return this->sendResponse(client_fd, 400, "Bad Request", this->_serverMap[client_fd]->error_path["400"]);

    std::string file_content = body.substr(start + 4);
    size_t end = file_content.find("--");
    if (end != std::string::npos)
        file_content = file_content.substr(0, end);

    std::string tmp = request.getBody().substr(request.getBody().find("filename") + 10);
    std::string filename = tmp.substr(0, tmp.find("\""));

    if (filename.find("..") != std::string::npos || filename.find('/') != std::string::npos)
    {
        std::cerr << "Error: Invalid filename (contains relative paths or slashes)" << std::endl;
        return this->sendResponse(client_fd, 400, "Bad Request", this->_serverMap[client_fd]->error_path["400"]);
    }

    std::string upload_path = "file_uploaded/" + filename;
    struct stat file_info;
    if (stat(upload_path.c_str(), &file_info) == 0)
    {
        if (S_ISDIR(file_info.st_mode))
        {
            std::cerr << "Error: Upload path is a directory" << std::endl;
            return this->sendResponse(client_fd, 403, "FORBIDDEN", this->_serverMap[client_fd]->error_path["403"]);
        }
        std::cerr << "Error: File already exists" << std::endl;
        return this->sendResponse(client_fd, 409, "Conflict", this->_serverMap[client_fd]->error_path["409"]);
    }

    std::ofstream outfile(upload_path.c_str(), std::ios::binary);
    if (!outfile.is_open())
    {
        std::cerr << "Error: Could not open file for writing" << std::endl;
        return this->sendResponse(client_fd, 500, "Internal Server Error", this->_serverMap[client_fd]->error_path["500"]);
    }

    if (!outfile.write(file_content.c_str(), file_content.size()))
    {
        std::cerr << "Error: Failed to write file content" << std::endl;
        outfile.close();
        return this->sendResponse(client_fd, 500, "Internal Server Error", this->_serverMap[client_fd]->error_path["500"]);
    }

    outfile.close();
    this->sendResponse(client_fd, 200, "OK",this->_serverMap[client_fd]->error_path["200"]);
}

void    Server::processCGI(int client_fd, Request request, InfoServer& currentServerInfo)
{
    std::string scriptPath = request.getPath().substr(1);
    if (autoIndexOn(currentServerInfo, request.getPath()))
        return handleAutoIndex(client_fd, request.getPath());
    if (scriptPath == "cgi-bin")
        scriptPath = "cgi-bin/script.py";
    if (access(scriptPath.c_str(), X_OK) == -1)
    {
        std::cerr << "Error : the cgi script is not executable." << std::endl;
        return this->sendResponse(client_fd, 500, "Internal Server Error", currentServerInfo.error_path["500"]);
    }

    int outFd[2], inFd[2];
    if (pipe(outFd) == -1 || pipe(inFd) == -1)
    {
        std::cerr << "Error : the function pipe isn't working." << std::endl;
        return this->sendResponse(client_fd, 500, "Internal Server Error", currentServerInfo.error_path["500"]);
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        std::cerr << "Error : the function fork have a problem." << std::endl;
        return this->sendResponse(client_fd, 500, "Internal Server Error", currentServerInfo.error_path["500"]);
    }
    else if (pid == 0)
    {
        
        alarm(5);
        char pwd[1024];
        getcwd(pwd, sizeof(pwd));
        scriptPath = (std::string)pwd + "/" + scriptPath;

        std::string scriptDir = scriptPath.substr(0, scriptPath.find_last_of('/'));
        if (chdir(scriptDir.c_str()) == -1)
        {
            std::cerr << "Error : the chdir doesn't work." << std::endl;
            this->sendResponse(client_fd, 500, "Internal Server Error", currentServerInfo.error_path["500"]);
            exit(1);
        }
        
        if (dup2(outFd[1], STDOUT_FILENO) == -1 || dup2(inFd[0], STDIN_FILENO) == -1)
        {
            perror("dup2");
            exit(1);
        }
        close(inFd[1]);
        close(inFd[0]);
        close(outFd[0]);
        close(outFd[1]);

        std::vector<std::string> env_vars;
        env_vars.push_back("REQUEST_METHOD=" + request.getMethod());
        env_vars.push_back("QUERY_STRING=" + request.getQueryString());

        if (request.getHeaders().count("Content-Length"))
        {
            env_vars.push_back("CONTENT_LENGTH=" + request.getHeaders().at("Content-Length"));
        }

        std::vector<char*> envp;
        for (size_t i = 0; i < env_vars.size(); i++)
            envp.push_back(const_cast<char*>(env_vars[i].c_str()));
        envp.push_back(NULL);

        char *argv[] = {(char*)(scriptPath.c_str()), NULL};
        execve(argv[0], argv, envp.data());
        perror("execve");
        exit(1);
    }
    else
    {
        close(inFd[0]);
        close(outFd[1]);

        if (request.getMethod() == "POST")
        {
            std::string body = request.getBody();
            write(inFd[1], body.c_str(), body.size());
        }
        close(inFd[1]);
        
        int status;
        pid_t result = waitpid(pid, &status, WNOHANG);
        int elapsed_time = 0;
        
        while (result == 0 && elapsed_time < 5)
        {
            sleep(1);
            elapsed_time++;
            result = waitpid(pid, &status, WNOHANG);
        }

        if (result == 0)
        {
            std::cerr << "Error: CGI execution timeout.\n";
            kill(pid, SIGKILL);
            waitpid(pid, NULL, 0);
            return this->sendResponse(client_fd, 504, "Gateway Timeout", currentServerInfo.error_path["504"]);
        }
        else if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
        {
            std::cerr << "Error: CGI script exited with status " << WEXITSTATUS(status) << ".\n";
            return this->sendResponse(client_fd, 500, "Internal Server Error", currentServerInfo.error_path["500"]);
        }

        std::string responseBody;
        char buffer[1024];
        ssize_t bytesRead;

        while ((bytesRead = read(outFd[0], buffer, sizeof(buffer) - 1)) > 0)
        {
            buffer[bytesRead] = '\0';
            responseBody += buffer;
        }
        close(outFd[0]);

        std::size_t contentLengthPos = responseBody.find("Content-Length:");
        if (contentLengthPos != std::string::npos)
        {
            std::string contentLengthStr = responseBody.substr(contentLengthPos, responseBody.find("\r\n", contentLengthPos));
            std::cout << "CGI returned Content-Length: " << contentLengthStr << std::endl;
        }

        std::string http_response = 
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: " + toString(responseBody.size()) + "\r\n"
            "Content-Type: text/html\r\n\r\n" + responseBody;
        if (!this->sendAll(client_fd, http_response))
        {
            std::cerr << "Echec de l'envoi de la reponse CGI" << std::endl;
            return this->sendResponse(client_fd, 500, "Internal Server Error", currentServerInfo.error_path["500"]);
        }
    }
}