 /*
 * smtp_auth.c
 *
 * SMTP authentication support.
 * Provides a simple authentication layer that can be
 * called by the main SMTP server.
 */

#include <stdio.h>
#include <string.h>
#include "smtp_auth.h"

#define MAX_USERNAME 128
#define MAX_PASSWORD 128

typedef struct {
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
} smtp_user_t;

/* Example account.
 * In a real server, credentials should NOT be hard-coded.
 */
static const smtp_user_t users[] = {
    {
        "admin",
        "change-this-password"
    },
    {
        "mailer",
        "change-this-password-too"
    }
};

#define USER_COUNT (sizeof(users) / sizeof(users[0]))

int smtp_authenticate(
    const char *username,
    const char *password
)
{
    if (username == NULL || password == NULL)
        return 0;

    for (size_t i = 0; i < USER_COUNT; i++) {

        if (strcmp(username, users[i].username) == 0 &&
            strcmp(password, users[i].password) == 0) {

            printf(
                "[AUTH] User '%s' authenticated successfully\n",
                username
            );

            return 1;
        }
    }

    printf(
        "[AUTH] Authentication failed for '%s'\n",
        username
    );

    return 0;
}
