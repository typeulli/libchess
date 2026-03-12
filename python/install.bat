@echo off
py ..\collect.py python
pip install .
rmdir /s /q build
rmdir /s /q libchess
rmdir /s /q libchess.egg-info