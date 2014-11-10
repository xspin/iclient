##iClient v1.0 for windows DCSM客户端

-- 运行iClient.exe的参数说明:

  帐户设置:

	-u, --username        设置用户名

	-p, --password        设置密码

  可选参数:

	-g, --gateway         设置网关 

	-d, --dns             设置DNS地址 

	--device              设置网卡，默认为eth0

	--dhcp                使用DHCP（实际上没用）

	--ip                  设置IP地址

	--mask                设置子网掩码

	-l                    注销下线命令

	-h, --help            显示帮助
---------------------------------------------------------

可将用户名、密码等参数写入run.bat中再运行。（可惜密码是明文的T.T）

-- info.exe
    查看网卡信息

-- logoff.bat
    注销脚本

-- run.bat
    运行脚本

-- RunInBKG.bat
    后台运行

-- WARNING
    实际上，显示的网卡信息不一定与要使用的网卡信息相同（因为显示与使用是不同的方式实现）。
    不能连上时，请把除了本地连接外的其他都禁用，可以连上后再启用。
