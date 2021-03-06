/******************************************************************************************************************
    Copyright 2014, 2015 UnoffLandz

    This file is part of unoff_server_4.

    unoff_server_4 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    unoff_server_4 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with unoff_server_4.  If not, see <http://www.gnu.org/licenses/>.
*******************************************************************************************************************/
/******************************************************************************************************************

                                    COMPILER SETTINGS

To compile server, set the following compiler flags :

    -std=c99                - target c99 compliance
    -Wconversion            - 64bit compliance
    -wall                   - all common compiler warnings
    -wextra                 - extra compiler warnings
    -pendantic              - strict iso c/c++ warnings
    -std=C++11              - target c++ 11 compliance
    -Wunreachable-code      - warn on unreachable code
    -Wredundant-decis       - warn on duplicate declarations

                                    LINKING INFORMATION

To compile server, link with the following libraries :

    libev.a                 - libev event library
    libsqlite3.so           - sqlite database library

****************************************************************************************************/
/***************************************************************************************************

                                TO - DO

DONE NEW implemented send_inventory_text protocol
DONE separate module for chat #commands


BUG Player chat in dark gray even when active
BUG Player chat shows chan 32
BUG Bingo chat shows chan -30

TEST multiple chat channel handling
TEST multiple guild application handling

bag_proximity (reveal and unreveal) use destroy and create in place of revised client code
#jump to default to first walkable tile
make separate function to extract 3d object list from elm file
reimplement function get_nearest_unoccupied_tile
need #command to withdraw application to join guild
need #letter system to inform ppl if guild application has been approved/rejected also if guild member leaves
6+separate module for ops #commands
separate module for devs #commands
transfer server welcome message to the database
#command to change guild chan join/leave notification colours
remove character_type_name field from CHARACTER_TYPE_TABLE
map object reserve respawn
#command to #letter all members of a guild
finish script loading
widen distance that new/beamed chars are from other chars
#IG guild channel functionality
OPS #command to #letter all chars
need #command to #letter all guild members (guild master only)
implement guild stats
Table to separately record all drops/pick ups in db
Table to separately record chars leaving and joining guilds
save guild applicant list to database
document idle_buffer2.h
convert attribute struct so as attribute type can be addressed programatically
identify cause of stall after login (likely to be loading of inventory from db)
identify cause of char bobbing
put inventory slots in a binary blob (may solve stall on log in)
improve error handling on upgrade_database function
refactor function current_database_version
create circular buffer for receiving packets
need #function to describe char and what it is wearing)
document new database/struct relationships
finish char_race_stats and char_gender_stats functions in db_char_tbl.c

***************************************************************************************************/

#include <stdio.h>      //supports printf function
#include <string.h>     //supports memset and strcpy functions
#include <errno.h>      //supports errno function
#include <arpa/inet.h>  //supports recv and accept function
#include <ev.h>         //supports ev event library
#include <fcntl.h>      //supports fcntl
#include <unistd.h>     //supports close function, ssize_t data type and TEMP_FAILURE_RETRY

#include "server_parameters.h"
#include "global.h"
#include "logging.h"
#include "server_messaging.h"
#include "clients.h"
#include "game_data.h"
#include "db/database_functions.h"
#include "client_protocol_handler.h"
#include "server_protocol_functions.h"
#include "db/db_attribute_tbl.h"
#include "db/db_character_race_tbl.h"
#include "db/db_character_tbl.h"
#include "db/db_character_type_tbl.h"
#include "db/db_chat_channel_tbl.h"
#include "db/db_game_data_tbl.h"
#include "db/db_gender_tbl.h"
#include "db/db_map_tbl.h"
#include "db/db_season_tbl.h"
#include "db/db_object_tbl.h"
#include "db/db_e3d_tbl.h"
#include "db/db_map_object_tbl.h"
#include "db/db_upgrade.h"
#include "db/db_guild_tbl.h"
#include "date_time_functions.h"
#include "broadcast_actor_functions.h"
#include "movement.h"
#include "server_start_stop.h"
#include "attributes.h"
#include "chat.h"
#include "characters.h"
#include "idle_buffer2.h"
#include "file_functions.h"
#include "objects.h"
#include "harvesting.h"
#include "gender.h"
#include "character_type.h"
#include "colour.h"
#include "broadcast_actor_functions.h"
#include "packet.h"
#include "bags.h"
#include "string_functions.h"
#include "maps.h"

#define _GNU_SOURCE 1   //supports TEMP_FAILURE_RETRY
#define DEBUG_MAIN 0
#define VERSION "4.4"

struct ev_io *libevlist[MAX_CLIENTS] = {NULL};

extern int current_database_version();

