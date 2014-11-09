/*
 * =====================================================================================
 *
 *       Filename:  idclient.cpp
 *
 *    Description:  main source file for iClient
 *
 *        Version:  1.0
 *        Created:  11/11/2014
 *       Revision:  none
 *
 *         Author:  xspin<code@e-spin.org>
 *
 * =====================================================================================
 */
#include "iclient.h"

//#include <assert.h>

/* #####   GLOBLE VAR DEFINITIONS   ######################### */
/*-----------------------------------------------------------------------------
 *  程序的主控制变量
 *-----------------------------------------------------------------------------*/
#define AUTH_TIMES 100

int auth_times = 0;
char        errbuf[PCAP_ERRBUF_SIZE];  /* error buffer */
enum STATE  state;                     /* program state */
pcap_t      *handle;			   /* packet capture handle */
uint8_t      muticast_mac[] =            /* 802.1x的认证服务器多播地址 */
                        {0x01, 0x80, 0xc2, 0x00, 0x00, 0x03};


/* #####   GLOBLE VAR DEFINITIONS   ###################
 *-----------------------------------------------------------------------------
 *  用户信息的赋值变量，由init_argument函数初始化
 *-----------------------------------------------------------------------------*/
int         dhcp_on = 0;               /* DHCP 模式标记 */
int         background = 0;            /* 后台运行标记  */
char        devs_len = '0';
char        *dev = NULL;               /* 连接的设备名 */
char        *username = NULL;
char        *password = NULL;
char        *user_gateway = NULL;      /* 由用户设定的四个报文参数 */
char        *user_dns = NULL;
char        *user_ip = NULL;
char        *user_mask = NULL;
char        *client_ver = NULL;         /* 报文协议版本号 */
int         exit_flag = 0;

/* #####   GLOBLE VAR DEFINITIONS   #########################
 *-----------------------------------------------------------------------------
 *  报文相关信息变量，由init_info 、init_device函数初始化。
 *-----------------------------------------------------------------------------*/
char        dev_if_name[64];
size_t      username_length;
size_t      password_length;
uint32_t       local_ip;			       /* 网卡IP，网络序，下同 */
uint32_t       local_mask;			       /* subnet mask */
uint32_t       local_gateway = 0;
uint32_t       local_dns = 0;
uint8_t      local_mac[ETHER_ADDR_LEN]; /* MAC地址 */

//int         use_pseudo_ip = 0;          [> DHCP模式网卡无IP情况下使用伪IP的标志 <]

/* #####   TYPE DEFINITIONS   ######################### */
/*-----------------------------------------------------------------------------
 *  报文缓冲区，由init_frame函数初始化。
 *-----------------------------------------------------------------------------*/
uint8_t      eapol_start[108];            /* EAPOL START报文 */
uint8_t      eapol_logoff[108];           /* EAPOL LogOff报文 */
uint8_t      *eap_response_ident = NULL; /* EAP RESPON/IDENTITY报文 */
uint8_t      *eap_response_md5ch = NULL; /* EAP RESPON/MD5 报文 */
uint8_t      *eap_response_md5keep = NULL; /* EAP RESPON/MD5 报文 */

// debug function
void
print_hex(uint8_t *array, int count)
{
    printf("\nHex data input. Length = %d", count);
    int i;
    for(i = 0; i < count; i++){
        if ( !(i % 16))
            printf ("\n");
        printf("%02x ", array[i]);
    }
    printf("\n");
}

void
show_usage()
{
    printf( "\n"
            "iClient %s \n"
            "\t  -- Supllicant for DigiChina Authentication.\n"
            "\n"
            "  Usage:\n"
            "\tRun with your account info in arguments:\n\n"
            "\t-u, --username           Your username.\n"
            "\t-p, --password           Your password.\n"
            "\n"
            "  Optional Arguments:\n\n"
            "\t-g, --gateway         Specify Gateway server address. \n\n"

            "\t-d, --dns             Specify DNS server address. \n\n"

            "\t--ip                  Specify IP server address. \n\n"

            "\t--device              Specify which device to use.\n"
            "\t                      Default is usually eth0.\n\n"

            "\t--dhcp                Use DHCP mode if your ISP requests.\n"
            "\t                      You may need to run `dhclient' manualy to\n"
            "\t                      renew your IP address after successful \n"
            "\t                      authentication.\n\n"

            "\t--mask                packet to the server with an IP and MASK, use \n"
            "\t                      this arguments to specify them, or program will\n"
            "\t                      use a pseudo one.  Affacts only when both promoted.\n\n"

            //"\t-l                    Tell the process to Logoff.\n\n"

            "\t-h, --help            Show this help.\n\n"
            "\n"
            "  About iClient:\n\n"
            "\tThis program is a C/C++ implementation to DigiChina Authentication,\n"

            "\t\t\t\t\t\t\t\t2014.09.01\n",
            ZDC_VER);
}


