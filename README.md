# 🌐 Webserv - Projet 42

## 📌 Description

**Webserv** est un projet système avancé de l'école 42 consistant à implémenter un **serveur HTTP** en C++98, conforme à la norme **HTTP/1.1**.  
L'objectif est de comprendre les **mécanismes du web**, de la gestion des connexions à la réponse HTTP, en passant par le traitement de fichiers statiques, les méthodes HTTP, et les CGI.

C'est un projet complexe qui mêle des connaissances réseau, système, et web.

---

## 🧠 Objectifs pédagogiques

- Implémenter un **serveur web multiclient** avec `poll`
- Gérer les **méthodes HTTP** : `GET`, `POST`, `DELETE`
- Traiter les **fichiers de configuration personnalisés**
- Servir des fichiers statiques (HTML, CSS, images…)
- Implémenter un support pour les **CGI** (scripts exécutables)
- Gérer les **erreurs HTTP**, les **redirections**, et les **timeouts**
- Être conforme à la norme **RFC 2616** (HTTP/1.1)

---

## ⚙️ Fonctionnalités principales

- Multiplexage d'I/O avec `poll()`
- Serveur capable de gérer plusieurs hôtes/ports
- Parsing d’un fichier de configuration inspiré de celui de Nginx
- Support de plusieurs connexions clients simultanées
- Exécution de scripts CGI (ex: Python)
- Gestion de l’upload de fichiers avec la méthode `POST`
- Gestion des erreurs HTTP (404, 405, 500, etc.)
- Pages d'erreur personnalisées
- Redirections (`301`, `302`)
- Indexation automatique de répertoires (`autoindex`)
- Timeout et fermeture de connexions inactives

---

## 📂 Tester le projet

```bash
$ git clone https://github.com/vmondor/webserv.git
  cd webserv
  make run
```

Tapez localhost:8082 dans la barre de recherche de votre navigateur
