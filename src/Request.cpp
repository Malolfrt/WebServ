#include "../include/Request.hpp"

Request::Request(const InfoServer &serverInfo) :  _method(""), _http_version(""), _url(""), _body(""), _keep_alive(false), _path(""),_raw_request(""), _serverInfo(serverInfo)
{
    
}

Request::Request()
{

}

Request::~Request()
{

}

int Request::readRequest(int client_fd)
{
    char buffer[1024];
    int bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
    if (bytes_read == -1)
    {
        return -1;
    }
    if (bytes_read == 0)
    {
        return 0;
    }
    else
    {
        buffer[bytes_read] = '\0';
        this->_raw_request = std::string(buffer);
        return 1;
    }
}

std::string Request::getMethod() const
{
    return this->_method;
}

std::string Request::getPath() const
{
    return this->_path;
}

std::string Request::getUrl() const
{
    return this->_url;
}

std::string Request::getHttpVersion() const
{
    return this->_http_version;
}

std::string Request::getBody() const
{
    return this->_body;
}

std::string Request::getQueryString() const
{
    return this->_queryString;
}

std::map<std::string, std::string> Request::getHeaders() const
{
    return this->_headers;
}

bool Request::getConnection() const
{
    return this->_keep_alive;
}

bool Request::getMethodAllow() const
{
    return this->_method_allow;
}


int Request::parseRequest(int client_fd)
{
    std::istringstream stream(this->_raw_request);
    std::string line;

    if (std::getline(stream, line))
    {
        std::istringstream line_stream(line);
        line_stream >> this->_method >> this->_url >> this->_http_version;
        if (this->_http_version != "HTTP/1.1")
        {
            std::cerr << "Error : this is not the good version for HTTP : " << this->_http_version << std::endl;
            throw ParseHTTP();
        }
        parseUrl();
    }

    this->parseHeaders();

    while (std::getline(stream, line) && line != "\r")
        continue;
    if (this->_headers.find("Content-Length") != this->_headers.end())
    {
        std::string content_type = this->getHeaders()["Content-Type"];
        if (content_type.find("multipart/form-data") != std::string::npos)
        {
            size_t content_length = std::atoi(this->_headers["Content-Length"].c_str());
            std::string body;
            char buffer[4096];
            size_t total_received = 0;

            while (total_received < content_length)
            {
                ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
                if (bytes_received == 0)
                {
                    std::cerr << "Error: Connection closed" << std::endl;
                    return 0;
                }
                if (bytes_received == -1)
                {
                    std::cerr << "Error: No reception data" << std::endl;
                    return -1;
                }
                buffer[bytes_received] = '\0';  
                body.append(buffer, bytes_received);
                total_received += bytes_received;
                if (body.find("------WebKitFormBoundary") == 0)
                    continue;
                else
                    break;
                if (body.size() >= (size_t)this->_serverInfo.client_max_body_size["default"])
                    break;
            }
            this->_body = body;
        }
        else
        {
            int content_length = std::atoi(this->_headers["Content-Length"].c_str());
            std::string body(content_length, '\0');
            stream.read(&body[0], content_length);
            this->_body = body;
        }
    }

    if (this->_headers.find("Connection") != this->_headers.end())
    {
        if (this->_headers["Connection"] == "keep-alive\r")
            this->_keep_alive = true;
        else
            this->_keep_alive = false;

    }
    this->parseAllowedMethods();
    return 1;
}

void    Request::parseUrl()
{
    if (this->_url.empty())
    {
         std::cerr << "Error : this is not the good version for HTTP : " << this->_http_version << std::endl;
        throw ParseHTTP();
    }

    size_t query_pos = this->_url.find('?');
    if (query_pos != std::string::npos)
    {
        this->_queryString = this->_url.substr(query_pos + 1);
        
        this->_path = this->_url.substr(0, query_pos);
        std::string query = this->_url.substr(query_pos + 1);
        std::istringstream query_stream(query);
        std::string param;

        while (std::getline(query_stream, param, '&'))
        {
            size_t pos = param.find('=');
            if (pos != std::string::npos)
            {
                std::string key = param.substr(0, pos);
                std::string value = param.substr(pos + 1);
                if (key.find(' ') != std::string::npos || value.find(' ') != std::string::npos)
                {
                    std::cerr << "Error: Invalid query parameter: " << key << "=" << value << std::endl;
                    throw ParseHTTP();
                }
                if (key.find('\t') != std::string::npos || value.find('\t') != std::string::npos)
                {
                    std::cerr << "Error: Invalid query parameter: " << key << "=" << value << std::endl;
                    throw ParseHTTP();
                }
            }
        }
    }
    else
        this->_path = this->_url;

    if (this->_path == "/favicon.ico")
        return ;
    
    char pwd[1024];
    getcwd(pwd, 1024);
    std::string res = (std::string)pwd + "/html" + this->_path;
    if (this->_path.empty() || this->_path[0] != '/')
    {
        std::cerr << "Error : the url syntax is wrong : " << this->_url << std::endl;
        throw ParseHTTP();
    }
}

void    Request::parseHeaders()
{
    std::istringstream stream(this->_raw_request);
    std::string line;

    std::getline(stream, line);

    while (std::getline(stream, line) && line != "\r")
    {
        size_t pos = line.find(": ");
        if (pos != std::string::npos)
        {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 2);
            this->_headers[key] = value;
        }
    }
}


const LocationConfig* Request::findLocation(const std::string &path) const
{
   const LocationConfig *best_match = NULL;
   size_t best_match_length = 0;

   for (std::vector<LocationConfig>::const_iterator it = this->_serverInfo.locations.begin();
        it != this->_serverInfo.locations.end(); it++)
    {
        if (path.find(it->path) == 0 && it->path.length() > best_match_length)
        {
            best_match = &(*it);
            best_match_length = it->path.length();
        }
    }

    if (!best_match)
    {
        for (std::vector<LocationConfig>::const_iterator it = this->_serverInfo.locations.begin();
             it != this->_serverInfo.locations.end(); it++)
        {
            if (it->path == "/")
            {
                best_match = &(*it);
                break;
            }
        }
    }
    return best_match;
}

void Request::parseAllowedMethods()
{
    const LocationConfig *location = this->findLocation(this->_path);
    if (!location)
    {
        std::cerr << "Error : no matching location for path : " << this->_path << std::endl;
        throw ParseHTTP();
    }

    std::map<std::string, std::string>::const_iterator it = location->options.find("allow_methods");
    if (it == location->options.end())
    {
            if (this->_method != "GET")
            {
                std::cerr << "Error : this method is not allowed in this location : " << this->_method << std::endl;
                throw ParseHTTP();
            }
            else
                return ;
    }

    this->_method_allow = false;
    if (it->second.find(this->_method) != std::string::npos)
        this->_method_allow = true;
    if (!this->_method_allow)
    {
        std::cerr << "Error : this method is not allowed in this location : " << this->_method << std::endl;
        throw ParseHTTP();
    }
}