/*
 * ===  FUNCTION  ======================================================================
 *         Name:  get_md5_digest
 *  Description:  calcuate for md5 digest
 * =====================================================================================
 */
char*
get_md5_digest(const char* str, size_t len)
{
    static md5_byte_t digest[16];
	md5_state_t state;
	md5_init(&state);
	md5_append(&state, (const md5_byte_t *)str, len);
	md5_finish(&state, digest);

    return (char*)digest;
}


/*
 * ===  FUNCTION  ======================================================================
 *         Name:  get_eap_type
 *  Description:  根据报文的动作位返回enum EAPType内定义的报文类型
 * =====================================================================================
 */
enum EAPType
get_eap_type(const struct eap_header *eap_header)
{
    switch (eap_header->eap_t){
        case 0x01:
            if ( eap_header->eap_op == 0x01 &&
                        eap_header->eap_id == 0x03 )
                return EAP_REQUEST_IDENTITY_KEEP_ALIVE;
            if ( eap_header->eap_op == 0x01)
                return EAP_REQUEST_IDENTITY;
            if ( eap_header->eap_op == 0x04)
                return EAP_REQUETS_MD5_CHALLENGE;
            if ( eap_header->eap_op == 0xfa)
                return EAP_REQUEST_MD5_KEEP_ALIVE;

            break;
        case 0x03:
        //    if (eap_header->eap_id == 0x02)
            return EAP_SUCCESS;
            break;
        case 0x04:
            return EAP_FAILURE;
    }
    fprintf (stderr, "&&IMPORTANT: Unknown Package : eap_t:     %02x\n"
                    "                               eap_id:     %02x\n"
                    "                               eap_op:     %02x\n",
                    eap_header->eap_t, eap_header->eap_id,
                    eap_header->eap_op);
    return EAP_ERROR;
}


/*
 * ===  FUNCTION  ======================================================================
 *         Name:  action_by_eap_type
 *  Description:  根据eap报文的类型完成相关的应答
 * =====================================================================================
 */
void
action_by_eap_type(enum EAPType pType,
                        const struct eap_header *header,
                        const struct pcap_pkthdr *packetinfo,
                        const uint8_t *packet) {
//    printf("PackType: %d\n", pType);
    switch(pType){
        case EAP_SUCCESS:
            state = ONLINE;
            fprintf(stdout, ">>Protocol: EAP_SUCCESS\n");
            fprintf(stdout, "---\n&&Info: Authorized Access to Network. \n");
            print_server_info (packet, packetinfo->caplen);
            if (background){
                background = 0;         /* 防止以后误触发 */
                //daemon_init();
            }
            break;
        case EAP_FAILURE:
            state = READY;
            fprintf(stdout, ">>Protocol: EAP_FAILURE\n");
            if(state == ONLINE){
                fprintf(stdout, "&&Info: SERVER Forced Logoff\n");
            }
            if (state == STARTED){
                fprintf(stdout, "&&Info: Invalid Username or Client info mismatch.\n");
            }
            if (state == ID_AUTHED){
                fprintf(stdout, "&&Info: Invalid Password.\n");
            }
            print_server_info (packet, packetinfo->caplen);
            if(auth_times > AUTH_TIMES)
                pcap_breakloop (handle);
            else
                reauthorize(&auth_times);
            break;
        case EAP_REQUEST_IDENTITY:
            if (state == STARTED){
                fprintf(stdout, ">>Protocol: REQUEST EAP-Identity\n");
            }
            send_eap_packet(EAP_RESPONSE_IDENTITY);
            break;
        case EAP_REQUETS_MD5_CHALLENGE:
            state = ID_AUTHED;
            fprintf(stdout, ">>Protocol: REQUEST MD5-Challenge(PASSWORD)\n");
            fill_password_md5((uint8_t*)header->eap_md5_challenge, header->eap_id);
            send_eap_packet(EAP_RESPONSE_MD5_CHALLENGE);
            break;
        case EAP_REQUEST_IDENTITY_KEEP_ALIVE:
            if (state == ONLINE){
                fprintf(stdout, ">>Protocol: REQUEST EAP_REQUEST_IDENTITY_KEEP_ALIVE\n");
             break;
            }

           // 使用伪IP模式认证成功后，获取真实IP，并写入RES/IDTY数据块
           //if (use_pseudo_ip){
               //若获取成功，关闭伪IP模式标签
               //if (set_device_new_ip() == 0) {
                   //use_pseudo_ip = 0;
               //}
           //}

            send_eap_packet(EAP_RESPONSE_IDENTITY_KEEP_ALIVE);
            break;
        case EAP_REQUEST_MD5_KEEP_ALIVE:
            fprintf(stdout, ">>Protocol: REQUEST ZD Private KEEP MD5(USERNAME)\n");
            fill_uname_md5((uint8_t*)header->eap_md5_challenge-1, header->eap_id);
            send_eap_packet(EAP_REQUEST_MD5_KEEP_ALIVE);
            break;
        default:
            pcap_breakloop (handle);
            return;
    }
}


