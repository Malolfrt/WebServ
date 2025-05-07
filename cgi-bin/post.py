#!/usr/bin/env python3

import sys
import os
import json
import urllib.parse

print("Content-Type: application/json\n")

content_length = os.getenv("CONTENT_LENGTH")
if content_length:
    try:
        content_length = int(content_length)
        post_data = sys.stdin.read(content_length)
        parsed_data = urllib.parse.parse_qs(post_data)

        # Transformer en JSON
        response = {key: value[0] for key, value in parsed_data.items()}
        print(json.dumps(response, indent=4))
    except ValueError:
        print(json.dumps({"error": "Invalid CONTENT_LENGTH"}))
else:
    print(json.dumps({"error": "No CONTENT_LENGTH provided"}))