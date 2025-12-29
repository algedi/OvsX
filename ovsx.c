# include <stdio.h>
# include <hiredis/hiredis.h>

void Draw(redisContext *c){
	redisReply *reply = redisCommand(c, "HGET game:current board");
	if(reply->type == REDIS_REPLY_STRING){
		char *s = reply->str;
		printf(" %c | %c | %c \n", s[0], s[1], s[2]);
		printf("---|---|--- \n");
		printf(" %c | %c | %c \n", s[3], s[4], s[5]);
		printf("---|---|--- \n");
		printf(" %c | %c | %c \n", s[6], s[7], s[8]);
	}
	freeReplyObject(reply);
}

int IsFinished(redisContext *c){
	redisReply *reply = redisCommand(c, "HGET game:current board");
	char cWin = '=';
	if(reply->type == REDIS_REPLY_STRING){
		char *s = reply->str;
		if(s[0] == s[1] && s[1] == s[2]) cWin = s[0];
		if(s[3] == s[4] && s[4] == s[5]) cWin = s[3];
		if(s[6] == s[7] && s[7] == s[8]) cWin = s[6];
		if(s[0] == s[3] && s[3] == s[6]) cWin = s[0];
		if(s[1] == s[4] && s[4] == s[7]) cWin = s[1];
		if(s[2] == s[5] && s[5] == s[8]) cWin = s[2];
		if(s[0] == s[4] && s[4] == s[8]) cWin = s[0];
		if(s[2] == s[4] && s[4] == s[6]) cWin = s[2];
	}	
	reply = redisCommand(c, "HGET game:current turn");
	int nReturn = 0;
	if(cWin == '=' && reply->integer == 9) {
		nReturn = 1;
		redisCommand(c, "HGET game:current winner %s", cWin);
	}
	freeReplyObject(reply);
	return nReturn;
}

int main(void){
	printf("INIT ...!\n\n\n");
	redisContext *c = redisConnect("127.0.0.1", 6379);
	redisCommand(c, "HSET game:current board %s", "OXO XXO  ");

	Draw(c);

	redisFree(c);
	return 0;
}