/*
 * ===  FUNCTION  ======================================================================
 *         Name:  send_eap_packet
 *  Description:  根据eap类型发送相应数据包
 * =====================================================================================
 */
void
send_eap_packet(enum EAPType send_type)
{
    uint8_t *frame_data;
    int     frame_length = 0;
    switch(send_type){
        case EAPOL_START:
            state = STARTED;
            frame_data= eapol_start;
            frame_length = 108;
            fprintf(stdout, ">>Protocol: SEND EAPOL-Start\n");
            break;
        case EAPOL_LOGOFF:
            state = READY;
            frame_data = eapol_logoff;
            frame_length = 108;
            fprintf(stdout, ">>Protocol: SEND EAPOL-Logoff\n");
            break;
        case EAP_RESPONSE_IDENTITY:
            frame_data = eap_response_ident;
            frame_length = 14 + 9 + username_length + 72;
            /* Hack ! KEEP_ALIVE报文跟RESP_IDNT报文只有这一个字节区别 */
            if (*(frame_data + 14 + 5) != 0x01){
                *(frame_data + 14 + 5) = 0x01;
            }
            fprintf(stdout, ">>Protocol: SEND EAP-Response/Identity\n");
            break;
        case EAP_RESPONSE_MD5_CHALLENGE:
            frame_data = eap_response_md5ch;
            frame_length = 14 + 10 + 16 + username_length + 72;
            fprintf(stdout, ">>Protocol: SEND EAP-Response/Md5-Challenge\n");
            break;
        case EAP_RESPONSE_IDENTITY_KEEP_ALIVE:
            frame_data = eap_response_ident;
            frame_length = 14 + 9 + username_length + 72;
            /* Hack ! KEEP_ALIVE报文跟RESP_IDNT报文只有这一个字节区别 */
            if (*(frame_data + 14 + 5) != 0x03){
                *(frame_data + 14 + 5) = 0x03;
            }
            fprintf(stdout, ">>Protocol: SEND EAP_RESPONSE_IDENTITY_KEEP_ALIVE\n");
            break;
        case EAP_REQUEST_MD5_KEEP_ALIVE:
            frame_data = eap_response_md5keep;
            frame_length = 14 + 9 + 16 + 72;
            break;
        default:
            fprintf(stderr,"&&IMPORTANT: Wrong Send Request Type.%02x\n", send_type);
            return;
    }
    if (pcap_sendpacket(handle, frame_data, frame_length) != 0)
    {
        fprintf(stderr,"&&IMPORTANT: Error Sending the packet: %s\n", pcap_geterr(handle));
        return;
    }
}


/*
 * ===  FUNCTION  ======================================================================
 *         Name:  get_packet
 *  Description:  pcap的回呼函数，当收到EAPOL报文时自动被调用
 * =====================================================================================
 */
void
get_packet(uint8_t *args, const struct pcap_pkthdr *header,
    const uint8_t *packet)
{
	/* declare pointers to packet headers */
    const struct ether_header *ethernet;  /* The ethernet header [1] */
    const struct eap_header *eap_header;

    ethernet = (struct ether_header*)(packet);
    eap_header = (struct eap_header *)(packet + SIZE_ETHERNET);

    enum EAPType p_type = get_eap_type(eap_header);
    action_by_eap_type(p_type, eap_header, header, packet);
    return;
}


/*
 * ===  FUNCTION  ======================================================================
 *         Name:  init_frames
 *  Description:  初始化发送帧的数据
 * =====================================================================================
 */
