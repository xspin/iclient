#iClient1.0 for Linux 说明

##编译：
    
    编译需要libpcap库，一般Linux发行版里面安装libpcap包即可，在ubuntu中，需要libpcap-dev：

        > sudo apt-get install libpcap-dev

    编译命令：

        > make
    
##安装：
    
    在安装前，请用户先编辑运行脚本文件runiclient，将其中的user和pass分别修改成您的帐号和密码并保存。 

    安装需要root权限，这通常使用sudo或者su -c

    > sudo ./install 

    安装程序会复制核心程序iclient以及用户脚本runiclient到系统目录/usr/bin，并设置相关属性，如果用户希望安装到其他目录，可给出目的路径，如sudo ./install /usr/local/bin，但请保证目的目录在系统PATH环境变量内。 

    成功执行安装将看到####Installation Done.####的提示。 

    make install也可完成安装，同样有make uninstall以供卸载。再次提醒安装前先修改runiclient文件内的账户信息。 

##运行：
	
    如果用户配置的帐号信息无误并且安装成功，那么用户只需要运行runiclient，即可看到有关的认证成功的信息。加上参数‘-b’可后台运行。

##终止：
    
    用户执行一次`runiclient -l`或'iclient -l'，即可成功离线。 

##其他：

    当用户使用的认证网卡不是默认的第一个网卡（如eth0）时，可使用runiclient --dev eth1这样的参数方式启动程序，或者修改runiclient文件内ARGS=""，加入自定义的参数。 

------
https://github.com/xspin/iclient

xspin<xnipse@gmail.com>
