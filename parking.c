///////////////////////////////////////////////////////////
//
//  Copyright(C), 2013-2017, GEC Tech. Co., Ltd.
//
//  文件: parking/parking.c
//  日期: 2017-9
//  描述: 停车场计费系统，功能：
//        A) 刷卡进场，并拍照留档，重复刷卡无效
//        B) 刷卡出场，自动计算费用，重复刷卡无效
//
//  作者: Vincent Lin (林世霖)   微信公众号：秘籍酷
//  技术微店: http://weidian.com/?userid=260920190
//  技术交流: 260492823（QQ群）
//
///////////////////////////////////////////////////////////

#include "common.h"

char get_dev_info[8];
char pcd_config[9];
int  g_buz;

char *province[] = {"粤", "赣", "湘", "鄂", "贵",
		    "黑", "沪", "京", "蒙", "藏",
		    "甘", "豫", "鲁", "晋", "桂",
		    "云", "滇", "辽", "川", "闵",
		    "徽", "苏", "津", "吉", "冀",
		    "陕", "宁", "浙", "渝", "琼",
		    "港", "澳", "台"};

char alphanumeric[] = { 'A', 'B', 'C', 'D', 'E',
			'F', 'G', 'H', 'I', 'J',
			'K', 'L', 'M', 'N', 'O',
			'P', 'Q', 'R', 'S', 'T',
			'U', 'V', 'W', 'X', 'Y',
			'Z', '0', '1', '2', '3',
			'4', '5', '6', '7', '8',
			'9', '0'};

// 设置窗口参数:9600速率
void init_tty(int fd)
{    
	//声明设置串口的结构体
	struct termios termios_new;
	//先清空该结构体
	bzero( &termios_new, sizeof(termios_new));

	cfmakeraw(&termios_new);

	//设置波特率
	cfsetispeed(&termios_new, B9600);
	cfsetospeed(&termios_new, B9600);

	// CLOCAL和CREAD分别用于本地连接和接受使能
	// 首先要通过位掩码的方式激活这两个选项。    
	termios_new.c_cflag |= CLOCAL | CREAD;

	//通过掩码设置数据位为8位
	termios_new.c_cflag &= ~CSIZE;
	termios_new.c_cflag |= CS8; 

	//设置无奇偶校验
	termios_new.c_cflag &= ~PARENB;

	//一位停止位
	termios_new.c_cflag &= ~CSTOPB;

	// 可设置接收字符和等待时间，无特殊要求可以将其设置为0
	termios_new.c_cc[VTIME] = 0;
	termios_new.c_cc[VMIN] = 1;

	// 用于清空输入/输出缓冲区
	tcflush (fd, TCIFLUSH);

	//完成配置后，可以使用以下函数激活串口设置
	if(tcsetattr(fd, TCSANOW, &termios_new))
		printf("Setting the serial1 failed!\n");
}


// 计算校验和
unsigned char CalBCC(unsigned char *buf, int n)
{
	int i;
	unsigned char bcc=0;
	for(i = 0; i < n; i++)
	{
		bcc ^= *(buf+i);
	}
	return (~bcc);
}

void beep(int buz, float microsec)
{
	ioctl(buz, 0, 1);
	usleep(microsec * 1000*1000);
	ioctl(buz, 1, 1);
}

char *random_alphanumeric(void)
{
	static char licence_num[7];

	srand(time(NULL));
	licence_num[0] = alphanumeric[rand() % 26];
	licence_num[1] = alphanumeric[rand() % 37];

	srand(licence_num[1]);
	licence_num[2] = alphanumeric[rand() % 37];

	srand(licence_num[2]);
	licence_num[3] = alphanumeric[rand() % 37];

	srand(licence_num[3]);
	licence_num[4] = alphanumeric[rand() % 37];

	srand(licence_num[4]);
	licence_num[5] = alphanumeric[rand() % 37];

	return licence_num;
}

char *take_photo(void)
{
	// 拍一张照片

	// 自动识别出其车牌号

	// 返回车牌号作为照片的文件名存入数据库
	char *licence = calloc(1, 10);
	snprintf(licence, 10, "%s%s", province[rand()%33], random_alphanumeric());

	return licence; // licence举例："粤B9MK48"
}

bool send_cmd_to_uart(int fd, char * cmd, char *ack)
{
	assert(cmd);

	char RBuf[128];
	while(1)
	{
		// 向串口发送指令
		tcflush (fd, TCIFLUSH);
		write(fd, cmd, strlen(cmd));

		usleep(10*1000);

		bzero(RBuf, 128);
		int nread = read(fd, RBuf, 128);
		if(nread == -1 && errno == EAGAIN)
		{
			continue;
		}
		else if(nread == -1)
		{
			perror("read() failed");
			return false;
		}

		//应答帧状态部分为0 则请求成功
		if(RBuf[2] == 0x00)	
		{
			if(ack != NULL)
				memcpy(ack, RBuf, nread);
			break;
		}

		usleep(100*1000);
	}
	return true;
}

