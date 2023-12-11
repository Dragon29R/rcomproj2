#define USER_NAME_OK 331
#define LOGIN_OK 230
#define SERVER_READY 220

#define MAX_SIZE  400
struct{
    char user_name[MAX_SIZE];
    char password[MAX_SIZE];
    char file[MAX_SIZE];
    char resource[MAX_SIZE];
    char host[MAX_SIZE];
    char ip[MAX_SIZE];
}RESOURCE;

typedef enum{
    Start,
    End
    }STATE;