//declare prototypes
void socket_accept_callback(struct ev_loop *loop, struct ev_io *watcher, int revents);
void socket_read_callback(struct ev_loop *loop, struct ev_io *watcher, int revents);
void socket_write_callback(struct ev_loop *loop, struct ev_io *watcher, int revents);
void timeout_cb(EV_P_ struct ev_timer* timer, int revents);
void timeout_cb2(EV_P_ struct ev_timer* timer, int revents);
void idle_cb(EV_P_ struct ev_idle *watcher, int revents);

int sd;

void start_server(){

    /** RESULT   : starts the server

        RETURNS  : void

        PURPOSE  : code modularisation

        NOTES    :
    **/

    struct ev_loop *loop = ev_default_loop(0);

    struct ev_io *socket_watcher = (struct ev_io*)malloc(sizeof(struct ev_io));
    struct ev_idle *idle_watcher = (struct ev_idle*)malloc(sizeof(struct ev_idle));
    struct ev_timer *timeout_watcher = (struct ev_timer*)malloc(sizeof(struct ev_timer));
    struct ev_timer *timeout_watcher2 = (struct ev_timer*)malloc(sizeof(struct ev_timer));

    struct sockaddr_in server_addr;

    //check database version
    int database_version = get_database_version();

    if(database_version != REQUIRED_DATABASE_VERSION) {

        printf("Database version [%i] not equal to [%i] - use -U option to upgrade your database\n", database_version, REQUIRED_DATABASE_VERSION);
        return;
    }

    //clears garbage from the struct
    memset(&clients, 0, sizeof(clients));

    //load data from database into memory
    log_text(EVENT_INITIALISATION, "");//insert logical separator in log file

    load_db_e3ds();
    log_text(EVENT_INITIALISATION, "");//insert logical separator in log file

    load_db_objects();
    log_text(EVENT_INITIALISATION, "");//insert logical separator in log file

    load_db_maps();
    log_text(EVENT_INITIALISATION, "");//insert logical separator in log file

    load_db_map_objects();
    log_text(EVENT_INITIALISATION, "");//insert logical separator in log file

    load_db_char_races();
    log_text(EVENT_INITIALISATION, "");//insert logical separator in log file

    load_db_genders();
    log_text(EVENT_INITIALISATION, "");//insert logical separator in log file

    load_db_char_types();
    log_text(EVENT_INITIALISATION, "");//insert logical separator in log file

    load_db_attributes();
    log_text(EVENT_INITIALISATION, "");//insert logical separator in log file

    load_db_channels();
    log_text(EVENT_INITIALISATION, "");//insert logical separator in log file

    load_db_game_data();
    log_text(EVENT_INITIALISATION, "");//insert logical separator in log file

    load_db_seasons();
    log_text(EVENT_INITIALISATION, "");//insert logical separator in log file

    load_db_guilds();
    log_text(EVENT_INITIALISATION, "");//insert logical separator in log file

    //gather initial stats
    get_db_last_char_created(); //loads details of the last char created from the database into the game_data struct
    game_data.char_count=get_db_char_count();

    //create server socket & bind it to socket address
    if((sd = socket(AF_INET, SOCK_STREAM, 0))==-1){

        int errnum=errno;

        log_event(EVENT_ERROR, "socket failed in function %s: module %s: line %i", __func__, __FILE__, __LINE__);
        log_text(EVENT_ERROR, "error [%i] [%s]", errnum, strerror(errnum));
        stop_server();
    }

    //clear struct and fill with server socket data
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    log_event(EVENT_INITIALISATION, "setting up server socket on address [%s]: port [%i]", inet_ntoa(server_addr.sin_addr), PORT);

    //allow the socket to be immediately reused
    int bReuseaddr = 1;
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (const char*) &bReuseaddr, sizeof(bReuseaddr)) != 0) {

        int errnum=errno;

        log_event(EVENT_ERROR, "setsockopt failed in function %s: module %s: line %i", __func__, __FILE__, __LINE__);
        log_text(EVENT_ERROR, "error [%i] [%s]", errnum, strerror(errnum));
        stop_server();
    }

    //bind the server socket to an address
    if(bind(sd, (struct sockaddr*) &server_addr, sizeof(server_addr))==-1){

        int errnum=errno;

        log_event(EVENT_ERROR, "bind failed in function %s: module %s: line %i", __func__, __FILE__, __LINE__);
        log_text(EVENT_ERROR, "error [%i] [%s]", errnum, strerror(errnum));
        stop_server();
    }

    //listen for incoming client connections
    if(listen(sd, 5)==-1){

        int errnum=errno;

        log_event(EVENT_ERROR, "listen failed in function %s: module %s: line %i", __func__, __FILE__, __LINE__);
        log_text(EVENT_ERROR, "error [%i] [%s]", errnum, strerror(errnum));
        stop_server();
    }

    //start the event watchers
    ev_timer_init(timeout_watcher, timeout_cb, 0.05, 0.05);
    ev_timer_start(loop, timeout_watcher);

    ev_timer_init(timeout_watcher2, timeout_cb2, GAME_MINUTE_INTERVAL, GAME_MINUTE_INTERVAL);
    ev_timer_start(loop, timeout_watcher2);

    ev_io_init(socket_watcher, socket_accept_callback, sd, EV_READ);
    ev_io_start(loop, socket_watcher);

    ev_idle_init(idle_watcher, idle_cb);
    ev_idle_start(loop, idle_watcher);

    log_event(EVENT_INITIALISATION, "server initialisation complete");

    while(1) {

        ev_run(loop, 0);
    }
}

