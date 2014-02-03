#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "datetime_functions.h"
#include "global.h"
#include "protocol.h"
#include "string_functions.h"
#include "chat.h"
#include "files.h"
#include "motd.h"

void send_motd_header(int connection){

    /** public function - see header */

    char text_out[1024]="";
    char time_stamp_str[9]="";
    char verbose_date_stamp_str[50]="";
    int i=0;
    char chars_in_game_str[1024]="";
    int chars_in_game_count=0;
    int char_id=0;
    int chars_created_count=0;
    int last_char_creation_date=0;
    int last_char_created_id=0;
    long int server_up_time=0;
    char time_up_str[50]="";

    //send separator line so MOTD is distinct from previous text
    send_server_text(connection, CHAT_SERVER, " ");

    //prepare and send server 'up' time
    server_up_time=time(NULL)-server_start_time;
    get_time_up_str(server_up_time, time_up_str);

    sprintf(text_out, "%cServer up                  : %s", c_blue2+127, time_up_str);

    send_server_text(connection, CHAT_SERVER, text_out);

    //prepare and send server start time
    get_time_stamp_str(server_start_time, time_stamp_str);
    get_verbose_date_str(server_start_time, verbose_date_stamp_str);

    sprintf(text_out, "%cServer started at          : %s on %s", c_blue2+127, time_stamp_str, verbose_date_stamp_str);

    send_server_text(connection, CHAT_SERVER, text_out);

    //prepare and send characters in game
    for(i=0; i<clients.max; i++){

        if(clients.client[i]->status==LOGGED_IN){
            char_id=clients.client[i]->character_id;
            sprintf(chars_in_game_str, "%s %s",  chars_in_game_str, characters.character[char_id]->char_name);
            chars_in_game_count++;
        }
    }

    if(chars_in_game_count==0) {
        sprintf(text_out, "%cCharacters in game         : [0] None", c_blue2+127);
    }
    else {
        sprintf(text_out, "%cCharacters in game         : [%i] %s", c_blue2+127, chars_in_game_count, chars_in_game_str);
    }

    send_server_text(connection, CHAT_SERVER, text_out);

    //prepare and send number of characters created
    for(i=0;i<characters.count; i++){
        chars_created_count++;
    }

    sprintf(text_out, "%cCharacters created to date : %i", c_blue2+127, chars_created_count);

    send_server_text(connection, CHAT_SERVER, text_out);

    //prepare and send last character created
    for(i=0; i<characters.count; i++){

        if(characters.character[i]->char_created>last_char_creation_date){
            last_char_creation_date=characters.character[i]->char_created;
            last_char_created_id=i;
        }
    }

    get_time_stamp_str(last_char_creation_date, time_stamp_str);
    get_verbose_date_str(last_char_creation_date, verbose_date_stamp_str);

    sprintf(text_out, "%cLast character created     : %s at %s %s", c_blue2+127, characters.character[last_char_created_id]->char_name, time_stamp_str, verbose_date_stamp_str);

    send_server_text(connection, CHAT_SERVER, text_out);

    //prepare and send time and date of this connection
    get_time_stamp_str(time(NULL), time_stamp_str);
    get_verbose_date_str(time(NULL), verbose_date_stamp_str);

    sprintf(text_out, "%cConnection at              : %s on %s", c_blue2+127, time_stamp_str, verbose_date_stamp_str);

    send_server_text(connection, CHAT_SERVER, text_out);

    //prepare and send connection ip address
    sprintf(text_out, "%cConnection IP address      : %s", c_blue2+127, clients.client[connection]->ip_address);

    send_server_text(connection, CHAT_SERVER, text_out);

    //send separator line so MOTD is distinct from subsequent text
    send_server_text(connection, CHAT_SERVER, " ");//separator
}

int send_motd_file(int connection){

    FILE *file;
    char line_in[1024]="";

    if((file=fopen(MOTD_FILE, "r"))==NULL) return FILE_NOT_FOUND;

    while( fgets(line_in, 80, file) != NULL){
        str_trim_right(line_in);
        if(strcmp(line_in, "")==0) strcpy(line_in, " ");
        send_server_text(connection, CHAT_SERVER, line_in);
    }

    fclose(file);

    return FILE_FOUND;
}

void send_motd(int connection){

    /** public function - see header */

    send_motd_header(connection);
    send_motd_file(connection);
}