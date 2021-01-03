echo off

set FILE_TO_SIGN=%~dpnx1

C:\signtool\sign.bat "%FILE_TO_SIGN%"
