#!/bin/bash
./dabd >fromDABD <toDABD &
./dabgui.py <fromDABD >toDABD