void
init_frames()
{
    int data_index;

    /*****  EAPOL Header  *******/
    uint8_t eapol_eth_header[SIZE_ETHERNET];
    struct ether_header *eth = (struct ether_header *)eapol_eth_header;
    //struct ether_header *eth = (struct ether_header *) new BYTE[SIZE_ETHERNET];
    memcpy (eth->ether_dhost, muticast_mac, 6);
    memcpy (eth->ether_shost, local_mac, 6);
    eth->ether_type =  htons (0x888e);

    uint8_t start_logoff_tail[15*6-2] = {0};
    /**** EAPol START ****/
    uint8_t start_data[4] = {0x01, 0x01, 0x00, 0x00};
    memcpy (eapol_start, eapol_eth_header, 14);
    memcpy (eapol_start + 14, start_data, 4);
    memcpy (eapol_start + 18, start_logoff_tail, 88);

    /****EAPol LOGOFF ****/
    uint8_t logoff_data[4] = {0x01, 0x02, 0x00, 0x00};
    memcpy (eapol_logoff, eapol_eth_header, 14);
    memcpy (eapol_logoff + 14, logoff_data, 4);
    memcpy (eapol_logoff + 18, start_logoff_tail, 88);

    //print_hex(eapol_start, 6*16);
    /****DCBA Private Info Tailer ***/
    uint8_t local_info_tailer[72] = {0};
    int tail_len = 72;

    local_info_tailer[0] = dhcp_on;

    struct dcba_tailer *dcba_info_tailer =
                (struct dcba_tailer *)(local_info_tailer + 1);

    dcba_info_tailer->local_ip          = local_ip;
    dcba_info_tailer->local_mask        = local_mask;
    dcba_info_tailer->local_gateway     = local_gateway;
    dcba_info_tailer->local_dns         = local_dns;

    char* username_md5 = get_md5_digest(username, username_length);
    memcpy (dcba_info_tailer->username_md5, username_md5, 16);

    /*strncpy ((char*)dcba_info_tailer->client_ver, client_ver, 13);*/
    memset((char*)dcba_info_tailer->tail, 0, 39);
    dcba_info_tailer->tail[15] = 0xc1;
    dcba_info_tailer->tail[16] = 0x18;
    dcba_info_tailer->tail[38] = 0x40;

   //print_hex (local_info_tailer, tail_len);

    /* EAP RESPONSE IDENTITY */
    uint8_t eap_resp_iden_head[9] = {0x01, 0x00,
                                    0x00, 5 + tail_len + username_length,  /* eapol_length */
                                    0x02, 0x01,
                                    0x00, 5 + username_length,       /* eap_length */
                                    0x01};

    eap_response_ident = (uint8_t*)malloc (14 + 9 + username_length + tail_len);
    memset (eap_response_ident, 0, 14 + 9 + username_length + tail_len);

    data_index = 0;
    memcpy (eap_response_ident + data_index, eapol_eth_header, 14);
    data_index += 14;
    memcpy (eap_response_ident + data_index, eap_resp_iden_head, 9);
    data_index += 9;
    memcpy (eap_response_ident + data_index, username, username_length);
    data_index += username_length;
    memcpy (eap_response_ident + data_index, local_info_tailer, tail_len);

   /*print_hex (eap_response_ident, 14 + 9 + username_length + tail_len);*/

    /** EAP RESPONSE MD5 Challenge **/
    uint8_t eap_resp_md5_head[10] = {0x01, 0x00,
                                   0x00, 6 + 16 + username_length + tail_len, /* eapol-length */
                                   0x02, 0x00,
                                   0x00, 6 + 16 + username_length, /* eap-length */
                                   0x04, 0x10};
    eap_response_md5ch = (uint8_t*)malloc (14 + 4 + 6 + 16 + username_length + tail_len);

    data_index = 0;
    memcpy (eap_response_md5ch + data_index, eapol_eth_header, 14);
    data_index += 14;
    memcpy (eap_response_md5ch + data_index, eap_resp_md5_head, 10);
    data_index += 26;// 剩余16位在收到REQ/MD5报文后由fill_password_md5填充
    memcpy (eap_response_md5ch + data_index, username, username_length);
    data_index += username_length;
    memcpy (eap_response_md5ch + data_index, local_info_tailer, tail_len);

    /** EAP RESPONSE MD5 Keep Alive **/
    uint8_t eap_resp_md5keep_head[9] = {0x01, 0x00,
                                   0x00, 5 + 16 + 33, /* eapol-length */
                                   0x02, 0xff,
                                   0x00, 5 + 16, /* eap-length */
                                   0xfa};
    eap_response_md5keep = (uint8_t*)malloc (14 + 4 + 5 + 16 + tail_len);


    data_index = 0;
    memcpy (eap_response_md5keep + data_index, eapol_eth_header, 14);
    data_index += 14;
    memcpy (eap_response_md5keep + data_index, eap_resp_md5keep_head, 9);
    data_index += 25;// 剩余16位在收到REQ/MD5报文后由fill_keey_md5填充
    memcpy (eap_response_md5keep + data_index, local_info_tailer, tail_len);

   /*print_hex (eap_response_md5ch, 14 + 4 + 6 + 16 + username_length + tail_len);*/
}

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  fill_password_md5
 *  Description:  给RESPONSE_MD5_Challenge报文填充相应的MD5值。
 *  只会在接受到REQUEST_MD5_Challenge报文之后才进行，因为需要
 *  其中的Key
 * =====================================================================================
 */
