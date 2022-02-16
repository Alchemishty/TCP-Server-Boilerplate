#include <iostream>
#include <SDL/SDL.h>
#include <SDL/SDL_net.h>
#include <vector>
#include <cstring>

using namespace std;

/*
Response Code Legend
1 - Change code received from a client for everyone
2 - Exit code received from a client
3 - Change code received from one client for another
*/

struct clientData
{
    TCPsocket socket;
    Uint32 timeout;
    int id;
    clientData(TCPsocket sock, Uint32 t, int i) : socket(sock), timeout(t), id(i){};
};

int main(int argc, char **argv)
{
    SDL_Init(SDL_INIT_EVERYTHING);
    SDLNet_Init();
    int curID = 0, clientNum = 0, maxClients = 30;
    SDL_Event event;
    IPaddress ip;
    SDLNet_ResolveHost(&ip, NULL, 1234);
    vector<clientData> socketVector;
    char temp[1400];
    bool running = true;
    SDLNet_SocketSet sockets = SDLNet_AllocSocketSet(30);
    SDL_SetVideoMode(640, 480, 32, SDL_SWSURFACE);
    TCPsocket server = SDLNet_TCP_Open(&ip);

    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type = SDL_QUIT || event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
            {
                running = false;
            }
            TCPsocket tempSocket = SDLNet_TCP_Accept(server);
            if (tempSocket)
            {
                if (clientNum < maxClients)
                {
                    SDLNet_TCP_AddSocket(sockets, tempSocket);
                    socketVector.push_back(clientData(tempSocket, SDL_GetTicks(), curID));
                    clientNum++;
                    sprintf(temp, "0 %d \n", curID);
                    curID++;
                    cout << "New connection" << curID << "\n";
                }
                else
                {
                    sprintf(temp, "3 \n");
                }
                SDLNet_TCP_Send(tempSocket, temp, strlen(temp) + 1);
            }

            // Check for Incoming Messages
            while (SDLNet_CheckSockets(sockets, 0) > 0)
            {
                for (int i = 0; i < socketVector.size(); i++)
                {
                    if (SDLNet_SocketReady(socketVector[i].socket))
                    {
                        socketVector[i].timeout = SDL_GetTicks();
                        SDLNet_TCP_Recv(socketVector[i].socket, temp, 1400);
                        int num = temp[0] - '0';
                        int j = 1;
                        while (temp[j] >= '0' && temp[j] <= '9')
                        {
                            num *= 10;
                            num += temp[j] - '0';
                            j++;
                        }

                        if (num == 1)
                        {
                            for (int k = 0; k < socketVector.size(); k++)
                            {
                                if (k == i)
                                    continue;
                                SDLNet_TCP_Send(socketVector[k].socket, temp, strlen(temp) + 1);
                            }
                        }
                        else if (num == 2)
                        {
                            for (int k = 0; k < socketVector.size(); k++)
                            {
                                if (k == i)
                                    continue;
                                SDLNet_TCP_Send(socketVector[k].socket, temp, strlen(temp) + 1);
                            }
                            SDLNet_TCP_DelSocket(sockets, socketVector[i].socket);
                            SDLNet_TCP_Close(socketVector[i].socket);
                            socketVector.erase(socketVector.begin() + i);
                            clientNum--;
                        }
                        else if (num == 3)
                        {
                            int rID;
                            sscanf(temp, "3 %d", &rID);
                            for (int k = 0; k < socketVector.size(); k++)
                            {
                                if (socketVector[k].id == rID)
                                {
                                    SDLNet_TCP_Send(socketVector[k].socket, temp, strlen(temp) + 1);
                                    break;
                                }
                            }
                        }
                    }
                }
            }

            // Disconnect at timeout (5 seconds)
            for (int j = 0; j < socketVector.size(); j++)
            {
                if (SDL_GetTicks() - socketVector[j].timeout > 5000)
                {
                    sprintf(temp, "2 %d \n", socketVector[j].id);
                    for (int k = 0; k < socketVector.size(); k++)
                    {
                        SDLNet_TCP_Send(socketVector[k].socket, temp, strlen(temp) + 1);
                    }
                    SDLNet_TCP_DelSocket(sockets, socketVector[j].socket);
                    SDLNet_TCP_Close(socketVector[j].socket);
                    socketVector.erase(socketVector.begin() + j);
                    clientNum--;
                }
                SDL_Delay(1);
            }
        }
    }

    for (int i = 0; i < socketVector.size(); i++)
    {
        SDLNet_TCP_Close(socketVector[i].socket);
    }
    SDLNet_FreeSocketSet(sockets);
    SDLNet_TCP_Close(server);
    SDLNet_Quit();
    SDL_Quit();
}