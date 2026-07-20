#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "dependencies/include/libpq-fe.h"
#define PG_HOST "127.0.0.1"
#define PG_DB "NoleggioAuto"
#define PG_PORT 5432

// windows: gcc noleggioauto.c -L dependencies\lib -lpq -o noleggioauto

//Funzione di controllo della connessione al database
void checkConnection(PGconn* conn);

//Funzione di controllo integrita' del risultato della query
void checkResults(PGresult *res, PGconn *conn);

//Stampa dei risultati di una query
void printResults(PGresult *res);

//Stampa lista delle query disponibili
void printQuery();

//funzione per autenticazione e accesso al DB di postgreSQL
PGconn* authentication();

int main(int argc, char **argv){
    PGconn* conn;
    conn = authentication();
    PGresult *res;
    unsigned int scelta = 7;
    printQuery();
    //variabile di controllo stato di scanf, aiuta a gestire casi in cui l'utente inserisce valori diversi da INT
    bool status=0;
    int c;
    while(scelta!=0){
    do{
        status =0;
        status = scanf("%d", &scelta);
        if(scelta ==0){
            printf("exit");
            PQfinish(conn);
            return 0;
        }
        if(scelta == 6) printQuery();
        if(scelta > 6 || status!=1)
            printf("Valore non valido\n");
        //in caso di inserimento di caratteri da parte dell'utente, faccio clean dello stream per evitare loop infinito
        while ((c = getchar()) != '\n' && c!=EOF);
        c=0;
    } while(scelta > 6 || status != 1);

    switch (scelta){
        case 1:
        {
            res = PQexec(conn, "SELECT C.idContratto, C.canone as SaldoRimanente FROM contratto C LEFT JOIN Pagamento P ON C.idcontratto = P.contratto GROUP BY C.idcontratto, C.canone HAVING sum(P.importo) IS NULL UNION SELECT C.idContratto, ( C.canone - sum(P.importo)) as SaldoRimanente FROM contratto C JOIN Pagamento P ON C.idcontratto = P.contratto GROUP BY C.idcontratto, C.canone having sum(P.importo) < C.canone order by idcontratto;");
            checkResults(res, conn);
            printResults(res);
            break;
        }
        case 2:
        {
            res = PQexec(conn, "drop view if exists sommacanonecliente; CREATE VIEW sommacanonecliente as SELECT cl.idcliente, SUM(co.canone) as sommacanone FROM contratto co join cliente cl on co.cliente = cl.idcliente group by cl.idcliente order by cl.idcliente; SELECT DISTINCT scc.idcliente, scc.sommacanone FROM sommacanonecliente scc join contratto con on scc.idcliente = con.cliente join veicolo v on v.targa = con.veicolo join tariffario t on v.tariffariomarca = t.marca and v.tariffariomodello = t.modello where t.tariffabase > 200 order by scc.idcliente");
            checkResults(res, conn);
            printResults(res);
            break;
        }
        case 3:
        {
            res = PQexec(conn, "drop view if exists contomanutenzioni; create view contomanutenzioni as select v.targa, count(*) as nmanutenzioni from veicolo v join manutenzione m on v.targa = m.veicolo where m.Tipologia = 'STRAORDINARIA' group by targa; select cm.targa, t.marca, t.modello, cm.nmanutenzioni from contomanutenzioni cm join veicolo v on cm.targa = v.targa join tariffario t on v.tariffariomarca = t.marca and v.tariffariomodello = t.modello");
            checkResults(res, conn);
            printResults(res);
            break;
        }
        case 4:
        {
            res = PQprepare(conn, "FilialeX", "select F.idfiliale, V.Totali, V1.Disponibili, V2.Noleggiati, V3.Manutenzione from Filiale F LEFT JOIN ( select filiale, count(*) as Totali from Veicolo GROUP BY filiale ) as V on F.idfiliale=V.filiale LEFT JOIN (select filiale, count(*) as Disponibili from Veicolo  where Stato = 'DISPONIBILE' GROUP BY filiale ) as V1 on F.idfiliale=V1.filiale LEFT JOIN ( select filiale, count(*) as Noleggiati from Veicolo where Stato = 'NOLEGGIATO' GROUP BY filiale) as V2 on F.idfiliale=V2.filiale LEFT JOIN ( select filiale, count(*) as Manutenzione from Veicolo where Stato = 'MANUTENZIONE' GROUP BY filiale) as V3 on F.idfiliale=V3.filiale where F.idfiliale = $1::int", 1, NULL);
            unsigned int x = 6;
            status =0;
            printf("Scegliere la filiale di riferimento (1-5)\n");
            printf("Inserire 0 per tornare alla selezione query\n");
            bool ritornoselezione = 0;
            do{
                status = scanf("%d", &x);
                if(x>5 || status!=1){
                    printf("Inserire un idfiliale valido\n");
                    //in caso di inserimento di caratteri da parte dell'utente, faccio clean dello stream per evitare loop infinito
                    while ((c=getchar())!= '\n' && c!=EOF);
                    c=0;
                }
                if(x==0){
                    ritornoselezione=1;
                    break;
                }
            }while (x> 5 || status !=1);
            if(ritornoselezione==1){
                PQclear(res);
                printf("Ritorno alla selezione query...\n");
                ritornoselezione=0;
                break;
            }
            //Avendo il valore INT dobbiamo trasformarlo in un valore char* per poterlo dare in input a PQexecPrepared
            char valorescelto[12];
            int lunghezza = snprintf(valorescelto, sizeof(valorescelto), "%d", x);
            //controllo della stringa appena formattata, se la lunghezza è negativa c'è un problema
            if(lunghezza<0) exit(1);
            const char* paramvalues[1];
            paramvalues[0] = valorescelto;
            PQclear(res);
            res = PQexecPrepared(conn, "FilialeX",1, paramvalues, NULL, 0,0);
            checkResults(res, conn);
            printResults(res);
            break;
        }
        case 5:
        {
            res = PQexec(conn, "drop view if exists FrequenzaAssicurazione; CREATE VIEW FrequenzaAssicurazione as select a.Tipologia, COUNT(*) as sottoscrizioni from contratto con join copertura cop on con.idcontratto = cop.contratto join assicurazione a on cop.assicurazione = a.tipologia where con.datainizio >= '2025-01-01' and con.datainizio <= '2025-12-31' and a.tipologia <> 'RCA' group by a.Tipologia; select fa.tipologia, fa.sottoscrizioni from frequenzaAssicurazione fa where fa.sottoscrizioni = (select max(sottoscrizioni) from frequenzaAssicurazione)");
            checkResults(res, conn);
            printResults(res);
            break;
        }
    }
}
}

