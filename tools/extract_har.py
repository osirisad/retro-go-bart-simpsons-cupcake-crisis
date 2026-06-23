#!/usr/bin/env python3
"""Unpack an itch.io HAR capture into ignore/har_extracted/."""
from __future__ import annotations

import argparse
import base64
import json
import os
import re
from urllib.parse import unquote, urlparse


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("har", help="Path to .har file")
    parser.add_argument(
        "-o",
        "--out",
        default=os.path.join(os.path.dirname(__file__), "..", "ignore", "har_extracted"),
    )
    args = parser.parse_args()
    out_dir = os.path.abspath(args.out)
    os.makedirs(out_dir, exist_ok=True)

    with open(args.har, encoding="utf-8") as f:
        har = json.load(f)

    saved = 0
    for entry in har["log"]["entries"]:
        content = entry["response"].get("content", {})
        text = content.get("text", "")
        if not text:
            continue
        data = (
            base64.b64decode(text)
            if content.get("encoding") == "base64"
            else text.encode("utf-8")
        )
        url = entry["request"]["url"]
        m = re.search(r"/html/\d+/(.*)", url)
        rel = m.group(1) if m else urlparse(url).path.lstrip("/")
        rel = unquote(rel.split("?")[0])
        path = os.path.join(out_dir, rel.replace("/", os.sep))
        os.makedirs(os.path.dirname(path) or out_dir, exist_ok=True)
        with open(path, "wb") as f:
            f.write(data)
        saved += 1

    print(f"Wrote {saved} files to {out_dir}")


if __name__ == "__main__":
    main()