char *total_time(char *time_in, char *now)
{
	assert(time_in);
	assert(now);

	#define YEAR 0
	#define MON  1
	#define MDAY 2
	#define HOUR 3
	#define MIN  4
	#define SEC  5
	int val_time_in[6], val_now[6];

	char *delim = "-: \t";

	char *p = strtok(time_in, delim);
	int i;
	for(i=0; i<6; i++)
	{
		if(p != NULL)
		{
			val_time_in[i] = atoi(p);
			p = strtok(NULL, delim);
		}
	}

	p = strtok(now, delim);
	for(i=0; i<6; i++)
	{
		if(p != NULL)
		{
			val_now[i] = atoi(p);
			p = strtok(NULL, delim);
		}
	}

	int year = val_now[YEAR] - val_time_in[YEAR];
	int mon  = val_now[MON]  - val_time_in[MON];
	int mday = val_now[MDAY] - val_time_in[MDAY];
	int hour = val_now[HOUR] - val_time_in[HOUR];
	int min  = val_now[MIN]  - val_time_in[MIN];
	int sec  = val_now[SEC]  - val_time_in[SEC];

	static char ret[64];
	static char dis[64];
	bzero(ret, 64);
	bzero(dis, 64);

	if(year != 0)
	{
		snprintf(ret+strlen(ret), 64-strlen(ret), "%dY", year);
		snprintf(dis+strlen(dis), 64-strlen(dis), "%d年", year);
	}
	if(mon  != 0)
	{
		snprintf(ret+strlen(ret), 64-strlen(ret), "%dM", mon);
		snprintf(dis+strlen(dis), 64-strlen(dis), "%d月", mon);
	}
	if(mday != 0)
	{
		snprintf(ret+strlen(ret), 64-strlen(ret), "%dD", mday);
		snprintf(dis+strlen(dis), 64-strlen(dis), "%d日", mday);
	}

	if(hour != 0)
	{
		snprintf(ret+strlen(ret), 64-strlen(ret), "%dH", hour);
		snprintf(dis+strlen(dis), 64-strlen(dis), "%d时", hour);
	}
	if(min  != 0)
	{
		snprintf(ret+strlen(ret), 64-strlen(ret), "%dm", min);
		snprintf(dis+strlen(dis), 64-strlen(dis), "%d分", min);
	}
	if(sec  != 0)
	{
		snprintf(ret+strlen(ret), 64-strlen(ret), "%dS", sec);
		snprintf(dis+strlen(dis), 64-strlen(dis), "%d秒", sec);
	}

	printf("停车时长：%s\n", dis);

	return ret;
}

int cal_fee(char *total)
{
	assert(total);

	// 停车费用政策：￥1/秒，上不封顶（贵吧！）
	int fee = 0;
	char *p, *q;

	fee = atoi(strtok(total, "S"));
	
	/*
	if(strstr(total, "Y"))
	{
		p = strtok(total, "Y");
		if(p != NULL)
			fee += (atoi(p)*365*24*60*60);
	}

	if(strstr(total, "M"))
	{
		p = strtok(NULL, "M");
		if(p != NULL)
			fee += (atoi(p)*30*24*60*60);
	}

	if(strstr(total, "D"))
	{
		p = strtok(NULL, "D");
		if(p != NULL)
			fee += (atoi(p)*24*60*60);
	}

	if(strstr(total, "H"))
	{
		p = strtok(NULL, "H");
		if(p != NULL)
			fee += (atoi(p)*60*60);
	}
	
	if(strstr(total, "m"))
	{
		p = strtok(NULL, "m");
		if(p != NULL)
			fee += (atoi(p)*60);
	}

	if(strstr(total, "S"))
	{
		p = strtok(NULL, "S");
		if(p != NULL)
			fee += (atoi(p));
	}
	*/

	return fee;
}

void *routine(void *arg)
{
	assert(arg);
	sqlite3 *db = (sqlite3 *)arg;

	int fd = open(DEV_PATH2, O_RDWR | O_NOCTTY);
	init_tty(fd);

	int old=0, cardid=0;
	char RBuf[128];
	while(1)
	{
		send_cmd_to_uart(fd, get_dev_info, NULL);

		// 向串口发送指令
		tcflush (fd, TCIFLUSH);
		write(fd, pcd_config, 8);

		usleep(10*1000);

		bzero(RBuf, 128);
		read(fd, RBuf, 128);

		//应答帧状态部分为0 则成功
		if(RBuf[2] == 0x00) 
		{
			int i, j;
			for(i=RBuf[3]-1, j=0; i>=0; i--, j++)
			{
				memcpy((char *)&cardid+j, &RBuf[4+i], 1);
			}
		}
		else
		{
			continue;
		}

		if(cardid != 0)
		{
			char *licence, *time_in;
			if(stop_charging(db, cardid, &licence, &time_in))
			{
				time_t t = time(NULL);
				char *now = calloc(1, 64);
				struct tm *p = localtime(&t);
				snprintf(now, 64, "%d-%d-%d %02d:%02d:%02d",
						p->tm_year+1900, p->tm_mon+1, p->tm_mday,
						p->tm_hour, p->tm_min, p->tm_sec);

				fprintf(stderr, "入场时间: %s\n", time_in);

				char *total = total_time(time_in, now);
				fprintf(stderr, "停车时长: %s\n", total);
				fprintf(stderr, "车费: ￥%d\n\n", cal_fee(total));

				beep(g_buz, 0.5); // 长哔一声表示成功
			}
			else
			{
				fprintf(stderr, "\n此卡【未进场】.\n");

				// 短哔五声表示失败
				beep(g_buz, 0.1); usleep(50*1000);
				beep(g_buz, 0.1); usleep(50*1000);
				beep(g_buz, 0.1); usleep(50*1000);
				beep(g_buz, 0.1); usleep(50*1000);
				beep(g_buz, 0.1);
			}
		}
		old = cardid==0 ? old : cardid;
	}

	close(fd);
}


