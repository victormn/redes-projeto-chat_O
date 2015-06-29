#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <errno.h>
#include <vector>
#include <netdb.h>
#include <map>
#include <thread>

using namespace std;

typedef struct node{

    int sockServer;
    int IP;
    string name;

    map<string, int> friends;
    map<string, char*> buffer;   
    map<string, char*> ip;  

} Node;

Node serverNode;

vector<thread> rcvThread;

void init();
void acceptConnection();
void sendMessages(string rcvName);
void receiveMessages(string rcvName);
void addContact(char* serverIP);
void removeContact(string removedCont);
void sendGroupMessage();
int listFriends();
void listFriendsIP();
void showMessage();
int menu();
int isHostAlive(string hostAlive);

// Verifica se um host está online: retorna '1' se online, '0' caso contrário
int isHostAlive(string hostAlive){
 
    fd_set socket_set;
    struct timeval timer;
    int ret;
    
    int sockAlive = serverNode.friends.find(hostAlive)->second; 
  
    FD_ZERO(&socket_set);
    FD_SET(sockAlive, &socket_set);
  
    timer.tv_sec = 3;
    timer.tv_usec = 0;

    ret = select(sockAlive+1,&socket_set, &socket_set, NULL, &timer);

    if (ret > 1 || errno == EHOSTDOWN) {
        removeContact(hostAlive);
        close(sockAlive);
        return 0;
    }    
  
    return 1;

}

void init(){

    // Criar socket
    // Criar vetores: amigos e conexoes
    // Conectar com o líder
    // Repassar suas informações
    
    /* Função 'socket': int socket(int socket_family, int socket_type, int protocol) */
    // (1) AF_INET: protocolo IPv4
    // (2) SOCK_STREAM: provê fluxo de dados confiável  
    // (3) 0: Internet Protocol (default) 
    
    int sock, truee = 1;
    struct sockaddr_in serverAddr;
   
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1){

      perror("\x1B[33m!!! Erro ao criar Socket (socket) !!!\x1B[0m");
      exit(1);

    } 
   
    /* Função 'setsocktopt': int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen); */
    // (1) sock: o socket referenciado
    // (2) SOL_SOCKET: nível do socket
    // (3) SO_REUSEADDR: permitir que várias conexões sejam realizadas (default é 1)
   
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &truee,sizeof(int)) == -1){
   
       perror("\x1B[33m!!! Erro ao configurar Socket (setsockopt) !!!\x1B[0m");
       exit(1);
    }  
   
    // Configurando endereço de destino do 'serverAddr' 
   
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8548);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(serverAddr.sin_zero),8);
   
    /* Função 'bind': int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) */ 
    // (1) sock: o socket referenciado
    // (2) sockaddr: endereço a ser atribuído ao socket
    // (3) addr_len: tamanho do endereço atribuído
     
    if (bind(sock, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr)) == -1){
   
       perror("\x1B[33m !!! Erro ao vincular endereco do socket (bind) !!!\x1B[0m");
       exit(1);
    }
  
    /* Função 'listen': int listen(int sockfd, int backlog) */
    // (1) sockfd: socket referenciado
    // (2) backlog: número de conexões pendentes aceitas
    
    if (listen(sock, 10) == -1){
   
       perror("\x1B[33m !!! Erro de Listen !!!\x1B[0m");
       exit(1);
    }
    
    char cName[16];
    
    printf("\tBem vindo ao CHAT-O\n\n");
    
    printf("\x1B[33m\tSeu nome: ");
    scanf("%s", cName);  
   
    string name = cName;
       
    // Configurando valores de 'Node' para o socket criado 
  
    serverNode.name = name; 
    serverNode.sockServer = sock; 

    system("clear");
         
    //printf("\nNo TCP esperando por conexoes na porta 8548 (Servidor)\n");
    fflush(stdout);    
    
}

void acceptConnection(){ // thread

    while(1){
    
      struct sockaddr_in clientAddr;
      const char *yourName = serverNode.name.c_str();
      char contactName[50];
  
      unsigned int sAddrSize = sizeof(struct sockaddr_in);
  
      int connected = accept(serverNode.sockServer, (struct sockaddr *)&clientAddr, &sAddrSize);        
              
      // enviar uma mensagem e esperar receber uma resposta (nome)  
      send(connected, yourName, strlen(yourName), 0);
      sleep(1);
      int nameTam = recv(connected,contactName,1024,0);
      contactName[nameTam] = '\0';
      
      string server_name(contactName);
      
      //------------------------------------------------

      if (connected > 0)
      printf("\nVoce foi adicionado por \033[22;31m%s\x1B[0m!\n", contactName);             
      
      char *buffer = (char*)calloc(2048, sizeof(char));

      serverNode.friends.insert(pair<string,int>(server_name,connected));
      serverNode.buffer.insert(pair<string,char*>(server_name,buffer));
      serverNode.ip.insert(pair<string,char*>(server_name,inet_ntoa(clientAddr.sin_addr)));
      
      rcvThread.push_back(thread(receiveMessages, server_name));  
     
    }
}

