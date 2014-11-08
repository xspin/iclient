@echo off
title=iClient -ver1.0.n
color 02
rem ##########################################
echo 按 Ctrl-C 可停止程序
echo 其他说明请看 Readme

rem ##########################################

echo.
echo Ready to Authenticate...
echo.

rem 设置用户名
set USERNAME=unm

rem 设置密码
set PASSWORD=pwd

rem 设置IP
set IP=172.26.43.235
rem 如果手动设置IP就在下面加上参数（--ip %IP%）

iClient -u %USERNAME% -p %PASSWORD% 
rem --ip %IP%

echo.

