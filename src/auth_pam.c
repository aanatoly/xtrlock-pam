
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <security/pam_appl.h>

# define PAM_ERROR_PRINT(pamfunc, pamh) \
    fprintf(stderr, "%s failure: %s\n", pamfunc, pam_strerror(pamh, pam_error))


#define PAM_RUN(function, args)         \
        do {                        \
        pam_error = function args;          \
        if (pam_error != PAM_SUCCESS) {         \
            PAM_ERROR_PRINT(#function, pamh);     \
            goto pam_done;              \
        }                       \
        } while (0)

//#define DBG printf
#define DBG(...)

static char *luser, *lpassword;

static int
conv(int num_msg, const struct pam_message **msg,
    struct pam_response **resp, void *appdata_ptr)
{
    int i;

    int status = PAM_SUCCESS;
    struct pam_response *r;

    r = calloc(num_msg, sizeof (struct pam_response));
    if (r == NULL)
        return (PAM_BUF_ERR);


    for (i = 0; i < num_msg; i++) {
        r[i].resp_retcode = 0;
        r[i].resp = NULL;

        DBG("msg[%d]=%s\n", i, msg[i]->msg);
        switch (msg[i]->msg_style) {
        case PAM_ERROR_MSG:
            DBG("PAM_ERROR_MSG\n");
            break;

        case PAM_TEXT_INFO:
            DBG("PAM_TEXT_INFO\n");
            break;

        case PAM_PROMPT_ECHO_ON:
            DBG("PAM_PROMPT_ECHO_ON\n");
            break;

        case PAM_PROMPT_ECHO_OFF:
            DBG("PAM_PROMPT_ECHO_OFF\n");
            r[i].resp = strdup(lpassword);
            break;

        default:
            DBG("Unknown PAM msg_style: %d\n", msg[i]->msg_style);
        }
    }
#if 0
pam_error:
    if (status != PAM_SUCCESS) {

        for (i = 0; i < num_msg; i++) {
            if (r[i].resp) {
                bzero(r[i].resp, strlen(r[i].resp));
                free(r[i].resp);
            }
        }
        free(r);
        r = NULL;

    }
#endif
    *resp = r;
    return status;

}


int
auth_pam(char *user, char *password, char *module)
{
    int pam_error = 0;
    struct pam_conv pc = { conv, NULL };
    pam_handle_t *pamh;


    luser = user;
    lpassword = password;
    pc.conv = conv;
    PAM_RUN(pam_start, (module, NULL, &pc, &pamh));
    PAM_RUN(pam_set_item, (pamh, PAM_USER, user));
    PAM_RUN(pam_set_item, (pamh, PAM_TTY, ":0"));
    PAM_RUN(pam_authenticate, (pamh, PAM_DISALLOW_NULL_AUTHTOK));
    //PAM_RUN(pam_open_session, (*pamh, 0));
    //PAM_RUN(pam_setcred,(pamh, PAM_ESTABLISH_CRED));
    //env_add_array(pam_getenvlist(pamh), 1);

pam_done:
    pam_end(pamh, pam_error);
    return pam_error == PAM_SUCCESS;
}

