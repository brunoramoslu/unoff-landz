#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> //needed for send function
#include <sys/time.h>   //needed for gettimeofday function
#include <string.h>

#include "character_inventory.h"
#include "global.h"

void send_get_new_inventory_item(int connection, int item_image_id, int amount, int slot){

    unsigned char packet[11];

    packet[0]=GET_NEW_INVENTORY_ITEM;

    packet[1]=9;
    packet[2]=0;

    packet[3]=item_image_id % 256;
    packet[4]=item_image_id / 256;

    packet[5]=amount % 256;
    packet[6]=amount / 256 % 256;
    packet[7]=amount / 256 / 256 % 256;
    packet[8]=amount / 256 / 256 / 256 % 256;

    packet[9]=slot;

    packet[10]=0;//flags

    send(connection, packet, 11, 0);
}

void send_here_your_inventory(int connection){

    int i=0;
    unsigned char packet[(MAX_INVENTORY_SLOTS*8)+4];

    int data_length=2+(MAX_INVENTORY_SLOTS*8);
    int j;

    packet[0]=HERE_YOUR_INVENTORY;
    packet[1]=data_length % 256;
    packet[2]=data_length / 256;

    packet[3]=MAX_INVENTORY_SLOTS;


    for(i=0; i<MAX_INVENTORY_SLOTS; i++){

        j=4+(i*8);

        packet[j+0]=clients.client[connection]->client_inventory[i].image_id % 256; //image_id of item
        packet[j+1]=clients.client[connection]->client_inventory[i].image_id / 256;

        packet[j+2]=clients.client[connection]->client_inventory[i].amount % 256; //amount (when zero nothing is shown in inventory)
        packet[j+3]=clients.client[connection]->client_inventory[i].amount / 256 % 256;
        packet[j+4]=clients.client[connection]->client_inventory[i].amount / 256 / 256 % 256;
        packet[j+5]=clients.client[connection]->client_inventory[i].amount / 256 / 256 / 256 % 256;

        packet[j+6]=i; //inventory pos (starts at 0)
        packet[j+7]=0; //flags
    }

/*
    packet[0]=HERE_YOUR_INVENTORY;

    packet[1]=18; //packet count -1
    packet[2]=0;

    packet[3]=2;

    packet[4]=28;
    packet[5]=0;
    packet[6]=10;
    packet[7]=0;
    packet[8]=0;
    packet[9]=0;
    packet[10]=0;
    packet[11]=0;

    packet[12]=28;
    packet[13]=0;
    packet[14]=10;
    packet[15]=0;
    packet[16]=0;
    packet[17]=0;
    packet[18]=1;
    packet[19]=0;

    send(connection, packet, 20, 0);//packet count+1
*/

    send(connection, packet, (MAX_INVENTORY_SLOTS*8)+4, 0);
}

int get_used_inventory_slot(int connection, int image_id){

    int i;

    for(i=0; i<MAX_INVENTORY_SLOTS; i++){

        if(clients.client[connection]->client_inventory[i].image_id==image_id) return i;
    }

    return NOT_FOUND;
}

int get_unused_inventory_slot(int connection){

    int i;

    //search for slot with no image id
    for(i=0; i<MAX_INVENTORY_SLOTS; i++){
        if(clients.client[connection]->client_inventory[i].amount==0) return i;
    }

    return NOT_FOUND;
}

int get_char_carry_capacity(int connection){

    int race_id=clients.client[connection]->char_type;
    int initial_carry_capacity=race[race_id].initial_carry_capacity;
    int carry_capacity_multiplier=race[race_id].carry_capacity_multiplier;

    return initial_carry_capacity + (carry_capacity_multiplier * clients.client[connection]->physique);
}

int get_inventory_emu(int connection){

    int i=0;
    int total_emu=0;
    int image_id=0;

    for(i=0; i<MAX_INVENTORY_SLOTS; i++){

        image_id=clients.client[connection]->client_inventory[i].image_id;
        total_emu +=(clients.client[connection]->client_inventory[i].amount * item[image_id].emu);
    }

    return total_emu;

}

void send_get_new_bag(int connection, int bag_id){

    unsigned char packet[11];

    int map_id=clients.client[connection]->map_id;
    int map_axis=maps.map[map_id]->map_axis;
    int x_pos=clients.client[connection]->map_tile % map_axis;
    int y_pos=clients.client[connection]->map_tile / map_axis;

    packet[0]=GET_NEW_BAG;

    packet[1]=6;
    packet[2]=0;

    packet[3]=x_pos % 256;
    packet[4]=x_pos / 256;

    packet[5]=y_pos % 256;
    packet[6]=y_pos / 256;

    packet[7]=bag_id; //bag list number

    send(connection, packet, 8, 0);
}

void send_destroy_bag(int connection, int bag_id){

    unsigned char packet[11];

    packet[0]=DESTROY_BAG;

    packet[1]=3;
    packet[2]=0;

    packet[3]=bag_id % 256;
    packet[4]=bag_id / 256;

    send(connection, packet, 5, 0);
}