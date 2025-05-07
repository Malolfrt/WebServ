/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parsing.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mlefort <mlefort@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/16 13:47:28 by mlefort           #+#    #+#             */
/*   Updated: 2025/02/26 13:49:34 by mlefort          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "WebServ.hpp"

class Parsing
{
    public:

        Parsing();
        ~Parsing();

        void    parse(std::string filename);

        std::vector<InfoServer>& getServer();
        
        class ParseDidntEndWell : public std::exception
        {
            public:
                virtual const char* what() const throw()
                {
                    return "The Parsing finds something to stop the program !";
                }
        };
        
    private:
        void    extractBlock(std::ifstream &ifs);
            
        void    processBlock(const std::string &block, InfoServer &server);
            
        void    checkAllInfo(InfoServer  &currentServer);
        void    checkServerName(InfoServer &currentServer);
        void    checkHost(InfoServer &currentServer) const;
        void    checkPort(InfoServer &currentServer) const;
        void    checkBodySize(InfoServer &currentServer) const;
        void    checkPath(const std::string &path) const;
        void    checkAllowMethods(const std::string &method) const;
        void    checkAutoIndex(const std::string &index) const;
        void    checkReturn(const std::string &redirect);
        void    checkCGIPath(const std::string &paths) const;
        void    checkCGIExt(const std::string &ext) const;
        void    checkRoot(const std::string &rootPath) const;
        void    hadGoodExtension(std::string filename) const;
        void    checkBraces(std::ifstream &ifs);
        std::string trim(const std::string &str);
        std::string removeTrailingChar(const std::string &str, char c);
        std::pair<std::string, std::string> splitKeyValue(const std::string &str, char delimiter);
        int stringToInt(const std::string &str) const;
        bool isValidIPSegment(const std::string& segment) const;
        void    setErrorPath(InfoServer &currentServer);

        std::vector<InfoServer> _server;
};