void
fill_password_md5(uint8_t attach_key[], uint8_t eap_id)
{
    char *psw_key;
    char *md5;

    psw_key = (char*)malloc(1 + password_length + 16);
    psw_key[0] = eap_id;
    memcpy (psw_key + 1, password, password_length);
    memcpy (psw_key + 1 + password_length, attach_key, 16);

    md5 = get_md5_digest(psw_key, 1 + password_length + 16);
    memcpy (eap_response_md5ch + 14 + 10, md5, 16);

    free (psw_key);
}

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  fill_uname_md5
 *  Description:  给RESPONSE_MD5_KEEP_ALIVE报文填充相应的MD5值。
 *  只会在接受到REQUEST_MD5_KEEP_ALIVE报文之后才进行，因为需要
 *  其中的Key
 * =====================================================================================
 */
void
fill_uname_md5(uint8_t attach_key[], uint8_t eap_id)
{
    char *uname_key;
    char *md5;

    uname_key = (char*)malloc(username_length + 4);
    memcpy (uname_key, username, username_length);
    memcpy (uname_key + username_length, attach_key, 4);

    md5 = get_md5_digest(uname_key,username_length + 4);
    eap_response_md5keep[14+5]=eap_id;
    memcpy (eap_response_md5keep + 13 + 10, md5, 16);

    free (uname_key);
}


/*
 * ===  FUNCTION  ======================================================================
 *         Name:  init_info
 *  Description:  初始化本地信息。
 * =====================================================================================
 */
void init_info()
{
    if(username == NULL || password == NULL){
        fprintf (stderr,"Error: NO Username or Password promoted.\n"
                        "Try iClient --help for usage.\n");
        system("pause");
        cout<<endl;
        username = (char*)malloc(sizeof(char)*16);
        password = (char*)malloc(sizeof(char)*32);
        cout<<"Username: ";
        cin>>username;
        cout<<"Password: ";
        cin>>password;
        //exit(EXIT_FAILURE);
    }
    username_length = strlen(username);
    password_length = strlen(password);

    if (user_ip)
    {
        local_ip = inet_addr (user_ip);
        //cout<<"&&Info: set IP"<<endl;
    }
    //else
        //local_ip = 0;

    if (user_mask)
        local_mask = inet_addr (user_mask);
    //else
        //local_mask = inet_addr;

    if (user_gateway)
        local_gateway = inet_addr (user_gateway);
    //else
        //local_gateway = 0;

    if (user_dns)
        local_dns = inet_addr (user_dns);
    //else
        //local_dns = inet_addr("4.4.4.4");

    if (local_ip == -1 || local_mask == -1 || local_gateway == -1 || local_dns == -1) {
        fprintf (stderr,"ERROR: One of specified IP, MASK, Gateway and DNS address\n"
                        "in the arguments format error.\n");
        exit(EXIT_FAILURE);
    }

    if(client_ver == NULL)
        //client_ver = "3.5.05.0617fk";
        client_ver = "1.0.n";
    else{
        if (strlen (client_ver) > 13) {
            fprintf (stderr, "Error: Specified client version `%s' longer than 13 Bytes.\n"
                    "Try `iclient --help' for more information.\n", client_ver);
            exit(EXIT_FAILURE);
        }
    }
}


/*
 * ===  FUNCTION  ======================================================================
 *         Name:  init_device
 *  Description:  初始化设备。主要是找到打开网卡、获取网卡MAC、IP，
 *  同时设置pcap的初始化工作句柄。
 * =====================================================================================
 */
