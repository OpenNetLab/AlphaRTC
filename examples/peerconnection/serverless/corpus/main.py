#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import subprocess

import cmdinfer


def main():
    app = subprocess.Popen(
        ["peerconnection_serverless"] + sys.argv[1:],
        bufsize=1,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT)
    cmdinfer.main(app.stdout, app.stdin)
    app.wait()


if __name__ == "__main__":
    main()
