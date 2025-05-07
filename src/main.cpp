/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mlefort <mlefort@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/15 13:06:07 by mlefort           #+#    #+#             */
/*   Updated: 2025/02/13 11:36:14 by mlefort          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Parsing.hpp"
#include "../include/WebServ.hpp"

int gServerStop = 0;

void printInfoServer(std::vector<InfoServer> &server)
{
    for (size_t i = 0; i < server.size(); i++)
    {
        std::cout << std::endl;
        std::cout << "\t---PRINT INFO SERVER---" << i << "\t" << std::endl;
        std::cout << std::endl;

        std::cout << "Server Name: " << server[i].server_name << std::endl;
        std::cout << "Host: " << server[i].host << std::endl;

        std::cout << "Server Ports: " << std::endl;
        std::cout << "  Port : " << server[i].server_port["default"] << std::endl;

        std::cout << "Error Paths: " << std::endl;
        for (std::map<std::string, std::string>::const_iterator it = server[i].error_path.begin(); it != server[i].error_path.end(); ++it)
        {
            std::cout << "  " << it->first << ": " << it->second << std::endl;
        }

        std::cout << "Client Max Body Sizes: " << std::endl;
        std::cout << "  Default : " << server[i].client_max_body_size["default"] << std::endl;

        std::cout << "Root Paths: " << std::endl;
        for (std::map<std::string, std::string>::const_iterator it = server[i].root_path.begin(); it != server[i].root_path.end(); ++it)
        {
            std::cout << "  " << it->first << ": " << it->second << std::endl;
        }

        std::cout << "Index Files: " << std::endl;
        for (std::map<std::string, std::string>::const_iterator it = server[i].index_files.begin(); it != server[i].index_files.end(); ++it)
        {
            std::cout << "  " << it->first << ": " << it->second << std::endl;
        }

        std::cout << "Location Blocks: " << std::endl;
        for (std::vector<LocationConfig>::const_iterator it = server[i].locations.begin(); it != server[i].locations.end(); ++it) {
            std::cout << "  Path: " << it->path << std::endl;
            for (std::map<std::string, std::string>::const_iterator opt = it->options.begin(); opt != it->options.end(); ++opt) {
                std::cout << "    " << opt->first << ": " << opt->second << std::endl;
            }
        }
    }
}


void handle_sigint(int sig)
{
    (void)sig;
    gServerStop = 1;
}

int    startParsing(std::string filename, std::vector<InfoServer> &server)
{
    Parsing parser;

    try
    {
        parser.parse(filename);
    }
    catch(const std::exception & e)
    {
        std::cerr << e.what() << '\n';
        return 0;
    }

    server = parser.getServer();
    return 1;
}


int main( int ac, char **av )
{
    if (ac != 2)
    {
        std::cerr << "Error : wrong argument for configuration file." << std::endl;
        return 1;
    }
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, handle_sigint);
    std::vector<InfoServer> serverInfo;
    if (!startParsing((std::string)av[1], serverInfo))
        return 1;
    printInfoServer(serverInfo);
    try
    {   
        Server server(serverInfo);
        server.start();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Erreur inattendue : " << e.what() << std::endl;
        return 1;
    }
    return (0);
}