void init_device()
{
    struct          bpf_program fp;			/* compiled filter program (expression) */
    char            filter_exp[51];         /* filter expression [3] */
    pcap_if_t       *alldevs;
    pcap_addr_t     *addrs;
    PIP_ADAPTER_INFO pIpAdapterInfo = new IP_ADAPTER_INFO();
    //local_mac[0] = 0xf0; 
    //local_mac[1] = 0xde; 
    //local_mac[2] = 0xf1; 
    //local_mac[3] = 0xae; 
    //local_mac[4] = 0x08; 
    //local_mac[5] = 0x05; 
	/* Retrieve the device list */
	if(pcap_findalldevs(&alldevs, errbuf) == -1)
	{
		fprintf(stderr,"Error in pcap_findalldevs: %s\n", errbuf);
		exit(1);
	}
    //show_alldevs(alldevs, errbuf);
    getAdapters(pIpAdapterInfo);

    char dev_no = '0';
    if(devs_len>'1')
    {
        cout<<"\n*** Please Select a Adapter (Default eth0). "<<endl;
        cout<<"*** Input the number: ";
        dev_no = getchar();
    }


    if(dev_no < '0' or dev_no >= devs_len)
    {
        dev_no = '0';
        if(dev_no != '\n' or dev_no != NULL)
            cout<<"Ivalid Input, Using Default."<<endl;
    }
    int no = dev_no - '0';
    for(int i=0; i<no; i++)
    {
        alldevs = alldevs->next;
        pIpAdapterInfo = pIpAdapterInfo->Next;
    }
    cout<<"\n----> Selected [[[eth"<<no<<"]]]\n"<<endl;

    dev = alldevs->name;
    //dev = pIpAdapterInfo->AdapterName;
    //cout<<dev<<endl;
    //cout<<alldevs->name<<endl;
    strcpy (dev_if_name, dev);

	if (dev == NULL) {
		fprintf(stderr, "Couldn't find default device: %s\n", errbuf);
		exit(EXIT_FAILURE);
    }

	/* open capture device */
	handle = pcap_open_live(dev, SNAP_LEN, 1, 1000, errbuf);

	if (handle == NULL) {
		fprintf(stderr, "Couldn't open device %s: %s\n", dev, errbuf);
		exit(EXIT_FAILURE);
	}

	/* make sure we're capturing on an Ethernet device [2] */
	if (pcap_datalink(handle) != DLT_EN10MB) {
		fprintf(stderr, "%s is not an Ethernet\n", dev);
		exit(EXIT_FAILURE);
	}

    for (int i = 0; i < 6; i++)
        local_mac[i] = pIpAdapterInfo->Address[i];
    /* Get IP ADDR and MASK */
    //for (addrs = alldevs->addresses; addrs; addrs=addrs->next) {
        //if (addrs->addr->sa_family == AF_INET) {
            //local_ip = ((struct sockaddr_in *)addrs->addr)->sin_addr.s_addr;
            //local_mask = ((struct sockaddr_in *)addrs->netmask)->sin_addr.s_addr;
        //}
    //}
    //addrs = alldevs->addresses;
    //local_ip = ((struct sockaddr_in *)addrs->addr)->sin_addr.s_addr;
    //local_mask = ((struct sockaddr_in *)addrs->netmask)->sin_addr.s_addr;
    local_ip = inet_addr(pIpAdapterInfo->IpAddressList.IpAddress.String);
    local_mask = inet_addr(pIpAdapterInfo->IpAddressList.IpMask.String);
    local_gateway = inet_addr(pIpAdapterInfo->GatewayList.IpAddress.String);

    /* construct the filter string */
    sprintf(filter_exp, "ether dst %02x:%02x:%02x:%02x:%02x:%02x"
                        " and ether proto 0x888e",
                        local_mac[0], local_mac[1],
                        local_mac[2], local_mac[3],
                        local_mac[4], local_mac[5]);

	/* compile the filter expression */
	if (pcap_compile(handle, &fp, filter_exp, 0, 0) == -1) {
		fprintf(stderr, "Couldn't parse filter %s: %s\n",
		    filter_exp, pcap_geterr(handle));
		exit(EXIT_FAILURE);
	}

	/* apply the compiled filter */
	if (pcap_setfilter(handle, &fp) == -1) {
		fprintf(stderr, "Couldn't install filter %s: %s\n",
		    filter_exp, pcap_geterr(handle));
		exit(EXIT_FAILURE);
	}
    pcap_freecode(&fp);
    pcap_freealldevs(alldevs);
}

//void getMAC (char *DEVICE, uint8_t *lmac)
//{
    //int sockfd;
    //struct ifreq strureq;
    //sockfd = socket (PF_INET, SOCK_STREAM, 0);
    //memset (&strureq, 0, sizeof (strureq));
    //strncpy (&strureq.ifr_name, DEVICE, sizeof (strureq.ifr_name));
    //ioctl (sockfd, SIOCGIFHWADDR, &strureq);
    //strcpy (lmac, ether_ntoa (strureq.ifr_hwaddr.sa_data));
    //close (sockfd);
//}
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  set_device_new_ip
 *  Description:  用于DHCP模式下，当成功验证后并收到服务器发来的保鲜报文，
 *  调用本函数重新获取本机IP并写入应答报文中。
 * =====================================================================================
 */
