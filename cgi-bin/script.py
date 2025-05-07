#!/usr/bin/env python3

print("Content-Type: text/html\n")

html_content = """<!DOCTYPE html>
<html lang="fr">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Page CGI</title>
    <style>
        body {
            background-color: #34495e;
            color: #ecf0f1;
            font-family: 'Arial', sans-serif;
            text-align: center;
            display: flex;
            flex-direction: column;
            justify-content: center;
            align-items: center;
            height: 100vh;
            margin: 0;
            box-sizing: border-box;
        }
        h1 {
            font-size: 3.5rem;
            margin-bottom: 20px;
            text-shadow: 2px 2px 10px rgba(0, 0, 0, 0.2);
        }
        h2 {
            font-size: 2rem;
            margin-bottom: 30px;
        }
        .container {
            background-color: #2c3e50;
            padding: 40px;
            border-radius: 8px;
            box-shadow: 0px 4px 6px rgba(0, 0, 0, 0.3);
            width: 90%;
            max-width: 500px;
        }
        .btn {
            margin-top: 20px;
            display: inline-block;
            padding: 12px 20px;
            font-size: 1.2rem;
            border: 2px solid #3498db;
            border-radius: 5px;
            text-decoration: none;
            color: #3498db;
            transition: background-color 0.3s, color 0.3s;
            cursor: pointer;
        }
        .btn:hover {
            background-color: #3498db;
            color: white;
        }
        input {
            width: 80%;
            padding: 10px;
            margin: 10px 0;
            border: 1px solid #3498db;
            border-radius: 5px;
            background: #34495e;
            color: #ecf0f1;
            font-size: 1.2rem;
        }
    </style>
</head>
<body>
    <h1>Bienvenue sur la page CGI</h1>
    <h2><em>- Script Dynamique -</em></h2>
    <a class="btn" href="../">Retour à l'accueil</a>
</body>
</html>"""

print(html_content)
