// ========================================= 
// PORCHERIA INIZIALE                       
//   Serve a farmi un'idea di come impostare
//   la logica di gioco e imparare Redis
//   Andra' riscritto interamente e aggiunta
//   una gestione degli errori (gioco/codice)
// =========================================

# include <stdio.h>
# include <string.h>
# include <time.h>
# include <hiredis/hiredis.h>
# include <unistd.h>
#include <termios.h>

# define MAX_TURN 10
# define MAX_GAME 100000

# define O_MAN    0   // 1 = manuale   0 = automatico 
# define X_MAN    0
# define O_LERN   1   // 1 = learning  0 = no learning
# define X_LERN   1
# define AUTO     1
# define START    '-'



// =========================================
//  STRUTTURE DATI IN REDIS
// -----------------------------------------
// HASH
//   game:current
//     game    = numero della partita    
//     turn    = mossa
//     board   = sequenza X-X-O-OXO 
//     next    = O | X
//     winner  = O | X | = | -
//

void Draw(redisContext *c){
	printf("\033[2J\033[H");

	redisReply *reply = redisCommand(c, "HGET game:current game");
	printf("GAME: %s\n", reply->str);
	reply = redisCommand(c, "HGET game:current turn");
	printf("TURNO: %s\n", reply->str);
	reply = redisCommand(c, "HGET game:current next");
	printf("GIOCA: %s\n\n\n", reply->str);

	reply = redisCommand(c, "HGET game:current board");
      	char *s = reply->str;

	for(int n = 0; n < 9; n++) 
		if(s[n] == '-') s[n] = ' ';
	printf(" %c | %c | %c \n", s[6], s[7], s[8]);
	printf("---|---|--- \n");
	printf(" %c | %c | %c \n", s[3], s[4], s[5]);
	printf("---|---|--- \n");
	printf(" %c | %c | %c \n\n\n", s[0], s[1], s[2]);

	if(reply) freeReplyObject(reply);
}


// ========================================
// restituisce 1 se ha trovato un vincitore
// oppure se sono piene tutte le caselle
// pareggio altrimenti 0
//
// deposita in game:current winner O, X 
// oppure = in caso di pareggio
// ========================================
int IsFinished(redisContext *c){
	redisReply *pReply = redisCommand(c, "HGET game:current board");
	if(pReply == NULL) {
        	printf("\nErrore nella query Redis\n");
        	return 0;
    	}
	char cWin = '=';
	if(pReply->type == REDIS_REPLY_STRING){
		char s[10];
		memcpy(s, pReply->str, 9);
		s[9] = '\0';

		if(s[0] == s[1] && s[1] == s[2] && s[0] != '-') cWin = s[0];
		if(s[3] == s[4] && s[4] == s[5] && s[3] != '-') cWin = s[3];
		if(s[6] == s[7] && s[7] == s[8] && s[6] != '-') cWin = s[6];
		if(s[0] == s[3] && s[3] == s[6] && s[0] != '-') cWin = s[0];
		if(s[1] == s[4] && s[4] == s[7] && s[1] != '-') cWin = s[1];
		if(s[2] == s[5] && s[5] == s[8] && s[2] != '-') cWin = s[2];
		if(s[0] == s[4] && s[4] == s[8] && s[0] != '-') cWin = s[0];
		if(s[2] == s[4] && s[4] == s[6] && s[2] != '-') cWin = s[2];
	}	
	pReply = redisCommand(c, "HGET game:current turn");
	int nTurn = atoi(pReply->str);

	int nReturn = 0;
	if(cWin != '=' || (cWin == '=' && nTurn >= 10)) {
		char sWin[2];
		sWin[0] = cWin;
		sWin[1] = '\0';
		redisCommand(c, "HSET game:current winner %s", sWin);
		nReturn = 1;
	}
	if(pReply) freeReplyObject(pReply);
	return nReturn;
}

// =========================================
// Redis tools
// =========================================
// char* GetString(redisContext *c, char *sCmd){
//	redisReply *reply = redisCommand(c, "HGET game:current winner");
//	if(winReply->type == REDIS_REPLY_STRING) printf("Il vincitore e': %s", winReply->str);
//}
char GetChar(void) {
    struct termios oldt, newt;
    char ch;
    tcgetattr(STDIN_FILENO, &oldt);          // Salva le impostazioni attuali
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);        // Disabilita buffer di riga ed eco
    tcsetattr(STDIN_FILENO, TCSANOW, &newt); // Applica le nuove impostazioni
    ch = getchar();                          // Legge il carattere
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt); // Ripristina le impostazioni originali
    return ch;
}