//int set_device_new_ip()
//{
   //struct ifreq ifr;
   //int sock;

   //strcpy(ifr.ifr_name, dev);
   //if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
   //{
       //perror("socket");
       //exit(EXIT_FAILURE);
   //}
   //if(ioctl(sock, SIOCGIFADDR, &ifr) < 0)
   //{
       //return -1;
   //}
   //if(ioctl(sock, SIOCGIFNETMASK, &ifr) < 0)
   //{
       //return -1;
   //}
   //local_ip = ((struct  sockaddr_in*)&ifr.ifr_addr)->sin_addr.s_addr;
   //local_mask = ((struct sockaddr_in*)&ifr.ifr_netmask)->sin_addr.s_addr;

   //size_t data_index = 14 + 9 + username_length + 1;
   //memcpy (eap_response_ident + data_index, &local_ip, 4);
   //data_index += 4;
   //memcpy (eap_response_ident + data_index, &local_mask, 4);
   //return 0;
//}


/*
 * ===  FUNCTION  ======================================================================
 *         Name:  code_convert
 *  Description:  字符串编码转换
 * =====================================================================================
 */
//int
//code_convert(char *from_charset, char *to_charset,
             //char *inbuf, size_t inlen, char *outbuf, size_t outlen)
//{
    //iconv_t cd;

    //cd = iconv_open(to_charset,from_charset);

    //if (cd==0)
      //return -1;
    //memset(outbuf,0,outlen);

    //if (iconv (cd, &inbuf, &inlen, &outbuf, &outlen)==-1)
      //return -1;
    //iconv_close(cd);
    //return 0;
//}


/*
 * ===  FUNCTION  ======================================================================
 *         Name:  print_server_info
 *  Description:  提取中文信息并打印输出
 * =====================================================================================
 */

void
print_server_info (const uint8_t *packet, uint16_t packetlength)
{
    const uint8_t *str;

    {
        if ( *(packet + 0x2A) == 0x12) {
            str = (packet + 0x2B);
            goto FOUND_STR;
        }
        if (packetlength < 0x42)
            return;
        if ( *(packet + 0x42) == 0x12) {
            str = (packet + 0x43);
            goto FOUND_STR;
        }
        if (packetlength < 0x9A)
            return;
        if ( *(packet + 0x9A) == 0x12) {
            str = (packet + 0x9B);
            goto FOUND_STR;
        }
        if (packetlength < 0x120)
            return;
        if ( *(packet + 0x120) == 0x12) {
            str = (packet + 0x121);
            goto FOUND_STR;
        }
        return;
    }

    FOUND_STR:;

    //char info_str [1024] = {0};
    //code_convert ("gb2312", "utf-8", (char*)(str + 1), *str, info_str, 1024);
    fprintf (stdout, ">>Server Info: %s\n", str);
}


/*
 * ===  FUNCTION  ======================================================================
 *         Name:  show_local_info
 *  Description:  显示信息
 * =====================================================================================
 */
void show_local_info ()
{
    char buf[64];
    in_addr addr_ip, addr_mask, addr_gateway, addr_dns;
    //printf("######## Client ver. %s $Revision: 93 $ #########\n", ZDC_VER);
    printf("Device:     %s\n", dev_if_name);
    printf("MAC:        %02x:%02x:%02x:%02x:%02x:%02x\n",
                       local_mac[0],local_mac[1],local_mac[2],
                        local_mac[3],local_mac[4],local_mac[5]);
    memcpy(&addr_ip, &local_ip, 4);
    memcpy(&addr_mask, &local_mask, 4);
    memcpy(&addr_gateway, &local_gateway, 4);
    memcpy(&addr_dns, &local_dns, 4);
    printf("IP:         %s\n", inet_ntoa(addr_ip));
    printf("MASK:       %s\n", inet_ntoa(addr_mask));
    printf("Gateway:    ");
    if(local_gateway)   printf("%s\n", inet_ntoa(addr_gateway));
    else                printf("---\n");
    printf("DNS:        ");
    if(local_dns)       printf("%s\n", inet_ntoa(addr_dns));
    else                printf("---\n");
    printf("\nClient Version: %s\n", client_ver);
    printf("##################################################\n\n\n");
}


/*
 * ===  FUNCTION  ======================================================================
 *         Name:  init_arguments
 *  Description:  初始化和解释命令行的字符串。getopt_long
 * =====================================================================================
 */