void sendMessages(string rcvName){

    int connected = serverNode.friends.find(rcvName)->second;   
    
    const char* cRcvName = rcvName.c_str();

    if(connected == 0){

      printf("\n\x1B[33m!!! Erro no nome !!!\x1B[0m\n");
      return;

    }

    if(isHostAlive(rcvName) == 0){
      printf("\n! \033[22;31m%s \x1B[0mesta offline !\n", cRcvName);
      return;
    }

    printf("\n!Digite \033[22;31mENVIAR \x1B[0m para enviar a mensagem e voltar ao menu!\n");
            
    printf("\x1B[32m\nPara \033[22;37m%s:\n\n", cRcvName);
    printf("\x1B[0m");
        
    char send_data[2048];

    while (1){

     fgets(send_data, 2048, stdin);
     

      if (strcmp(send_data,"ENVIAR\n") == 0){
     
        fflush(stdout);
        break;

      }else{           
        // Função send(int socket, void*buffer, size_t size, int flags)
        send(connected, send_data, strlen(send_data), 0);
      }
             
    }
}

void receiveMessages(string rcvName){ // thread
          
    int connected = serverNode.friends.find(rcvName)->second;  

    while(1){  
    
    const char* cRcvName = rcvName.c_str();
    if(isHostAlive(rcvName) == 0){
      printf("\n! \033[22;31m%s \x1B[0mesta offline !\n", cRcvName);
      return;
    }

      char *tempBuffer = (char*)calloc(2048, sizeof(char));     
    
      int tamMessage = recv(connected, tempBuffer, 2048, 0);   

      if(strcmp(tempBuffer,"EXCLUIR") == 0){

        printf("\n! \033[22;31m%s \x1B[0mte excluiu !\n", cRcvName);
        removeContact(rcvName);
        break;

      } 
      
      strcat(serverNode.buffer.find(rcvName)->second,tempBuffer);
                            
      fflush(stdout);
    }
}


void addContact(char* serverIP){

    struct hostent *host;
    struct sockaddr_in server_addr;
    int sock;
    const char *yourName = serverNode.name.c_str();
    char contactName[50];
    
    host = gethostbyname(serverIP);
    
   if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1){
   
      perror("\x1B[33m!!! Erro de Socket !!!\x1B[0m");
      exit(1);
   }
   
   server_addr.sin_family = AF_INET;
   server_addr.sin_port = htons(8548);
   server_addr.sin_addr = *((struct in_addr *)host->h_addr);
   bzero(&(server_addr.sin_zero),8);
   
   if (connect(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1){
   
      perror("\x1B[33m!!! Erro de conexao !!!\x1B[0m");
      return;
   }

    // enviar uma mensagem e esperar receber uma resposta (nome)  
    send(sock, yourName, strlen(yourName), 0);
    sleep(1);
    int nameTam = recv(sock,contactName,2048,0);
    contactName[nameTam] = '\0';
    
    string server_name(contactName);
    //------------------------------------------------


      
  char *buffer = (char*)calloc(2048, sizeof(char));

  serverNode.friends.insert(pair<string,int>(server_name,sock));
  serverNode.buffer.insert(pair<string,char*>(server_name,buffer));
  serverNode.ip.insert(pair<string,char*>(server_name,serverIP));

  rcvThread.push_back(thread(receiveMessages, server_name));


}

void removeContact(string removedCont){

 
    serverNode.friends.erase(removedCont);
    serverNode.buffer.erase(removedCont);
    serverNode.ip.erase(removedCont);  
    close(serverNode.sockServer);
}


void sendGroupMessage(){
   
    char send_data[2048];
    string sockNames[10];
    string name = "init";
    char cName[50];
    int i, j, flagEnviar = 0;

  if(serverNode.friends.size() == 0){
    printf("\n\x1B[33m!!! Nenhum contato !!!\x1B[0m\n");
    return;
  }
    
    printf("Insira os contatos para enviar a mensagem em grupo e digite 'FIM':\n");
    
    for(i=0; name != "FIM" && i < serverNode.friends.size(); i++){ 
               
        printf("Contato %d: ", i);
        scanf("%s", cName);
        name = cName;
        sockNames[i] = name;       
    }

    printf("\n!Digite \033[22;31mENVIAR \x1B[0m para enviar a mensagem e voltar ao menu!\n");
            
    printf("Mensagem em grupo: \n");   
    
    while (1){

      fgets(send_data, 2048, stdin);
     
      for(j=0; j<i; j++){
     
        int connected = serverNode.friends.find(sockNames[j])->second;          
 
        if (strcmp(send_data,"ENVIAR\n") == 0){
       
          fflush(stdout);
          flagEnviar = 1;
          break;

        }else{           
          // Função send(int socket, void*buffer, size_t size, int flags)
          send(connected, send_data, strlen(send_data), 0);
        }
      }
      if(flagEnviar == 1) break;
    }

}

