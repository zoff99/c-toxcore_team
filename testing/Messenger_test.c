/* Messenger test
 *
 * This program adds a friend and accepts all friend requests with the proper message.
 *
 * It tries sending a message to the added friend.
 *
 * If it receives a message from a friend it replies back.
 *
 *
 * This is how I compile it: gcc -O2 -Wall -D VANILLA_NACL -o test ../core/Lossless_UDP.c ../core/network.c ../core/net_crypto.c ../core/Messenger.c ../core/DHT.c ../nacl/build/${HOSTNAME%.*}/lib/amd64/{cpucycles.o,libnacl.a,randombytes.o} Messenger_test.c
 *
 *
 * Command line arguments are the ip, port and public_key of a node (for bootstrapping).
 *
 * EX: ./test 127.0.0.1 33445 CDCFD319CE3460824B33BE58FD86B8941C9585181D8FBD7C79C5721D7C2E9F7C
 *
 * Or the argument can be the path to the save file.
 *
 * EX: ./test Save.bak
 */

/*
 * Copyright © 2016-2017 The TokTok team.
 * Copyright © 2013 Tox project.
 *
 * This file is part of Tox, the free peer to peer instant messenger.
 *
 * Tox is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Tox is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Tox.  If not, see <http://www.gnu.org/licenses/>.
 */
#define _XOPEN_SOURCE 600

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../toxcore/Messenger.h"
#include "misc_tools.c"

#if !defined(_WIN32) && !defined(__WIN32__) && !defined (WIN32)
#include <arpa/inet.h>

#endif

static void print_message(Messenger *m, uint32_t friendnumber, unsigned int type, const uint8_t *string, size_t length,
                          void *userdata)
{
    printf("Message with length %zu received from %u: %s \n", length, friendnumber, string);
    m_send_message_generic(m, friendnumber, type, (const uint8_t *)"Test1", 6, 0);
}

/* TODO(irungentoo): needed as print_request has to match the interface expected by
 * networking_requesthandler and so cannot take a Messenger * */
static Messenger *m;

static void print_request(Messenger *m2, const uint8_t *public_key, const uint8_t *data, size_t length, void *userdata)
{
    printf("Friend request received from: \n");
    printf("ClientID: ");
    uint32_t j;

    for (j = 0; j < 32; j++) {
        if (public_key[j] < 16) {
            printf("0");
        }

        printf("%hhX", public_key[j]);
    }

    printf("\nOf length: %zu with data: %s \n", length, data);

    if (length != sizeof("Install Gentoo")) {
        return;
    }

    if (memcmp(data , "Install Gentoo", sizeof("Install Gentoo")) == 0)
        //if the request contained the message of peace the person is obviously a friend so we add him.
    {
        printf("Friend request accepted.\n");
        m_addfriend_norequest(m2, public_key);
    }
}

int main(int argc, char *argv[])
{
    /* let user override default by cmdline */
    uint8_t ipv6enabled = TOX_ENABLE_IPV6_DEFAULT; /* x */
    int argvoffset = cmdline_parsefor_ipv46(argc, argv, &ipv6enabled);

    if (argvoffset < 0) {
        exit(1);
    }

    /* with optional --ipvx, now it can be 1-4 arguments... */
    if ((argc != argvoffset + 2) && (argc != argvoffset + 4)) {
        printf("Usage: %s [--ipv4|--ipv6] ip port public_key (of the DHT bootstrap node)\n", argv[0]);
        printf("or\n");
        printf("       %s [--ipv4|--ipv6] Save.bak (to read Save.bak as state file)\n", argv[0]);
        exit(0);
    }

    Messenger_Options options = {0};
    options.ipv6enabled = ipv6enabled;
    m = new_messenger(&options, 0);

    if (!m) {
        fputs("Failed to allocate messenger datastructure\n", stderr);
        exit(0);
    }

    if (argc == argvoffset + 4) {
        uint16_t port = net_htons(atoi(argv[argvoffset + 2]));
        uint8_t *bootstrap_key = hex_string_to_bin(argv[argvoffset + 3]);
        int res = DHT_bootstrap_from_address(m->dht, argv[argvoffset + 1],
                                             ipv6enabled, port, bootstrap_key);
        free(bootstrap_key);

        if (!res) {
            printf("Failed to convert \"%s\" into an IP address. Exiting...\n", argv[argvoffset + 1]);
            exit(1);
        }
    } else {
        FILE *file = fopen(argv[argvoffset + 1], "rb");

        if (file == NULL) {
            printf("Failed to open \"%s\" - does it exist?\n", argv[argvoffset + 1]);
            return 1;
        }

        int read;
        uint8_t buffer[128000];
        read = fread(buffer, 1, 128000, file);
        printf("Messenger loaded: %i\n", messenger_load(m, buffer, read));
        fclose(file);
    }

    m_callback_friendrequest(m, print_request);
    m_callback_friendmessage(m, print_message);

    printf("OUR ID: ");
    uint32_t i;
    uint8_t address[FRIEND_ADDRESS_SIZE];
    getaddress(m, address);

    for (i = 0; i < FRIEND_ADDRESS_SIZE; i++) {
        if (address[i] < 16) {
            printf("0");
        }

        printf("%hhX", address[i]);
    }

    setname(m, (const uint8_t *)"Anon", 5);

    char temp_hex_id[128];
    printf("\nEnter the address of the friend you wish to add (38 bytes HEX format):\n");

    if (!fgets(temp_hex_id, sizeof(temp_hex_id), stdin)) {
        exit(0);
    }

    if ((strlen(temp_hex_id) > 0) && (temp_hex_id[strlen(temp_hex_id) - 1] == '\n')) {
        temp_hex_id[strlen(temp_hex_id) - 1] = '\0';
    }

    uint8_t *bin_id = hex_string_to_bin(temp_hex_id);
    int num = m_addfriend(m, bin_id, (const uint8_t *)"Install Gentoo", sizeof("Install Gentoo"));
    free(bin_id);

    perror("Initialization");

    while (1) {
        uint8_t name[128];
        getname(m, num, name);
        printf("%s\n", name);

        m_send_message_generic(m, num, MESSAGE_NORMAL, (const uint8_t *)"Test", 5, 0);
        do_messenger(m, NULL);
        c_sleep(30);
        FILE *file = fopen("Save.bak", "wb");

        if (file == NULL) {
            return 1;
        }

        uint8_t *buffer = (uint8_t *)malloc(messenger_size(m));
        messenger_save(m, buffer);
        size_t write_result = fwrite(buffer, 1, messenger_size(m), file);

        if (write_result < messenger_size(m)) {
            return 1;
        }

        free(buffer);
        fclose(file);
    }

    kill_messenger(m);
}