void init_arguments(int *argc, char ***argv)
{
    /* Option struct for progrm run arguments */
    static struct option long_options[] =
        {
        {"help",        no_argument,        0,              'h'},
        {"background",  no_argument,        &background,    1},
        {"dhcp",        no_argument,        &dhcp_on,       1},
        {"device",      required_argument,  0,              2},
        {"ver",         required_argument,  0,              3},
        {"username",    required_argument,  0,              'u'},
        {"password",    required_argument,  0,              'p'},
        {"ip",          required_argument,  0,              4},
        {"mask",        required_argument,  0,              5},
        {"gateway",     required_argument,  0,              'g'},
        {"dns",         required_argument,  0,              'd'},
        {0, 0, 0, 0}
        };
    int c;
    while (1) {

        /* getopt_long stores the option index here. */
        int option_index = 0;
        c = getopt_long ((*argc), (*argv), "u:p:g:d:hbl",
                        long_options, &option_index);
        if (c == -1)
            break;
        switch (c) {
            case 0:
               break;
            case 'b':
                background = 1;
                break;
            case 2:
                dev = optarg;
                break;
            case 3:
                client_ver = optarg;
                break;
            case 4:
                user_ip = optarg;
                break;
            case 5:
                user_mask = optarg;
                break;
            case 'u':
                username = optarg;
                break;
            case 'p':
                password = optarg;
                break;
            case 'g':
                user_gateway = optarg;
                break;
            case 'd':
                user_dns = optarg;
                break;
            case 'l':
                exit_flag = 1;
                break;
            case 'h':
                show_usage();
                exit(EXIT_SUCCESS);
                break;
            case '?':
                if (optopt == 'u' || optopt == 'p'|| optopt == 'g'|| optopt == 'd')
                    fprintf (stderr, "Option -%c requires an argument.\n", optopt);
                exit(EXIT_FAILURE);
                break;
            default:
                fprintf (stderr,"Unknown option character `\\x%x'.\n", c);
                exit(EXIT_FAILURE);
        }
    }
}

int getAdapters(PIP_ADAPTER_INFO &pIpAdapterInfo)
{
    //PIP_ADAPTER_INFO结构体指针存储本机网卡信息
    //得到结构体大小,用于GetAdaptersInfo参数
    unsigned long stSize = sizeof(IP_ADAPTER_INFO);
    //调用GetAdaptersInfo函数,填充pIpAdapterInfo指针变量;其中stSize参数既是一个输入量也是一个输出量
    int nRel = GetAdaptersInfo(pIpAdapterInfo,&stSize);
    if (ERROR_BUFFER_OVERFLOW==nRel)
    {
         //如果函数返回的是ERROR_BUFFER_OVERFLOW
         //则说明GetAdaptersInfo参数传递的内存空间不够,同时其传出stSize,表示需要的空间大小
        //这也是说明为什么stSize既是一个输入量也是一个输出量
        //释放原来的内存空间
        delete pIpAdapterInfo;
        //重新申请内存空间用来存储所有网卡信息
        pIpAdapterInfo = (PIP_ADAPTER_INFO)new BYTE[stSize];
        //再次调用GetAdaptersInfo函数,填充pIpAdapterInfo指针变量
        nRel=GetAdaptersInfo(pIpAdapterInfo,&stSize);    
    }
    if (ERROR_SUCCESS==nRel)
    {
        //输出网卡信息
        showAdapter(pIpAdapterInfo);
    }
    else{
		fprintf(stderr,"Error, Not find Ethernet Adapter.\n");
        system("pause");
		exit(1);
    }
    return 0;
}
void showAdapter(PIP_ADAPTER_INFO pIpAdapterInfo)
{
    int i=0;
    PIP_ADAPTER_INFO p = pIpAdapterInfo;
    //可能有多网卡,因此通过循环去判断
    while (p)
    {
        cout<<"----> eth"<<i++<<endl;
        devs_len++;
        cout<<" Adapter Name: \n\t"<<p->AdapterName<<endl;
        cout<<" Adapter Description: \n\t"<<p->Description<<endl;
        cout<<" MAC Address: \n\t";//<<pIpAdapterInfo->Address;
        for (UINT i = 0; i < p->AddressLength; i++)
        {
            if (i==p->AddressLength-1)
            {
                printf("%02x\n", p->Address[i]);
            }
            else
            {
                printf("%02x:", p->Address[i]);
            }
        }
        cout<<" IP Address: \n\t";
        //可能网卡有多IP,因此通过循环去判断
        IP_ADDR_STRING *pIpAddrString =&(p->IpAddressList);
        do 
        {
            cout<<pIpAddrString->IpAddress.String<<endl;
            pIpAddrString=pIpAddrString->Next;
        } while (pIpAddrString);
        p= p->Next;
        cout<<"---------------------------------------------------"<<endl;
    }
    return;
}

static void
signal_interrupted (int signo)
{
    fprintf(stdout,"\n&&Info: USER Interrupted. \n");
    send_eap_packet(EAPOL_LOGOFF);
    pcap_breakloop (handle);
}

void reauthorize(int *t)
{
    (*t)++;
    fprintf(stdout, "&&Info: Try to Reauthorize (%d)\n", *t);
    send_eap_packet (EAPOL_LOGOFF);
    state = READY;
    Sleep(10*1000);
    send_eap_packet (EAPOL_START);
}
