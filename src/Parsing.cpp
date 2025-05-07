/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parsing.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mlefort <mlefort@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/16 13:47:24 by mlefort           #+#    #+#             */
/*   Updated: 2025/02/26 17:24:16 by mlefort          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Parsing.hpp"

Parsing::Parsing():  _server()
{
    
}

Parsing::~Parsing()
{
    
}

void    Parsing::parse(std::string filename)
{
    hadGoodExtension(filename);
    std::ifstream ifs(filename.c_str());
    if (!ifs)
    {
        std::cerr << "Error : unable to open configuration file." << std::endl;
        throw ParseDidntEndWell();
    }
    checkBraces(ifs);
    extractBlock(ifs);
}

void    Parsing::extractBlock(std::ifstream &ifs)
{
    ifs.clear();
    ifs.seekg(0, std::ios::beg);
    std::stack<char>            braces;
    std::string                 currentBlock;
    std::string                 line;

    while (std::getline(ifs, line))
    {
        line = trim(line);
        if (line.empty() || line[0] == '#')
            continue ;
        currentBlock += line + '\n';
        for (size_t i = 0; i < line.size(); i++)
        {
            char c = line[i];
            if (c == '{')
                braces.push(c);
            else if (c == '}')
            {
                if (!braces.empty())
                    braces.pop();
                else
                {
                    std::cerr << "Error : Unmatched closing brace '}'." << std::endl;
                    throw ParseDidntEndWell();
                }
            }
        }
        if (braces.empty() && !currentBlock.empty())
        {
            InfoServer newServer;
            processBlock(currentBlock, newServer);
            currentBlock.clear();
            checkAllInfo(newServer);
            setErrorPath(newServer);
            this->_server.push_back(newServer);
        }
    }
    if (!braces.empty())
    {
        std::cerr << "Error : Unmatched opening brace '{'." << std::endl;
        throw ParseDidntEndWell();
    }
}

void    Parsing::processBlock(const std::string &block, InfoServer &server)
{
    std::istringstream iss(block);
    std::string line;
    bool insideLocation = false;
    LocationConfig  currentLocation;
    
    while (std::getline(iss, line))
    {
        line = trim(line);
        if (line.empty() || line[0] == '#')
            continue ;

        if (line.find("location") == 0)
        {
            if (insideLocation)
            {
                server.locations.push_back(currentLocation);
                currentLocation = LocationConfig();
            }
            insideLocation = true;
            currentLocation.path = line.substr(8);
            currentLocation.path = trim(currentLocation.path);
            continue;
        }

        if (line.find('{') != std::string::npos || line.find('}') != std::string::npos)
        {
            if (insideLocation && line.find('}') != std::string::npos)
            {
                server.locations.push_back(currentLocation);
                currentLocation = LocationConfig();
                insideLocation = false;
            }
            continue; 
        }    
        
        if (insideLocation)
        {
            if (line.find(';') == std::string::npos)
            {
                std::cerr << "Error : Missing ';' at the end of line: " << line << std::endl;
                throw ParseDidntEndWell();
            }
            line = removeTrailingChar(line, ';');
            std::pair<std::string, std::string> keyValue = splitKeyValue(line, ' ');
            currentLocation.options[keyValue.first] = keyValue.second;
        }
        else
        {
            if (line.find(';') == std::string::npos)
            {
                std::cerr << "Error : Missing ';' at the end of line: " << line << std::endl;
                throw ParseDidntEndWell();
            }
            line = removeTrailingChar(line, ';');
            std::pair<std::string, std::string> keyValue = splitKeyValue(line, ' ');
            if (keyValue.first == "server_name")
                server.server_name = keyValue.second;
            else if (keyValue.first == "host")
                server.host = keyValue.second;
            else if (keyValue.first == "listen")
            {
                if (server.server_port["default"] != 0)
                {
                    std::cerr << "Error : multiply definition for listen." << std::endl;
                    throw ParseDidntEndWell();
                }
                int port = stringToInt(keyValue.second);
                server.server_port["default"] = port;
            }
            else if (keyValue.first == "root")
                server.root_path["default"] = keyValue.second;
            else if (keyValue.first == "index")
                server.index_files["default"] = keyValue.second;
            else if (keyValue.first == "error_page")
            {
                std::pair<std::string, std::string> errorPair = splitKeyValue(keyValue.second, ' ');
                server.error_path[errorPair.first] = errorPair.second; 
            }
            else if (keyValue.first == "client_max_body_size")
            {
                int body_size = stringToInt(keyValue.second);
                server.client_max_body_size["default"] = body_size;
            }
        }
    }
    if (insideLocation)
        server.locations.push_back(currentLocation);
}

void    Parsing::checkAllInfo(InfoServer  &currentServer)
{
    checkServerName(currentServer);
    checkHost(currentServer);
    checkPort(currentServer);
    checkBodySize(currentServer);
    std::map<std::string, std::string>::const_iterator rootIt = currentServer.root_path.find("default");
    if (rootIt != currentServer.root_path.end())
        checkRoot(rootIt->second);
    std::map<std::string, std::string>::const_iterator indexIt = currentServer.index_files.find("default");
    if (indexIt != currentServer.index_files.end())
        checkPath(indexIt->second);
    for (std::map<std::string, std::string>::const_iterator it = currentServer.error_path.begin(); it != currentServer.error_path.end(); it++)
        checkPath(it->second);

    for (size_t i = 0; i < currentServer.locations.size(); i++)
    {
        const LocationConfig& currentLocation = currentServer.locations[i];
        std::string path = currentLocation.path;
        path = trim(path);
        path = removeTrailingChar(path, '{');
        path = trim(path);
        currentServer.locations[i].path = path;
        checkRoot(path);
        for (std::map<std::string, std::string>::const_iterator it = currentLocation.options.begin(); it != currentLocation.options.end(); it++)
        {
            if (it->first == "allow_methods")
                checkAllowMethods(it->second);
            else if (it->first == "autoindex")
                checkAutoIndex(it->second);
            else if (it->first == "index")
            {
                if (path[0] == '/')
                    path = path.substr(1);
                std::string tmp = path + "/" + it->second;
                checkPath(tmp);
            }
            else if (it->first == "return")
                checkReturn(it->second);
            else if (it->first == "root")
                checkRoot(it->second);
            else if (it->first == "cgi_path")
                checkCGIPath(it->second);
            else if (it->first == "cgi_ext")
                checkCGIExt(it->second);
        }
    }
}

void    Parsing::checkCGIExt(const std::string &ext) const
{
    if (ext.empty())
    {
        std::cerr << "Error : cgi_ext option is empty." << std::endl;
        throw ParseDidntEndWell();
    }
    std::istringstream iss(ext);
    std::string extension;
    while (iss >> extension)
    {
        if (extension[0] != '.')
        {
            std::cerr << "Error : Invalid CGI extension : " << extension << std::endl;
            throw ParseDidntEndWell();
        }
    }
}


void    Parsing::checkCGIPath(const std::string &paths) const
{
  if (paths.empty())
    {
        std::cerr << "Error : cgi_path option is empty." << std::endl;
        throw ParseDidntEndWell();
    }
    std::istringstream iss(paths);
    std::string path;
    while (iss >> path)
        checkPath(path);
}

void Parsing::checkReturn(const std::string &redirect)
{
    if (redirect.empty())
    {
        std::cerr << "Error : return option is empty." << std::endl;
        throw ParseDidntEndWell();
    }
    std::pair<std::string,std::string> tmp = splitKeyValue(redirect, ' ');
    if (tmp.first.empty() || tmp.second.empty())
    {
        std::cerr << "Error : return option is empty." << std::endl;
        throw ParseDidntEndWell();
    }
    for (size_t i = 0; i < tmp.first.size(); i++)
    {
        if(!isdigit(tmp.first[i]))
        {        
            std::cerr << "Error : the return's code isn't good : " << tmp.first << std::endl;
            throw ParseDidntEndWell();    
        }
    }
    if (tmp.second[0] == '/')
        tmp.second = tmp.second.substr(1);
    if (access(tmp.second.c_str(), F_OK) == -1)
    {
        std::cerr << "Error : the return's file doesn't exist : " << tmp.second << std::endl;
        throw ParseDidntEndWell();   
    }
}

void    Parsing::checkAutoIndex(const std::string &index) const
{
    if (index.empty())
    {
        std::cerr << "Error : auto index options is empty." << std::endl;
        throw ParseDidntEndWell();
    }

    if (index != "on" && index != "off")
    {
        std::cerr << "Error : Invalid choice of options for auto index : " << index << std::endl;
        throw ParseDidntEndWell();
    }
}

void    Parsing::checkAllowMethods(const std::string &method) const
{
    const std::string validMethod[] = {"GET", "POST", "DELETE"};
    std::istringstream iss(method);
    std::string currentMethod;
    if (method.empty())
    {
        std::cerr << "Error : allowed methods is empty." << std::endl;
        throw ParseDidntEndWell();
    }
    while (iss >> currentMethod)
    {
        bool isValid = false;
        for (size_t i = 0; i < 3; i++)
        {
            if (currentMethod == validMethod[i])
            {
                isValid = true;
                break ;
            }
        }
        if (!isValid)
        {
            std::cerr << "Error : Invalid method : " << currentMethod << std::endl;
            throw ParseDidntEndWell();
        }
    }
}

void    Parsing::checkServerName(InfoServer &currentServer) 
{
    if (currentServer.server_name.empty() || currentServer.server_name.length() > 255)
    {
        std::cerr << "Error : Invalid Server Name format." << "---" << currentServer.server_name << "---" << std::endl; 
        throw ParseDidntEndWell(); 
    }
    for (size_t i = 0; i < currentServer.server_name.length(); i++)
    {
        char c = currentServer.server_name[i];

        if ((i == 0 || i == currentServer.server_name.length() - 1) && !isalnum(c))
        {
            std::cerr << "Error : Server Name must start and end with an alphanumeric character." << std::endl;
            throw ParseDidntEndWell();
        }
        
        if (!isalnum(c) && c != '-' && c != '.')
        {
            std::cerr << "Error : Invalid character '" << c << "' in Server Name." << std::endl;
            throw ParseDidntEndWell();
        }

        if ((c == '-' || c == '.') && (i == 0 || i == currentServer.server_name.length() - 1 || 
                    currentServer.server_name[i + 1] == '-' || currentServer.server_name[i + 1] == '.'))
        {
            std::cerr << "Error : Server Name contains consecutive or misplaced '-' or '.'." << std::endl;
            throw ParseDidntEndWell();
        }
    }
    for (std::vector<InfoServer>::const_iterator it = this->_server.begin(); it != this->_server.end(); it++)
    {
        if (it->server_name == currentServer.server_name)
        {
            std::cerr << "Error : server with the same name." << std::endl;
            throw ParseDidntEndWell();
        }
    }

}

void    Parsing::checkHost(InfoServer &currentServer) const
{
    if (currentServer.host.empty())
    {
        std::cerr << "Error : Host cannot be empty." << std::endl;
        throw ParseDidntEndWell(); 
    }

    size_t start = 0;
    size_t end = 0;
    int segmentCount = 0;

    while ((end = currentServer.host.find('.', start)) != std::string::npos)
    {
        std::string segment = currentServer.host.substr(start, end - start);
        if (!isValidIPSegment(segment))
        {
            std::cerr << "Error : Invalid segment in Host: " << segment << std::endl;
            throw ParseDidntEndWell();
        }
        start = end + 1;
        segmentCount++;
    }

    std::string lastSegment = currentServer.host.substr(start);
    if (!isValidIPSegment(lastSegment))
    {
        std::cerr << "Error : Invalid segment in Host: " << lastSegment << std::endl;
        throw ParseDidntEndWell();
    }
    segmentCount++;

    if (segmentCount != 4)
    {
        std::cerr << "Error : Host does not contain exactly 4 segments." << std::endl;
        throw ParseDidntEndWell();
    }
    for (std::vector<InfoServer>::const_iterator it = this->_server.begin(); it != this->_server.end(); it++)
    {
        if (it->host == currentServer.host)
        {
            std::cerr << "Error : server with the same host." << std::endl;
            throw ParseDidntEndWell();
        }
    }
}

void    Parsing::checkPort(InfoServer &currentServer) const
{
    if (currentServer.server_port.empty())
    {
        std::cerr << "Error : No port specified in 'listen'." << std::endl;
        throw ParseDidntEndWell(); 
    }
    
    for (std::map<std::string, int>::const_iterator it = currentServer.server_port.begin(); it != currentServer.server_port.end(); it++)
    {
        int port = it->second;
        if (port < 1 || port > 65535)
        {
            std::cerr << "Error : Port out of range (1-65535)." << port  << std::endl;
            throw ParseDidntEndWell(); 
        }
    }
    for (std::vector<InfoServer>::const_iterator it = this->_server.begin(); it != this->_server.end(); it++)
    {
        if (it->server_port == currentServer.server_port)
        {
            std::cerr << "Error : server with the same port." << std::endl;
            throw ParseDidntEndWell();
        }
    }
}

void    Parsing::checkBodySize(InfoServer &currentServer) const
{
    if (currentServer.client_max_body_size.empty())
    {
        std::cerr << "Error : client_max_body_size cannot be empty." << std::endl;
        throw ParseDidntEndWell();
    }
    for (std::map<std::string, int>::const_iterator it = currentServer.client_max_body_size.begin(); it != currentServer.client_max_body_size.end(); it++)
    {
        int bodySize = it->second;
        if (bodySize <= 0)
        {
            std::cerr << "Error : client_max_body_size must be a positive integer. Invalid value: " << bodySize << std::endl;
            throw ParseDidntEndWell();
        }
    }
}

void    Parsing::checkPath(const std::string &path) const
{
    struct stat pathStat;
    if (stat(path.c_str(), &pathStat) != 0)
    {
        std::cerr << "Error : Path does not exist: " << path << std::endl;
        throw ParseDidntEndWell();
    }
    if (S_ISDIR(pathStat.st_mode))
    {
        std::cerr << "Error : Path  is a directory." << path << std::endl;
        throw ParseDidntEndWell();
    }
    if (access(path.c_str(), R_OK) != 0)
    {
        std::cerr << "Error : Cannot access path (read permission denied): " << path << std::endl;
        throw ParseDidntEndWell();
    }
}

void    Parsing::checkRoot(const std::string &rootPath) const
{
    std::string tmp = rootPath;
    if (rootPath.size() != 1 && rootPath[0] == '/')
        tmp = rootPath.substr(1);
    if (tmp.empty())
    {
        std::cerr << "Error: Root path is empty." << std::endl;
        throw ParseDidntEndWell();
    }
    struct stat pathStat;
    if (stat(tmp.c_str(), &pathStat) != 0)
    {
        std::cerr << "Error: Unable to access root path: " << tmp << std::endl;
        throw ParseDidntEndWell();
    }
    if (!S_ISDIR(pathStat.st_mode))
    {
        std::cerr << "Error: Root path is not a directory: " << tmp << std::endl;
        throw ParseDidntEndWell();
    }
}

std::vector<InfoServer>& Parsing::getServer()
{
    return this->_server;
}

void    Parsing::checkBraces(std::ifstream &ifs)
{
    int countBraces = 0;
    std::string line;

    while (std::getline(ifs, line))
    {
        for (size_t i = 0; i < line.size(); i++)
        {
            if (line[i] == '{')
                countBraces++;
            else if (line[i] == '}')
                countBraces--;

            if (countBraces < 0)
            {
                std::cerr << "Error : Unexpected closing brace '}' in configuration file." << std::endl;
                throw ParseDidntEndWell();
            }
        }
    }

    if (countBraces != 0)
    {
        std::cerr << "Error : Unmatched braces in configuration file."  << std::endl;
        throw ParseDidntEndWell();
    }
}

void    Parsing::hadGoodExtension(std::string filename) const
{
    std::size_t dotPosition = filename.rfind('.');
    if (dotPosition == std::string::npos)
    {
        std::cerr << "Error : the configuration file doesn't have an extension." << std::endl;
        throw ParseDidntEndWell();
    }
    std::string extension = filename.substr(dotPosition);
    if (extension != ".conf")
    {
        std::cerr << "Error : the configuration file didn't have the correct extension." << std::endl;
        throw ParseDidntEndWell();
    }
}

std::string Parsing::trim(const std::string &str)
{
    size_t first = str.find_first_not_of(" \t");
    if (first == std::string::npos)
        return "";
    size_t last = str.find_last_not_of(" \t");
    return str.substr(first, (last - first + 1));
}

std::string Parsing::removeTrailingChar(const std::string &str, char c)
{
    if (!str.empty() && str[str.size() - 1] == c)
        return str.substr(0, str.size() - 1);
    return str;
}

std::pair<std::string, std::string> Parsing::splitKeyValue(const std::string &str, char delimiter)
{
    size_t pos = str.find(delimiter);
    if (pos == std::string::npos)
    {
        std::cerr << "Error : Syntax error at line : " << str << std::endl;
        throw ParseDidntEndWell();
    }
    std::string key = str.substr(0, pos);
    std::string value = str.substr(pos + 1);
    key = trim(key);
    value = trim(value);
    return std::make_pair(key, value);
}

int Parsing::stringToInt(const std::string &str) const
{
    std::istringstream iss(str);
    int value;
    iss >> value;
    if (iss.fail())
    {
        std::cerr << "Error : Invalid number format: " << str << std::endl;
        throw ParseDidntEndWell();
    }
    return value;
}

bool Parsing::isValidIPSegment(const std::string& segment) const
{
    if (segment.empty() || segment.length() > 3)
        return false;

    for (size_t i = 0; i < segment.length(); ++i)
    {
        if (!isdigit(segment[i]))
            return false;
    }

    int value = stringToInt(segment);
    return value >= 0 && value <= 255;
}

void Parsing::setErrorPath(InfoServer &currentServer)
{
    std::map<std::string, std::string> defaultErrors;
    defaultErrors["200"] = "html/success.html";
    defaultErrors["400"] = "html/error400.html";
    defaultErrors["403"] = "html/error403.html";
    defaultErrors["404"] = "html/error404.html";
    defaultErrors["405"] = "html/error405.html";
    defaultErrors["409"] = "html/error409.html";
    defaultErrors["413"] = "html/error413.html";
    defaultErrors["500"] = "html/error500.html";
    defaultErrors["504"] = "html/error504.html";

    for (std::map<std::string, std::string>::iterator it = defaultErrors.begin(); it != defaultErrors.end(); ++it)
    {
        if (currentServer.error_path[it->first].empty())
        {
            currentServer.error_path[it->first] = it->second;
        }
    }
}