int get_next_free_client_node(){

    for(int i=0; i< MAX_CLIENTS; i++){

        if(clients.client[i].client_status==LOGGED_OUT
        && clients.client[i].client_type==NONE) return i;
    }

    return -1;
}


void socket_accept_callback(struct ev_loop *loop, struct ev_io *watcher, int revents) {

    /** RESULT   : handles socket accept event

        RETURNS  : void

        PURPOSE  : handles new client connections

        NOTES    :
    **/

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_sd;

    struct ev_io *client_watcher = (struct ev_io*) malloc(sizeof(struct ev_io));

    if (client_watcher == NULL) {

        log_event(EVENT_ERROR, "malloc failed in function %s: module %s: line %i", __func__, __FILE__, __LINE__);
        stop_server();
    }

    if (EV_ERROR & revents) {

        log_event(EVENT_ERROR, "EV error in function %s: module %s: line %i", __func__, __FILE__, __LINE__);
        stop_server();
    }

    // socket accept: get file description
    client_sd = accept(watcher->fd, (struct sockaddr*) &client_addr, &client_len);

    if (client_sd ==-1) {

        //catch accept errors
        int errnum=errno;

        log_event(EVENT_ERROR, "accept failed in function %s: module %s: line %i", __func__, __FILE__, __LINE__);
        log_text(EVENT_ERROR, "socket [%i] error [%i] [%s]", watcher->fd, errnum, strerror(errnum));
        stop_server();
    }

    //get next free node on the client node array
    int node=get_next_free_client_node();

    if (node==-1) {

        //catch connection bounds exceeded
        log_event(EVENT_ERROR, "new connection exceeds client node max [%i]", MAX_CLIENTS);

        send_text(client_sd, CHAT_SERVER, "\nSorry but the server is currently full\n");
        close(client_sd);
        return;
    }

    // listen to new client
    ev_io_init(client_watcher, socket_read_callback, client_sd, EV_READ);
    ev_io_start(loop, client_watcher);

    libevlist[client_sd] = client_watcher;

    //set up connection data entry in client struct
    clients.client[node].socket=client_sd;
    clients.client[node].client_status=CONNECTED;
    strcpy(clients.client[node].ip_address, inet_ntoa(client_addr.sin_addr));

    //set up heartbeat
    gettimeofday(&time_check, NULL);
    clients.client[node].time_of_last_heartbeat=time_check.tv_sec;

    //send welcome message and motd to client
    send_text(client_sd, CHAT_SERVER, SERVER_WELCOME_MSG);
    send_motd(client_sd);
    send_text(client_sd, CHAT_SERVER, "\nHit any key to continue...\n");
}


void socket_accept_callback2(struct ev_loop *loop, struct ev_io *watcher, int revents) {

    /** RESULT   : handles socket accept event

        RETURNS  : void

        PURPOSE  : handles new client connections

        NOTES    :
    **/

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_sd;

    struct ev_io *client_watcher = (struct ev_io*) malloc(sizeof(struct ev_io));
    if (client_watcher == NULL) {

        log_event(EVENT_ERROR, "malloc failed in function %s: module %s: line %i", __func__, __FILE__, __LINE__);
        stop_server();
    }

    if (EV_ERROR & revents) {

        log_event(EVENT_ERROR, "EV error in function %s: module %s: line %i", __func__, __FILE__, __LINE__);
        stop_server();
    }

    // socket accept: get file description
    client_sd = accept(watcher->fd, (struct sockaddr*) &client_addr, &client_len);
    if (client_sd ==-1) {

        int errnum=errno;

        log_event(EVENT_ERROR, "accept failed in function %s: module %s: line %i", __func__, __FILE__, __LINE__);
        log_text(EVENT_ERROR, "socket [%i] error [%i] [%s]", watcher->fd, errnum, strerror(errnum));
        stop_server();
    }

    // too many connections
    if (client_sd > MAX_CLIENTS) {

        log_event(EVENT_ERROR, "new connection [%i] exceeds client array max [%i] ", client_sd, MAX_CLIENTS);

        //send message to client and deny connection
        send_text(client_sd, CHAT_SERVER, "\nSorry but the server is currently full\n");
        close(client_sd);
        return;
    }

    #if DEBUG_MAIN==1
    printf("client [%i] connected\n", client_sd);
    #endif

    // listen to new client
    ev_io_init(client_watcher, socket_read_callback, client_sd, EV_READ);
    ev_io_start(loop, client_watcher);

    libevlist[client_sd] = client_watcher;

    //set up connection data entry in client struct
    clients.client[client_sd].client_status=CONNECTED;
    strcpy(clients.client[client_sd].ip_address, inet_ntoa(client_addr.sin_addr));

    //set up heartbeat
    gettimeofday(&time_check, NULL);
    clients.client[client_sd].time_of_last_heartbeat=time_check.tv_sec;

    //send welcome message and motd to client
    send_text(client_sd, CHAT_SERVER, SERVER_WELCOME_MSG);
    send_motd(client_sd);
    send_text(client_sd, CHAT_SERVER, "\nHit any key to continue...\n");
}


