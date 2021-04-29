#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <linux/ethtool.h>
#include <linux/mdio.h>
#include <linux/sockios.h>

void show_usage(void)
{
	printf("mii_mgr -g -i [ifname] -p [phy number] -r [register number]\n");
	printf("  Get: mii_mgr -g -p 3 -r 4\n\n");
	printf("mii_mgr -s -p [phy number] -r [register number] -v [0xvalue]\n");
	printf("  Set: mii_mgr -s -p 4 -r 1 -v 0xff11\n");
	printf("#NOTE: Without -i , eth0 is default ifname!\n");
	printf("----------------------------------------------------------------------------------------\n");
	printf("Get: mii_mgr_cl45 -g -p [port number] -d [dev number] -r [register number]\n");
	printf("Example: mii_mgr_cl45 -g -p 3 -d 0x5 -r 0x4\n\n");
	printf("Set: mii_mgr_cl45 -s -p [port number] -d [dev number] -r [register number] -v [value]\n");
	printf("Example: mii_mgr_cl45 -s -p 4 -d 0x6 -r 0x1 -v 0xff11\n\n");
}

static int __phy_op(char *ifname,uint16_t phy_id,uint16_t reg_num, uint16_t *val, int cmd)
{
        static int sd = -1;

        struct ifreq ifr;
        struct mii_ioctl_data* mii = (struct mii_ioctl_data *)(&ifr.ifr_data);
        int err;

        if (sd < 0)
                sd = socket(AF_INET, SOCK_DGRAM, 0);

        if (sd < 0)
                return sd;

        strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));

        mii->phy_id  = phy_id;
        mii->reg_num = reg_num;
        mii->val_in  = *val;
        mii->val_out = 0;

        err = ioctl(sd, cmd, &ifr);
        if (err)
                return -errno;

        *val = mii->val_out;
        return 0;
}

int main(int argc, char *argv[])
{
	int opt;
	char options[] = "gsi:p:d:r:v:?t";
	int is_write = 0,is_cl45 = 0;
	unsigned int port=0, dev=0,reg_num=0,val=0;
	char ifname[IFNAMSIZ]="eth0";	
	uint16_t phy_id=0;


	if (argc < 6) {
		show_usage();
		return 0;
	}

	while ((opt = getopt(argc, argv, options)) != -1) {
		switch (opt) {
			case 'g':
				is_write=0;
				break;
			case 's':
				is_write=1;
				break;
			case 'i':
				strncpy(ifname,optarg, 5);
				break;	
			case 'p':
				port = strtoul(optarg, NULL, 16);
				break;
                        case 'd':				
                                dev = strtoul(optarg, NULL, 16);
				is_cl45 = 1;
				break;
			case 'r':
				reg_num = strtoul(optarg, NULL, 16);
				break;

			case 'v':
				val = strtoul(optarg, NULL, 16);
				break;
			case '?':
				show_usage();
				break;
		}
	}

	if(is_cl45)
		phy_id = mdio_phy_id_c45(port, dev);
	else
		phy_id = port;

	if(is_write) { 
		__phy_op(ifname,phy_id,reg_num,(uint16_t *)&val,SIOCSMIIREG);

		if(is_cl45)
			printf("Set: port%x dev%Xh_reg%Xh = 0x%04X\n",port, dev, reg_num, val);
		else
			printf("Set: phy[%x].reg[%x] = %04x\n",port, reg_num, val);
	}	
	else {
		__phy_op(ifname,phy_id,reg_num,(uint16_t *)&val,SIOCGMIIREG);

		if(is_cl45)
			printf("Get: port%x dev%Xh_reg%Xh = 0x%04X\n",port, dev, reg_num, val);
		else
			printf("Get: phy[%x].reg[%x] = %04x\n",port, reg_num, val);
	
	}

	return 0;	
}