// ==========================================
//                  MAIN
// ==========================================
int main(void){
	srand(time(NULL));
	printf("\033[2J\033[H");
	printf("INIT ...!\n\n\n");
	printf("Usa i tasti del tastierino numerico della tastiera\n\n\n");
	printf("per scegliere la casella di gioco\n\n\n");

	printf(" 7 | 8 | 9 \n");
	printf("---|---|---\n");
	printf(" 4 | 5 | 6 \n");
	printf("---|---|---\n");
	printf(" 1 | 2 | 3 \n\n\n\n");

	redisContext *c = redisConnect("127.0.0.1", 6379);
	if(c == NULL || c->err) {
		if(c) {
			printf("Errore di connessione: %s\n", c->errstr);
			redisFree(c);
		} else {
			printf("Impossibile allocare contesto Redis\n");
		}
		return 1;
	}

	int nGame = 1;
	int nCount = 0;
	while(nGame <= MAX_GAME){
		// int nNew = 1;

		int n = (rand() % 2); // random 0 | 1
		char sNext[] = "O";
		if(n) sNext[0] = 'X';

		if( START == 'O' ) sNext[0] = 'O';
		if( START == 'X' ) sNext[0] = 'X';

		printf("Nuova partita\n");
		printf("-------------\n");
		printf("GAME: %d\n", nGame);
		printf("INIZIA: %s\n\n", sNext);

		redisReply *reply = redisCommand(c, "HSET game:current board %s game %d turn %d winner %s next %s ", "---------", nGame, 1, "-", sNext);
		//sleep(1);

		while(!IsFinished(c)){
			Draw(c);

			usleep(1000);
			redisReply *replyGame = redisCommand(c, "HGET game:current next");
			char *sNext = replyGame->str;
			replyGame = redisCommand(c, "HGET game:current board");
			char *sBoard = replyGame->str;

			printf("Mossa giocatore: %s\n\n", sNext);

			int zManuale = 0;

			if(sNext[0] == 'X') zManuale = X_MAN;
			else zManuale = O_MAN;
			int nPosizione = 10; // volutamente non valido

			if(zManuale){
				// ================
				// RISPOSTA MANUALE
				// ================
				char cPosizione = GetChar();
				nPosizione = cPosizione - '1';
				if(nPosizione > 8 || nPosizione < 0){
					printf("\nTasto non valido\n");
					//sleep(1);
					continue;
				}
			}
			else {
				// ===================
				// RISPOSTA AUTOMATICA
				// ===================

				// nPosizione = rand() % 9;   // casuale
							   
				int nPesi[9];
				int nPesoMin = 0;
				int nPesoMax = 0;
				int nRangeRand[9];
				int nTopRange = 0;


				redisReply *reply = NULL;

				for(int n = 0; n < 9; n++){
					if(sBoard[n] == '-') {
			       			reply = redisCommand(c, "HGET player:%s:%s %d", sNext, sBoard, n);
						if(reply->type == REDIS_REPLY_STRING)	nPesi[n] = atoi(reply->str);
						else nPesi[n] = 0;
						if(nPesoMin > nPesi[n]) nPesoMin = nPesi[n];
						if(nPesoMax < nPesi[n]) nPesoMax = nPesi[n];
					}
					else nPesi[n] = 0;

				}

//				printf("MIN: %d  -- MAX: %d \n ", nPesoMin, nPesoMax);
//				for(int n = 0; n < 9; n++){
//					printf("[%d] peso %d => ", n, nPesi[n]);
//					if(sBoard[n] == '-') nPesi[n] = nPesi[n] - nPesoMin;
//					printf("%d  \n",nPesi[n]);
//				}

				printf("\nPosizione %s \n", sBoard);
				for(int n = 0; n < 9; n++){
					printf("[%d] peso %d ==> ", n, nPesi[n]);
					if(sBoard[n] == '-'){
						if(nPesi[n] < 0) nRangeRand[n] = nTopRange;
						else{
							nRangeRand[n] = 10 + nPesi[n] + nTopRange;
							nTopRange = nRangeRand[n];
						}
					}
					else	nRangeRand[n] = nTopRange;
					printf("%d\n", nRangeRand[n]);
				}
				
				if(nTopRange){
					int nRand = rand() % nTopRange;
					printf("nRand = %d  \n", nRand);
					nPosizione = 0;
					while(nPosizione < 9){
						if(sBoard[nPosizione] == '-' && nRangeRand[nPosizione] >= nRand) break;
						nPosizione++;
					}
				}
				else{
					// ===========================================
					// PARTITA PERSA  - TUTTE POSIZIONI PERDENTI -
					// DETERMINAZIONE POSIZIONE FORZATA ALL'ULTIMA 
					// CASELLA LIBERA (-1)
					// ===========================================
					for (int n = 0; n < 9 ; n++){
						if(sBoard[n] == '-') nPosizione = n;
					}
					if(nPosizione > 8) printf("ATTENZIONE posizione non valida >8\n");

					// ===========================================
					// SETTA LA POSIZIONE PRECEDENTE COME PERDENTE
					// ===========================================
//					printf("TUTTE LE MOSSE SONO PERDENTI\n");
//					GetChar();

					reply = redisCommand(c, "HGET game:current turn");
					int nTurn = atoi(reply->str);
//					printf("MOSSA ATTUALE %d\n", nTurn);
//					GetChar();

					reply = redisCommand(c, "LINDEX game:current:steps %d", 1);
					char s[14];
				        memcpy(s, reply->str, 13);
//					printf("POSIZIONE PRECEDENTE %s\n", s);
//					GetChar();
					s[11] = 0;
					redisCommand(c, "HSET player:%s %s -1", s, s+12);

				}

				printf("La nuova posizione e': %d\n", nPosizione);
						   
				if(reply) freeReplyObject(reply);
				if(AUTO){
					usleep(500);
				}
				else{
					printf("PREMI UN TASTO PER CONTINUARE oppure cambia posizione(tasti 1..9)\n");
					char c = GetChar();
					int n = c - '1';
					if(n < 9 && n >= 0) nPosizione = n;
				}
			}

			if(sBoard[nPosizione] != '-'){
				if(zManuale){
					printf("\nMossa non valida, casella occupata.\n");
					//sleep(1);
				}
				continue;
			}

			
			redisCommand(c, "LPUSH game:current:steps %s:%s:%d", sNext, sBoard, nPosizione);

			sBoard[nPosizione] = sNext[0];
			sNext[0] = sNext[0] == 'O' ? 'X' : 'O';

			redisCommand(c, "HSET game:current next %s", sNext);
			redisCommand(c, "HSET game:current board %s", sBoard);
			redisCommand(c, "HINCRBY game:current turn 1");

			if(replyGame) freeReplyObject(replyGame);
			printf("\n\n\n");
			usleep(1000);
		}

		// =====================
		//      FINE GAME
		//
		// 1) resoconto
		// 2) aggiornamento pesi     
		// =====================

		printf("\033[2J\033[H");
		reply = redisCommand(c, "HGET game:current winner");
		char sWinner  = reply->str[0];
		if(sWinner == '=') {printf("PAREGGIO\n"); nCount++;}
		else printf("Ha vinto %c\n", sWinner);

		reply = redisCommand(c, "LRANGE game:current:steps 0 -1");
		printf("Ci sono >> %ld << steps in game.current.steps\n", reply->elements);

		for(int n = 0; n < reply->elements; n++){
			char *sStep = reply->element[n]->str;
			printf("MOSSA[%d] = %s = ", n, sStep);


			//  +---------------- giocatore
			//  |    +----------- board
			//  |    |      +---- posizione
			//  |    v      |
			//  v --------- v
			// [X:XXO---OOX:4]
			//             ^
			//  0000000000111
			//  0123456789012
			//  
			//  sStep    gioocatore + board
			//  sStep+12 posizione

			sStep[11] = 0;

			int nLast = reply->elements - 1; // posizione perdente - quella precedente
							 // alla vittoria dell'avversario

			redisReply *replyP = redisCommand(c, "HGET player:%s %s", sStep, sStep+12);
			if(replyP->type == REDIS_REPLY_STRING)	printf("peso: %s => ", replyP->str);
			else printf("peso: 0 => ");

			int nLern = 0;
			if(sStep[0] == 'O') nLern = O_LERN;
			else nLern = X_LERN;

//			if(sWinner == sStep[0] && nLern)
//				redisCommand(c, "HINCRBY player:%s %s 10", sStep, sStep+12);

			if(sWinner != sStep[0] && nLern && sWinner != '=')
			{
				if(n == 1)
					redisCommand(c, "HSET player:%s %s -1", sStep, sStep+12);
//				else
//					redisCommand(c, "HINCRBY player:%s %s -1", sStep, sStep+12);
			}
			if(sWinner == '=' && nLern)
				redisCommand(c, "HINCRBY player:%s %s 5", sStep, sStep+12);

			replyP = redisCommand(c, "HGET player:%s %s", sStep, sStep+12);
			printf(" %s \n", replyP->str);
			if(replyP) freeReplyObject(replyP);
		}
		if(AUTO)
			usleep(10000);
		else{
			printf("PREMI UN TASTO PER CONTINUARE\n");
			GetChar();
		}
		redisCommand(c, "COPY game:current game:%d", nGame);
		redisCommand(c, "COPY game:current:steps game:%d:steps", nGame);
		redisCommand(c, "DEL game:current");
		redisCommand(c, "DEL game:current:steps");
		nGame++;
		if(nGame % 100 == 0) {redisCommand(c, "LPUSH pareggi %d", nCount); nCount = 0;}
		if(reply) freeReplyObject(reply);
		usleep(5000);
	}
	redisFree(c);
	return 0;
}