int find_client_node(int socket){

    for(int i=0; i< MAX_CLIENTS; i++){

        if(clients.client[i].client_status==LOGGED_IN
        && clients.client[i].client_type==PLAYER
        && clients.client[i].socket==socket) return i;
    }

    return -1;
}


void socket_read_callback(struct ev_loop *loop, struct ev_io *watcher, int revents) {

    /** RESULT   : handles socket read event

        RETURNS  : void

        PURPOSE  : handles existing client connections

        NOTES    :
    **/

    unsigned char buffer[1024];

    if (EV_ERROR & revents) {

        log_event(EVENT_ERROR, "EV error in function %s: module %s: line %i", __func__, __FILE__, __LINE__);
        stop_server();
    }

    if(EV_READ & revents){

        //find the client node
        int node=find_client_node(watcher->fd);

        if(node==-1){

            log_event(EVENT_ERROR, "cannot find socket [%i] to close in function %s: module %s: line %i", __func__, __FILE__, __LINE__);
            return;
        }

        //wrapping recv in this macro prevents connection reset by peer errors
        //read = TEMP_FAILURE_RETRY(recv(watcher->fd, buffer, 512, 0));

        ssize_t read=recv(watcher->fd, buffer, 512, 0);

        if (read <0) {

            //catch read errors
            int errnum=errno;

            if(errno == EINTR){

                log_event(EVENT_ERROR, "read registered EINTR in function %s: module %s: line %i", __func__, __FILE__, __LINE__);
                log_text(EVENT_ERROR, "sock [%i] error [%i] [%s]... ignoring", watcher->fd, errnum, strerror(errnum));
            }

            if(errno == EAGAIN){

                log_event(EVENT_ERROR, "read registered EAGAIN in function %s: module %s: line %i", __func__, __FILE__, __LINE__);
                log_text(EVENT_ERROR, "sock [%i] error [%i] [%s]... ignoring", watcher->fd, errnum, strerror(errnum));

                return;
            }

            if(errno == EWOULDBLOCK){

                log_event(EVENT_ERROR, "read registered EWOULDBLOCK in function %s: module %s: line %i", __func__, __FILE__, __LINE__);
                log_text(EVENT_ERROR, "sock [%i] error [%i] [%s]... ignoring", watcher->fd, errnum, strerror(errnum));

                return;
            }

            else{

                log_event(EVENT_ERROR, "read registered fatal error in function %s: module %s: line %i", __func__, __FILE__, __LINE__);
                log_text(EVENT_ERROR, "sock [%i] error [%i] [%s]... closing", watcher->fd, errnum, strerror(errnum));

                log_event(EVENT_SESSION, "closing client [%i] following read error", watcher->fd);

                close_connection_slot(watcher->fd);

                ev_io_stop(loop, libevlist[watcher->fd]);
                free(libevlist[watcher->fd]);
                libevlist[watcher->fd] = NULL;

                //clear the client node entry
                memset(&clients.client[node], 0, sizeof(clients.client[node]));

                return;
            }
        }

        if (read == 0) {

            #if DEBUG_MAIN==1
            printf("client [%i] disconnected\n", watcher->fd);
            #endif

            if (libevlist[watcher->fd]!= NULL) {

                //notify guild that char has logged off
                int guild_id=clients.client[node].guild_id;

                if(guild_id>0){

                    char text_out[80]="";

                    sprintf(text_out, "%c%s LEFT THE GAME", c_blue3+127, clients.client[node].char_name);
                    broadcast_guild_chat(guild_id, watcher->fd, text_out);
                }

                close_connection_slot(watcher->fd);

                ev_io_stop(loop, libevlist[watcher->fd]);
                free(libevlist[watcher->fd]);
                libevlist[watcher->fd] = NULL;

                //clear the struct
                memset(&clients.client[node], 0, sizeof(clients.client[node]));
            }
            return;
        }

        //check for data received from client
        if(read>0){

            log_event(EVENT_SESSION, "bytes received [%i]", read);

            //copy new bytes to client packet buffer
            memcpy(clients.client[node].packet_buffer+clients.client[node].packet_buffer_length, &buffer, (size_t)read);
            clients.client[node].packet_buffer_length+=(size_t)read;

            //if data is in the buffer then process it
            if(clients.client[node].packet_buffer_length>0) {

                do {

                    //update heartbeat
                    clients.client[node].time_of_last_heartbeat=time_check.tv_sec;

                    //if insufficient data to complete  a packet then wait for more data
                    size_t packet_length=get_packet_length(clients.client[node].packet_buffer);

                    if(clients.client[node].packet_buffer_length < packet_length) break;

                    //remove packet from buffer
                    clients.client[node].packet_buffer_length-=packet_length;
                    memmove(clients.client[node].packet_buffer, clients.client[node].packet_buffer + packet_length, clients.client[watcher->fd].packet_buffer_length);

                    //process the packet
                    process_packet(watcher->fd, clients.client[node].packet_buffer);

                } while(1);
            }
        }
    }
}