void checkConnection(PGconn* conn){
    if (PQstatus(conn) != CONNECTION_OK){
        printf("Errore di connessione: %s\n", PQerrorMessage(conn));
        PQfinish(conn);
        exit(1);
    }
    else printf("Connessione avvenuta con successo\n");
}

void checkResults(PGresult *res, PGconn *conn){
    if(PQresultStatus(res) != PGRES_TUPLES_OK){
        printf("Risultati inconsistenti %s\n", PQerrorMessage(conn));
        PQclear(res);
        PQfinish(conn);
        exit(1);
    }
}

void printResults(PGresult *res){
    int tuple = PQntuples(res);
    int campi = PQnfields(res);
    for(int i = 0; i <campi; i++){
        printf("%-20s", PQfname(res,i));
    }
    printf("\n");
    for(int i=0; i<tuple; i++){
        for(int j=0; j<campi; j++){
            printf("%-20s", PQgetvalue(res, i, j));
        }
        printf("\n");
    }
    PQclear(res);
}

void printQuery(){
    printf("Selezionare la query desiderata:\n");
    printf("1) Mostrare i contratti che risultano avere canone non saldato e il loro saldo rimanente\n");
    printf("2) Mostrare i clienti che hanno noleggiato almeno un auto di lusso e la somma dei canoni dei contratti a loro associati\n");
    printf("3) Mostrare targa, marca, modello e numero di manutenzioni straordinarie dei veicoli che sono stati soggetti ad almeno una manutenzione straordinaria\n");
    printf("4) Mostrare per una filiale x numero di veicoli totali, disponibili, noleggiati e in manutenzione\n");
    printf("5) Mostrare le assicurazioni piu' sottoscritte nell'anno 2025 (RCA esclusa)\n");
    printf("Inserire 0 per fare exit o 6 per stampare nuovamente le query disponibili\n");
}

PGconn* authentication(){
    char username[20];
    char password[20];
    printf("Inserire nome utente: ");
    scanf("%s", username);
    printf("Inserire password: ");
    scanf("%s", password);
    char conninfo[250];
    sprintf(conninfo,"user=%s password=%s dbname=%s hostaddr=%s port=%d", username, password, PG_DB, PG_HOST, PG_PORT);
    PGconn* conn;
    conn = PQconnectdb(conninfo);
    checkConnection(conn);
    return conn;
}