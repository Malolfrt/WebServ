/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   WebServ.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mlefort <mlefort@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/15 13:05:22 by mlefort           #+#    #+#             */
/*   Updated: 2025/02/26 17:49:40 by mlefort          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#define PATH_TO_CONF_FILE   "./config/"
#define PATH_TO_RACINE      "../"

/*      GLOBAL      */
#include <cstdio>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <string>
#include <fcntl.h>
#include <string.h>
#include <fstream>
#include <cstdlib>
#include <dirent.h>


/*      SOCKET/RESEAU       */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/wait.h>

/*      SIGNAL      */
#include <csignal>

/*      PARSING     */
#include <map>
#include <list>
#include <fstream>
#include <stack>
#include <vector>
#include <sstream>
#include <utility>
#include <sys/stat.h>

struct LocationConfig
{
    std::string                         path;
    std::map<std::string, std::string>  options;
};

struct InfoServer
{
    std::string                         server_name;
    std::string                         host;
    std::map<std::string, int>          server_port;     
    std::map<std::string, std::string>  error_path;       
    std::map<std::string, int>          client_max_body_size; 
    std::map<std::string, std::string>  root_path;
    std::map<std::string, std::string>  index_files;
    std::vector<LocationConfig>         locations;
};

struct FdGuard
{
    int fd_;
    FdGuard(int fd) : fd_(fd) {}
    ~FdGuard() { if (fd_ != -1) close(fd_); }
};

#include <exception>

#include "Server.hpp"
#include "Request.hpp"

extern int gServerStop;

#define TIMEOUT 5000