void socket_read_callback2(struct ev_loop *loop, struct ev_io *watcher, int revents) {

    /** RESULT   : handles socket read event

        RETURNS  : void

        PURPOSE  : handles existing client connections

        NOTES    :
    **/

    unsigned char buffer[1024];

    if (EV_ERROR & revents) {

        log_event(EVENT_ERROR, "EV error in function %s: module %s: line %i", __func__, __FILE__, __LINE__);
        stop_server();
    }

    if(EV_READ & revents){

        //wrapping recv in this macro prevents connection reset by peer errors
        //read = TEMP_FAILURE_RETRY(recv(watcher->fd, buffer, 512, 0));

        ssize_t read=recv(watcher->fd, buffer, 512, 0);

        if (read <0) {

            //catch read errors
            int errnum=errno;

            if(errno == EINTR){

                log_event(EVENT_ERROR, "read registered EINTR in function %s: module %s: line %i", __func__, __FILE__, __LINE__);
                log_text(EVENT_ERROR, "sock [%i] error [%i] [%s]... ignoring", watcher->fd, errnum, strerror(errnum));
            }

            if(errno == EAGAIN){

                log_event(EVENT_ERROR, "read registered EAGAIN in function %s: module %s: line %i", __func__, __FILE__, __LINE__);
                log_text(EVENT_ERROR, "sock [%i] error [%i] [%s]... ignoring", watcher->fd, errnum, strerror(errnum));

                return;
            }

            if(errno == EWOULDBLOCK){

                log_event(EVENT_ERROR, "read registered EWOULDBLOCK in function %s: module %s: line %i", __func__, __FILE__, __LINE__);
                log_text(EVENT_ERROR, "sock [%i] error [%i] [%s]... ignoring", watcher->fd, errnum, strerror(errnum));

                return;
            }

            else{

                log_event(EVENT_ERROR, "read registered fatal error in function %s: module %s: line %i", __func__, __FILE__, __LINE__);
                log_text(EVENT_ERROR, "sock [%i] error [%i] [%s]... closing", watcher->fd, errnum, strerror(errnum));

                log_event(EVENT_SESSION, "closing client [%i] following read error", watcher->fd);

                close_connection_slot(watcher->fd);

                ev_io_stop(loop, libevlist[watcher->fd]);
                free(libevlist[watcher->fd]);
                libevlist[watcher->fd] = NULL;

                //clear the struct


                memset(&clients.client[watcher->fd], 0, sizeof(clients.client[watcher->fd]));

                return;
            }
        }

        if (read == 0) {

            #if DEBUG_MAIN==1
            printf("client [%i] disconnected\n", watcher->fd);
            #endif

            if (libevlist[watcher->fd]!= NULL) {

                //notify guild that char has logged off
                int guild_id=clients.client[watcher->fd].guild_id;

                if(guild_id>0){

                    char text_out[80]="";

                    sprintf(text_out, "%c%s LEFT THE GAME", c_blue3+127, clients.client[watcher->fd].char_name);
                    broadcast_guild_chat(guild_id, watcher->fd, text_out);
                }

                close_connection_slot(watcher->fd);

                ev_io_stop(loop, libevlist[watcher->fd]);
                free(libevlist[watcher->fd]);
                libevlist[watcher->fd] = NULL;

                //clear the struct
                memset(&clients.client[watcher->fd], '\0', sizeof(clients.client[watcher->fd]));
            }
            return;
        }

        //check for data received from client
        if(read>0){

            log_event(EVENT_SESSION, "bytes received [%i]", read);
/*
             for(int i=0; i<read; i++){

                clients.client[watcher->fd].packet_buffer[clients.client[watcher->fd].packet_buffer_length]=buffer[i];
                clients.client[watcher->fd].packet_buffer_length++;
            }
*/
            //copy new bytes to client packet buffer
            memcpy(clients.client[watcher->fd].packet_buffer+clients.client[watcher->fd].packet_buffer_length, &buffer, (size_t)read);
            clients.client[watcher->fd].packet_buffer_length+=(size_t)read;

            //if data is in the buffer then process it
            if(clients.client[watcher->fd].packet_buffer_length>0) {

                do {

                    //update heartbeat
                    clients.client[watcher->fd].time_of_last_heartbeat=time_check.tv_sec;

                    //if insufficient data to complete  a packet then wait for more data
                    size_t packet_length=get_packet_length(clients.client[watcher->fd].packet_buffer);
                    if(clients.client[watcher->fd].packet_buffer_length<packet_length) break;

                    //grab packet for processing  and remove from buffer
                    //memcpy(packet, clients.client[watcher->fd].packet_buffer, packet_length);

                    //remove packet from buffer
                    clients.client[watcher->fd].packet_buffer_length-=packet_length;
                    memmove(clients.client[watcher->fd].packet_buffer, clients.client[watcher->fd].packet_buffer + packet_length, clients.client[watcher->fd].packet_buffer_length);

                    //process the packet
                    //process_packet(watcher->fd, packet);
                    process_packet(watcher->fd, clients.client[watcher->fd].packet_buffer);

                } while(1);
            }
        }

        //TEST CODE TO SUPPORT WRITE CALLBACK
        /*
        ev_io_stop(loop, watcher);
        ev_io_init(watcher, socket_write_callback, watcher->fd, EV_WRITE);
        ev_io_start(loop, watcher);
        */
    }
}


