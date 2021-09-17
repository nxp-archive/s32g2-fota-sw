/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:  BSD-3-Clause
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <getopt.h>
#include <errno.h>
#include <ctype.h>
#include <dlfcn.h>
#include <libgen.h>
#include <limits.h>
#include <inttypes.h>
#include <kcapi.h>
#include "security_common.h"
#include "security_hasher.h"

#define ERR_OK ((int16_t)(0x0001))
#define ERR_NOK ((int16_t)(0xFFFF))
// enum KCAPI_INIT_FLAG 
// {
//   initialized,
//   not_initialized
// };
// enum KCAPI_INIT_FLAG kcapi_init_flag = not_initialized;
// struct kcapi_handle *handle;


int16_t hasher_kcapi(struct kcapi_handle *handle, struct hash_params *params)
{
    uint8_t hashlen = params->hashlen;
    uint8_t *msg_ = params->msg;
    uint8_t md[128] = {0};
    int32_t left = params->msglen;
    int32_t kcapi_ret;

    usleep(50*1000);
    
    while (left > 0)
    {
        uint32_t todo = (left > INT_MAX) ? INT_MAX : left;
        if (kcapi_md_update(handle, msg_, todo) < 0)
        {
            fprintf(stderr,"kcapi_md_update failed!\n");
            return ERR_NOK;
        };
        left -= todo;
    };
    kcapi_ret = kcapi_md_final(handle, md, sizeof(md));
    if (kcapi_ret > 0)
    {
        if (hashlen > (uint32_t)kcapi_ret)
        {
            fprintf(stderr, "Invalid truncated hash size: %lu > %i\n",
                    (unsigned long)hashlen, kcapi_ret);
            return ERR_NOK;
        }
        else
        {
            memcpy(params->hashvalue, md, params->hashlen);
        }
    }
    else
    {
        fprintf(stderr, "kcapi_md_final failed\n");
        return ERR_NOK;
    }
    
     return ERR_OK;
}
int16_t hasher(const uint8_t *msg, int32_t msglen, uint8_t *hashvalue, int8_t *hashvaluelen, enum hashtype hashtype_)
{
    int16_t ret;
    int kcapi_ret;
    struct kcapi_handle *handle;
    struct hash_params _params;
    uint8_t md_buff[128];
    if (msg == NULL || msglen <= 0)
    {
        fprintf(stderr, "msg is not correct!\n");
        return ERR_NOK;
    }
    else
    {
        switch (hashtype_)
        {
        case SHA128_KCAPI:
            /* code */
            break;
        case SHA256_KCAPI:
        {
            *hashvaluelen = 32;
            _params.name = NAMES_SHA256[0];
            _params.hashlen = 32;
            _params.hashvalue = md_buff; //malloc(_params.hashlen);
            _params.msg = msg;
            _params.msglen = msglen;
            const char *hashname = _params.name.kcapiname;
            // if(kcapi_init_flag == not_initialized)
           // {
              kcapi_ret = kcapi_md_init(&handle, hashname, 0);
              if (kcapi_ret)
              {
                fprintf(stderr, "Allocation of %s cipher failed (ret=%d)\n",
                        hashname, kcapi_ret);
                goto cleanup;
              }
            //   else
            //   {
            //     kcapi_init_flag = initialized;
            //   }
           // }
            ret = hasher_kcapi(handle, &_params);
            if (ret != ERR_OK)
            {
                fprintf(stderr, "Hasher failed\n");
                goto cleanup;
            }
            memcpy(hashvalue, _params.hashvalue, *hashvaluelen);
        }
        break;
        default:
        {
            fprintf(stderr, "Please input correct hash function like SHA256\n");
            ret = ERR_NOK;
            goto cleanup;
        }
        }
    }
    kcapi_md_destroy(handle);
    return ret;
cleanup:
    kcapi_md_destroy(handle);
    ret = ERR_NOK;
   // kcapi_init_flag = not_initialized; 
    return ret;
}