int listFriends(){

  if(serverNode.friends.size() == 0){
    printf("\n\x1B[33m!!! Nenhum contato !!!\x1B[0m\n");
    return 1;
  }

  for (map<string,char*>::iterator it=serverNode.ip.begin(); it!=serverNode.ip.end(); ++it){
      
    const char *tempName = it->first.c_str();
    if(isHostAlive(it->first) == 0){
      printf("\n! \033[22;31m%s \x1B[0mesta offline !\n", tempName);
      return 1;
    }

  }

  printf("\033[01;31m- Amigos:\n");
  printf("\x1B[0m");

  for (map<string,int>::iterator it=serverNode.friends.begin(); it!=serverNode.friends.end(); ++it){
      
      const char *tempName = (it->first).c_str();
      
      printf(" # %s\n", tempName);

  }

  printf("\n");

  return 0;
}

void listFriendsIP(){

  for (map<string,char*>::iterator it=serverNode.ip.begin(); it!=serverNode.ip.end(); ++it){
      
    const char *tempName = it->first.c_str();
    if(isHostAlive(it->first) == 0){
      printf("\n! \033[22;31m%s \x1B[0mesta offline !\n", tempName);
      return;
    }

  }
  
  if(serverNode.friends.size() == 0){
    printf("\n\x1B[33m!!! Nenhum contato !!!\x1B[0m\n");
    return;
  }

  printf("\033[01;31m- Amigos:\n");
  printf("\x1B[0m");

  for (map<string,char*>::iterator it=serverNode.ip.begin(); it!=serverNode.ip.end(); ++it){
      
      const char *tempName = (it->first).c_str();
      
      printf(" # %s\t\t%s\n", tempName, it->second);

  }

  printf("\n");
}

void removeACK(string removedCont){
  
  char msm[8] = "EXCLUIR";

  int connected = serverNode.friends.find(removedCont)->second;
  send(connected, msm, strlen(msm), 0);

}

void showMessage(){

  if(serverNode.friends.size() == 0){
    printf("\n\x1B[33m!!! Nenhum contato !!!\x1B[0m\n");
    return;
  }

  for (map<string,char*>::iterator it=serverNode.ip.begin(); it!=serverNode.ip.end(); ++it){
      
    const char *tempName = it->first.c_str();
    if(isHostAlive(it->first) == 0){
      printf("\n! \033[22;31m%s \x1B[0mesta offline !\n", tempName);
      return;
    }

  }

  char *tempBuffer = (char*)calloc(2048, sizeof(char));  

  for (map<string,char*>::iterator it=serverNode.buffer.begin(); it!=serverNode.buffer.end(); ++it){

      const char *tempName = (it->first).c_str();

      printf("\x1B[32m\n! \033[22;37m%s \x1B[32mdisse: \n", tempName);
      printf("\x1B[0m");
 
      it->second[strlen(it->second)] = '\0';
      
      printf("%s", it->second);  

      strcpy(it->second, tempBuffer);

  }
}

int menu(){

    printf("\033[22;31m\n\t[1] Adicionar contato\n");
    printf("\x1B[33m\t[2] Listar amigos\n");      
    printf("\x1B[34m\t[3] Exibir mensagens\n");
    printf("\x1B[32m\t[4] Enviar mensagens\n");
    printf("\x1B[35m\t[5] Enviar mensagens em grupo\n");
    printf("\x1B[36m\t[6] Excluir contato\n");
    printf("\x1B[0m\t[7] Sair\n\n");
    printf("\x1B[0m");
    
    int botao, ret = 1, list;
    char ip[20], contactName[50];
    string auxName;
    
    scanf("%d", &botao);
    
    switch(botao){
    
        case 1: 
                //system("clear");
                printf("IP do contato: ");
                scanf("%s", ip);
                addContact(ip);
                fflush(stdout);
                break;
        case 2:
                listFriendsIP();
                fflush(stdout);
                break;
           
        case 3:
                //system("clear");
                showMessage();
                break;
                
        case 4:
                //system("clear");
                list = listFriends();
                if(list == 0){
                  printf("Nome do contato: ");
                  scanf("%s", contactName);
                  auxName=contactName;
                  sendMessages(auxName);
                  fflush(stdout);
                }
                break;  
        case 5:  
                list = listFriends();
                if(list == 0)
                  sendGroupMessage();
                break;    
        case 6:
                list = listFriends();
                if(list == 0){
                  printf("Excluir contato: ");
                  scanf("%s", contactName);
                  auxName=contactName;
                  removeACK(auxName);
                  removeContact(auxName);
                  fflush(stdout);
                }
                break;  
                
        case 7: 
                ret = 0;
                break;

       default:
              break;         
 
    
    }
      
    return ret;

}


int main(){    

    system("clear");

    init();
  
    thread accept(acceptConnection);
    
    while(1){
        
        if (menu() == 0){        
          return 0;
        }
    }
    for (auto& th : rcvThread) th.join(); 
    printf("\033[0m");
} 