void timeout_cb2(EV_P_ struct ev_timer* timer, int revents){

    /**     RESULT   : handles timeout event

            RETURNS  : void

            PURPOSE  : handles game time updates

            NOTES    :
    **/

    (void)(timer);//removes unused parameter warning
    (void)(loop);

    if (EV_ERROR & revents) {

        log_event(EVENT_ERROR, "EV error in function %s: module %s: line %i", __func__, __FILE__, __LINE__);
        stop_server();
    }

    //update game time
    game_data.game_minutes++;

    if(game_data.game_minutes>360){

        game_data.game_minutes=0;
        game_data.game_days++;

        push_sql_command("UPDATE GAME_DATA_TABLE SET GAME_DAYS=%i WHERE GAME_DATA_ID=1", game_data.game_days);
    }

    push_sql_command("UPDATE GAME_DATA_TABLE SET GAME_MINUTES=%i WHERE GAME_DATA_ID=1", game_data.game_minutes);
 }


void timeout_cb(EV_P_ struct ev_timer* timer, int revents){

    /** RESULT   : handles timeout event

        RETURNS  : void

        PURPOSE  : handles fixed interval processing tasks

        NOTES    :
    **/

    (void)(timer);//removes unused parameter warning
    (void)(loop);

    if (EV_ERROR & revents) {

        log_event(EVENT_ERROR, "EV error in function %s: module %s: line %i", __func__, __FILE__, __LINE__);
        stop_server();
    }

    //update time_check struct
    gettimeofday(&time_check, NULL);

    //check through each connect client and process pending actions
    for(int i=0; i<MAX_CLIENTS; i++){

        //restrict to clients that are logged on or connected
        if((clients.client[i].client_status==LOGGED_IN || clients.client[i].client_status==CONNECTED)
        && clients.client[i].client_type==PLAYER) {

            //check for lagged connection
            if(clients.client[i].time_of_last_heartbeat+HEARTBEAT_INTERVAL<time_check.tv_sec){

                #if DEBUG_MAIN==1
                printf("Client lagged out [%i] [%s]\n", i, clients.client[i].char_name);
                #endif

                log_event(EVENT_SESSION, "client [%i] char [%s] lagged out", i, clients.client[i].char_name);

                close_connection_slot(clients.client[i].socket);

                ev_io_stop(loop, libevlist[clients.client[i].socket]);
                free(libevlist[clients.client[i].socket]);
                libevlist[clients.client[i].socket] = NULL;

                memset(&clients.client[i], 0, sizeof(clients.client[i]));
            }

            //restrict to clients that are logged on
            if(clients.client[i].client_status==LOGGED_IN) {

                //update client game time
                if(clients.client[i].time_of_last_minute+GAME_MINUTE_INTERVAL<time_check.tv_sec){

                    clients.client[i].time_of_last_minute=time_check.tv_sec;
                    send_new_minute(i, game_data.game_minutes);

                    //update database with time char was last in game
                    push_sql_command("UPDATE CHARACTER_TABLE SET LAST_IN_GAME=%i WHERE CHAR_ID=%i;",(int)clients.client[i].time_of_last_minute, clients.client[i].character_id);
                }

                //process any char movements
                process_char_move(i, time_check.tv_usec); //use milliseconds

                //process any harvesting
                process_char_harvest(i, time_check.tv_sec); //use seconds
            }
        }
    }

    // check bags for poof time
    for(int i=0; i<MAX_BAGS; i++){

         if(bag[i].bag_refreshed>0 && bag[i].bag_refreshed+BAG_POOF_INTERVAL<time_check.tv_sec){

            //poof the bag
            broadcast_destroy_bag_packet(i);
        }
    }
}

