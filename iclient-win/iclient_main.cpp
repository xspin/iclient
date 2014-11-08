/*
 * =====================================================================================
 *
 *       Filename:  idclient_main.cpp
 *
 *    Description:  main function source file for iClient
 *
 *        Version:  1.0
 *        Created:  11/11/2014
 *       Revision:  none
 *
 *         Author:  xspin<code@e-spin.org>
 *
 * =====================================================================================
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include "iclient.h"

static void signal_interrupted (int signo);

extern pcap_t      *handle;
extern int          exit_flag;

static void
signal_interrupted (int signo)
{
    fprintf(stdout,"\n&&Info: USER Interrupted. \n");
    send_eap_packet(EAPOL_LOGOFF);
    pcap_breakloop (handle);
}

void show_alldevs(pcap_if_t *alldevs, char* errbuf)  
{  
    pcap_if_t *d;  
    struct in_addr net_ip_address;//网卡IP信息,在pcap.h里面有定义  
    uint32_t net_ip;  
    char *net_ip_string;  
  
    struct in_addr net_mask_address;  
    uint32_t net_mask;  
    char *net_mask_string;  
  
    int i=0;  
    /* 扫描列表 */  
    for(d=alldevs; d; d=d->next)  
    {  
        printf("MAC: %s\n",d->name);  
        printf("Description: %s\n",d->description);  
        pcap_lookupnet(d->name,&net_ip,&net_mask,errbuf);  
  
        net_ip_address.s_addr = net_ip;  
        net_ip_string = inet_ntoa(net_ip_address);//format  
        printf("IP: %s \n",net_ip_string);  
      
        net_mask_address.s_addr = net_mask;  
        net_mask_string = inet_ntoa(net_mask_address);//format  
        printf("MASK: %s \n",net_mask_string);  
        printf("\n");  
    }  
  
    /* 释放链表 */  
    pcap_freealldevs(alldevs);  
    printf("\n");  
}  

int main(int argc, char **argv)
{
    HANDLE m_hMutex = CreateMutex(NULL, true , "{F*CK-YOU-AND-ANYTING}");
    init_arguments (&argc, &argv);
    if(exit_flag)
    {
        if (GetLastError() == ERROR_ALREADY_EXISTS)
        {
            CloseHandle(m_hMutex);
            m_hMutex = NULL;
            fprintf(stdout,"\n&&Info: CloseHandle. \n");
        }
        //初始化设备，打开网卡，获得Mac、IP等信息
        init_device();

        //初始化用户信息
        //init_info();

        //初始化发送帧的缓冲区
        init_frames ();
        send_eap_packet(EAPOL_LOGOFF);
        pcap_breakloop (handle);
        fprintf(stdout,"\n&&Info: Logoff & Exit. \n");
        exit(1);
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        //AfxMessageBox(_T( "应用程序不可以重复启动" ),MB_OK | MB_APPLMODAL |MB_ICONSTOP);
        fprintf(stderr, "\n@@ERROR: Client Already Running!!!\n");
        exit(EXIT_FAILURE);
    }
    //初始化并解释程序的启动参数
    //init_arguments (&argc, &argv);

    //初始化设备，打开网卡，获得Mac、IP等信息
    init_device();

    //初始化用户信息
    init_info();

    //初始化发送帧的缓冲区
    init_frames ();

    signal (SIGINT, signal_interrupted);
    signal (SIGTERM, signal_interrupted);    
    show_local_info();

    send_eap_packet (EAPOL_LOGOFF);
    //发出第一个上线请求报文
    send_eap_packet (EAPOL_START);

    //进入回呼循环。以后的动作由回呼函数get_packet驱动，
    //直到pcap_break_loop执行，退出程序。
	pcap_loop (handle, -1, get_packet, NULL);   /* main loop */
    pcap_close (handle);
    system("pause");
    return 0;
}




