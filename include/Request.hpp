/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mlefort <mlefort@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/01/27 16:41:39 by malo              #+#    #+#             */
/*   Updated: 2025/02/19 13:52:22 by mlefort          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "WebServ.hpp"

class Request
{
    public:
        Request();
        Request(const InfoServer &serverInfo);
        ~Request();
        
        int readRequest(int client_fd);
        int parseRequest(int client_fd);

        std::string getMethod() const;
        std::string getQueryString() const;
        std::string getHttpVersion() const;
        std::string getUrl() const;
        std::string getBody() const;
        bool getConnection() const;
        bool getMethodAllow() const;
        
        std::string getPath() const;
        std::map<std::string, std::string> getHeaders() const;


        class ParseHTTP : public std::exception
        {
            public:
                virtual const char* what() const throw()
                {
                    return "The Parsing of the request HTTP find something !";
                }
        };

    private:
        void parseHeaders();
        void parseUrl();
        const LocationConfig* findLocation(const std::string &path) const;
        void parseAllowedMethods();
        
        std::string _method;
        std::string _http_version;
        std::string _url;
        std::string _body;
        bool _keep_alive;
        bool _method_allow;
        std::string _queryString;
        
        std::string _path;
        std::map<std::string, std::string> _headers;
        std::string _raw_request;
        std::map<std::string, std::string>  _allowed_methods;
        InfoServer  _serverInfo;
};