void timeout_cb_old(EV_P_ struct ev_timer* timer, int revents){

    /** RESULT   : handles timeout event

        RETURNS  : void

        PURPOSE  : handles fixed interval processing tasks

        NOTES    :
    **/

    (void)(timer);//removes unused parameter warning
    (void)(loop);

    if (EV_ERROR & revents) {

        log_event(EVENT_ERROR, "EV error in function %s: module %s: line %i", __func__, __FILE__, __LINE__);
        stop_server();
    }

    //update time_check struct
    gettimeofday(&time_check, NULL);

    //check through each connect client and process pending actions
    for(int i=0; i<MAX_CLIENTS; i++){

        //restrict to clients that are logged on or connected
        if(clients.client[i].client_status==LOGGED_IN || clients.client[i].client_status==CONNECTED) {

            //check for lagged connection
            if(clients.client[i].time_of_last_heartbeat+HEARTBEAT_INTERVAL<time_check.tv_sec){

                #if DEBUG_MAIN==1
                printf("Client lagged out [%i] [%s]\n", i, clients.client[i].char_name);
                #endif

                log_event(EVENT_SESSION, "client [%i] char [%s] lagged out", i, clients.client[i].char_name);

                close_connection_slot(i);

                ev_io_stop(loop, libevlist[i]);
                free(libevlist[i]);
                libevlist[i] = NULL;

                memset(&clients.client[i], '\0', sizeof(clients.client[i]));
            }

            //restrict to clients that are logged on
            if(clients.client[i].client_status==LOGGED_IN) {

                //update client game time
                if(clients.client[i].time_of_last_minute+GAME_MINUTE_INTERVAL<time_check.tv_sec){

                    clients.client[i].time_of_last_minute=time_check.tv_sec;
                    send_new_minute(i, game_data.game_minutes);

                    //update database with time char was last in game
                    push_sql_command("UPDATE CHARACTER_TABLE SET LAST_IN_GAME=%i WHERE CHAR_ID=%i;",(int)clients.client[i].time_of_last_minute, clients.client[i].character_id);
                }

                //process any char movements
                process_char_move(i, time_check.tv_usec); //use milliseconds

                //process any harvesting
                process_char_harvest(i, time_check.tv_sec); //use seconds
            }
        }
    }

    // check bags for poof time
    for(int i=0; i<MAX_BAGS; i++){

         if(bag[i].bag_refreshed>0 && bag[i].bag_refreshed+BAG_POOF_INTERVAL<time_check.tv_sec){

            //poof the bag
            broadcast_destroy_bag_packet(i);
        }
    }
}



void idle_cb (struct ev_loop *loop, struct ev_idle *watcher, int revents){

    /** RESULT   : handles server idle event

        RETURNS  : void

        PURPOSE  : enables idle event to be used for low priority processing tasks

        NOTES    :
    **/

    (void)(loop);
    (void)(watcher);

    if (EV_ERROR & revents) {

        log_event(EVENT_ERROR, "EV error in function %s: module %s: line %i", __func__, __FILE__, __LINE__);
        stop_server();
    }

    process_idle_buffer2();
}