bool is_swipped(sqlite3 *db, int cardid)
{
	assert(db);

	char *sql = "select count(*) from \"carinfo\" where carinfo.cardid = cardid;";

	char *errmsg;
	if(sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK)
	{
		fprintf(stderr, "SQL error: %s\n", errmsg);
		return false;
	}

	return true;
}

int main(void)
{
	// 初始化串口
	int fd = open(DEV_PATH1, O_RDWR | O_NOCTTY);
	init_tty(fd);

	// 打开蜂鸣器
	g_buz = open("/dev/beep", O_RDWR);

	// 打开/创建数据库parking.db，并创建表carinfo
	sqlite3 *db;
	sqlite3_open_v2("parking.db", &db,
			SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
			NULL);

	char *errmsg;
	char *create =  "create table if not exists "
			"carinfo(cardid  text primary key, "
			"licence text, "
			"time_in text not null, "
			"photo   text);";
	if(sqlite3_exec(db, create, NULL, NULL, &errmsg) != SQLITE_OK)
	{
		fprintf(stderr, "SQL error: %s\n", errmsg);
		exit(0);
	}

	sqlite3_exec(db, ".mode col", NULL, NULL, NULL);
	sqlite3_exec(db, ".header on", NULL, NULL, NULL);

	// 准备应用命令
	get_dev_info[0] = 0x07;	//帧长= 7 Byte
	get_dev_info[1] = 0x02;	//包号= 0 , 命令类型= 0x01
	get_dev_info[2] = 'A';	//命令= 'A'，读取设备信息
	get_dev_info[3] = 0x01;	//信息长度 = 1
	get_dev_info[4] = 0x52;	//请求模式:  ALL=0x52
	get_dev_info[5] = CalBCC(get_dev_info, get_dev_info[0]-2); //校验和
	get_dev_info[6] = 0x03; //结束标志

	pcd_config[0] = 0x08;	//帧长= 8 Byte
	pcd_config[1] = 0x02;	//包号= 0 , 命令类型= 0x01
	pcd_config[2] = 'B';	//命令= 'B'
	pcd_config[3] = 0x02;	//信息长度= 2
	pcd_config[4] = 0x93;	//防碰撞0x93: 一级防碰撞
	pcd_config[5] = 0x00;	//位计数0
	pcd_config[6] = CalBCC(pcd_config, pcd_config[0]-2); //校验和
	pcd_config[7] = 0x03;	//结束标志



	// 创建一条线程，处理车辆出场刷卡业务逻辑
	pthread_t tid;
	pthread_create(&tid, NULL, routine, (void *)db);



	int old=0, cardid=0;
	fprintf(stderr, "请将RFID卡靠近读卡器\n");

	char RBuf[128];
	while(1)
	{
		// 向串口发送指令
		send_cmd_to_uart(fd, get_dev_info, NULL); // 不需要返回结果

		// 向串口发送指令
		tcflush (fd, TCIFLUSH);
		write(fd, pcd_config, 8);

		usleep(10*1000);

		bzero(RBuf, 128);
		read(fd, RBuf, 128);

		//应答帧状态部分为0 则成功
		if(RBuf[2] == 0x00) 
		{
			int i, j;
			for(i=RBuf[3]-1, j=0; i>=0; i--, j++)
			{
				memcpy((char *)&cardid+j, &RBuf[4+i], 1);
			}
		}
		else
		{
			continue;
		}

		if(cardid != 0)
		{
			char *licence = take_photo();
			char *photo_path = calloc(1, 64);
			strcpy(photo_path, "/path/to/photos/");
			strcat(photo_path, licence);
			strcat(photo_path, ".jpg");

			if(start_charging(db, licence, cardid, photo_path))
			{
				// 长哔一声表示成功
				beep(g_buz, 0.5);
			}
			else
			{
				fprintf(stderr, "\n此卡【已进场】.\n");

				// 短哔五声表示失败
				beep(g_buz, 0.1); usleep(50*1000);
				beep(g_buz, 0.1); usleep(50*1000);
				beep(g_buz, 0.1); usleep(50*1000);
				beep(g_buz, 0.1); usleep(50*1000);
				beep(g_buz, 0.1);
			}
		}
		old = cardid==0 ? old : cardid;
	}

	close(fd);

	exit(0);
}
