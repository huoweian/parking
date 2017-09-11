#include "common.h"

static int count;
static bool executed = false;

int __callback(void *arg, int cols, char **col_value, char **col_name)
{
	executed = true;
	char ***pointers = (char ***)arg;

	count = cols;
	printf("count: %d\n", count);

	if(count == 0)
	{
		return 0;
	}

	int i;
	for(i=0; i<cols; i++)
	{
		printf("%s\t", col_name[i]);
	}
	printf("\n");


	for(i=0; i<cols; i++)
	{
		printf("%s\t", col_value[i]);
	}

	*pointers[0] = col_value[1]; // licence
	*pointers[1] = col_value[2]; // time_in

	printf("\n");
}


bool start_charging(sqlite3 *db, char *licence, int cardid, char *photo_path)
{
	assert(db);

	time_t t = time(NULL);
	char *now = calloc(1, 64);
	struct tm *p = localtime(&t);
	snprintf(now, 64, "%d/%d/%d %02d:%02d:%02d", p->tm_year+1900, p->tm_mon+1, p->tm_mday,
										   p->tm_hour, p->tm_min, p->tm_sec);

	char *sql = calloc(1, 128);
	snprintf(sql, 128, "insert into \"carinfo\" values('%#.8x', '%s', '%s', '%s');",
			cardid, licence, now, photo_path);

	char *errmsg;
	if(sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK)
	{
		return false;
	}

	printf("\n******* 欢迎光临! ******* \n");
	printf("进场时间：%s\n", now);
	printf("卡号：%#.8x\n", cardid);
	printf("车牌：%s\n", licence);
	printf("************************** \n");
	return true;
}


bool stop_charging(sqlite3 *db, int cardid, char **licence, char **time_in)
{
	assert(db);
	assert(licence);
	assert(time_in);

	char *sql = calloc(1, 128);

	// 查询此卡是否已经进场，是的话获取其信息
	bzero(sql, 128);
	snprintf(sql, 128, "select * from \"carinfo\" where cardid = '%#.8x';", cardid);
	char *errmsg;
	char **pointers[] = {licence, time_in};

	executed = false;
	if(sqlite3_exec(db, sql, __callback, pointers, &errmsg) != SQLITE_OK)
	{
		fprintf(stderr, "SQL error: %s\n", errmsg);
		return false;
	}

	if(executed == true && count != 0)
	{
		// 获取完信息之后删除这张卡对应的记录
		bzero(sql, 128);
		snprintf(sql, 128, "delete from \"carinfo\" where cardid = '%#.8x';", cardid);
		if(sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK)
		{
			fprintf(stderr, "SQL error: %s\n", errmsg);
			return false;
		}
	}
	else
		return false;

	return true;
}