int main(int argc, char *argv[]){

    /** RESULT   : handles command line arguments

        RETURNS  : dummy

        PURPOSE  : allows program to be started in different modes

        NOTES    :
    **/

    printf("UnoffLandz Server - version %s\n\n", VERSION);

    char db_filename[80]=DEFAULT_DATABASE_FILE_NAME;

    struct{

        bool start_server;
        bool create_database;
        bool upgrade_database;
        bool load_map;
        bool list_maps;
        bool help;
    }option;

    //clear struct to prevent garbage
    memset(&option, 0, sizeof(option));

    //set server start time
    game_data.server_start_time=time(NULL);

    //prepare start time for console and log message
    char time_stamp_str[9]="";
    char verbose_date_stamp_str[50]="";
    get_time_stamp_str(game_data.server_start_time, time_stamp_str);
    get_verbose_date_str(game_data.server_start_time, verbose_date_stamp_str);

    //clear logs
    initialise_logs();

    //parse command line
    log_text(EVENT_INITIALISATION, "Command line option count [%i]", argc);
    log_text(EVENT_INITIALISATION, "Parse command line...");

    for(int i=0; i<argc; i++){

        if (strcmp(argv[i], "-S") == 0)option.start_server=true;
        if (strcmp(argv[i], "-C") == 0)option.create_database=true;
        if (strcmp(argv[i], "-U") == 0)option.upgrade_database=true;
        if (strcmp(argv[i], "-M") == 0)option.load_map=true;
        if (strcmp(argv[i], "-L") == 0)option.list_maps=true;
        if (strcmp(argv[i], "-H") == 0)option.help=true;

        log_text(EVENT_INITIALISATION, "%i [%s]", i, argv[i]);// log each command line option
    }

    log_text(EVENT_INITIALISATION, "");// insert logical separator

    //execute start server
    if(option.start_server==true){

        if(argc==3) strcpy(db_filename, argv[2]);

        printf("SERVER START using %s at %s on %s\n", db_filename, time_stamp_str, verbose_date_stamp_str);

        log_text(EVENT_INITIALISATION, "SERVER START using %s at %s on %s", db_filename, time_stamp_str, verbose_date_stamp_str);
        log_text(EVENT_INITIALISATION, "");// insert logical separator

        open_database(db_filename);
        start_server();

        return 0;
    }

    //execute create database
    else if(option.create_database==true){

        if(argc==3) strcpy(db_filename, argv[2]);

        printf("CREATE DATABASE using %s at %s on %s\n", db_filename, time_stamp_str, verbose_date_stamp_str);

        log_text(EVENT_INITIALISATION, "CREATE DATABASE using %s at %s on %s", db_filename, time_stamp_str, verbose_date_stamp_str);
        log_text(EVENT_INITIALISATION, "");// insert logical separator

        create_database(db_filename);
        //_create_database("load_script.txt");

        return 0;
    }

    //execute upgrade database
    else if(option.upgrade_database==true){

        if(argc==3) strcpy(db_filename, argv[2]);

        printf("UPGRADE DATABASE using %s at %s on %s\n", db_filename, time_stamp_str, verbose_date_stamp_str);

        log_text(EVENT_INITIALISATION, "UPGRADE DATABASE using %s at %s on %s", db_filename, time_stamp_str, verbose_date_stamp_str);
        log_text(EVENT_INITIALISATION, "");// insert logical separator

        open_database(db_filename);
        upgrade_database(db_filename);

        return 0;
    }

    //execute load map
    else if(option.load_map==true && argc>=9){

        if(argc==10) strcpy(db_filename, argv[9]);

        int map_id=atoi(argv[2]);

        printf("LOAD MAP %i on %s at %s on %s\n", map_id, db_filename, time_stamp_str, verbose_date_stamp_str);
        log_text(EVENT_INITIALISATION, "LOAD MAP %i on %s at %s on %s", map_id, db_filename, time_stamp_str, verbose_date_stamp_str);

        open_database(db_filename);

        //load maps so we can find out if a map exists
        load_db_maps();

        if(get_db_map_exists(map_id)==true){

            printf( "Do you wish to replace map [%i] [%s] Y/N: ", map_id, maps.map[map_id].map_name);

            //get decision from stdin
            char decision[1]={0};
            if(fgets(decision, sizeof(decision), stdin)!=NULL){

                log_event(EVENT_ERROR, "something failed in fgets in function %s: module %s: line %i", __func__, __FILE__, __LINE__);
                stop_server();
            }

            //convert decision to upper case and determine if map replacement should proceed
            str_conv_upper(decision);
            if(strcmp(decision, "Y")==0){

                log_text(EVENT_INITIALISATION, "Remove existing map %i", map_id);
                delete_map(map_id);
            }

            printf("\n");
        }

        log_text(EVENT_INITIALISATION, "");// insert logical separator

        char elm_filename[80]="";
        strcpy(elm_filename, argv[3]);

        char map_name[80]="";
        strcpy(map_name, argv[4]);

        char map_description[80]="";
        strcpy(map_description, argv[5]);

        char author_name[80]="";
        strcpy(author_name, argv[6]);

        char author_email[80]="";
        strcpy(author_email, argv[7]);

        int development_status=atoi(argv[8]);

        add_db_map(map_id, elm_filename, map_name, author_name, map_description, author_email, development_status);

        return 0;
    }

    //execute list maps
    else if(option.list_maps==true){

        if(argc>2) strcpy(db_filename, argv[2]);

        printf("LIST MAPS using %s at %s on %s\n", db_filename, time_stamp_str, verbose_date_stamp_str);

        log_text(EVENT_INITIALISATION, "LIST MAPS using %s at %s on %s", db_filename, time_stamp_str, verbose_date_stamp_str);
        log_text(EVENT_INITIALISATION, "");// insert logical separator

        open_database(db_filename);
        list_db_maps();

        return 0;
    }

    //display command line options if no command line options are found or command line options are not recognised
    printf("Command line options...\n");
    printf("create database  -C optional [""database file name""]\n");
    printf("start server     -S optional [""database file name""]\n");
    printf("upgrade database -U optional [""database file name""]\n");
    printf("list loaded maps -L optional [""database file name""]\n");
    printf("load map         -M [map id] [""elm filename""] [""map name""] [""map description""]...\n");
    printf("                             [""author name""] [""author email""]...\n");
    printf("                             [""development status code""]...\n");
    printf("                             optional [""database file name""]\n");

    return 0;//otherwise we get 'control reached end of non void function'
}
