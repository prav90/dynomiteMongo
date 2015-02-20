/*
 * Ioannis Papapanagiotou - A new layer of MongoDB on top of Dynomite/twemproxy
 * Copyright (C) 2015
 */

/*
 * Dynomite - A thin, distributed replication layer for multi non-distributed storages.
 * Copyright (C) 2014 Netflix, Inc.
 */

/*
 * twemproxy - A fast and lightweight proxy for memcached protocol.
 * Copyright (C) 2011 Twitter, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <ctype.h>

#include "../dyn_core.h"
#include "dyn_proto.h"
#include "dyn_mongo.h"

/*
 * Yannis
 *
 * From MongoDB wire protocol specification
 *
 */

void
mongo_parse_req(struct msg *r)
{
    struct mbuf *b;
    uint8_t *p, *m;
    uint8_t ch;
    enum {
        SW_START,
        SW_SENTINEL
    }state;

    MsgHeader hdr;


    state = r->state;
    b = STAILQ_LAST(&r->mhdr, mbuf, next);

    ASSERT(r->request);
    ASSERT(r->data_store==2);
    ASSERT(state >= SW_START && state < SW_SENTINEL);
    ASSERT(b != NULL);
    ASSERT(b->pos <= b->last);

    /* validate the parsing maker */
    ASSERT(r->pos != NULL);
    ASSERT(r->pos >= b->pos && r->pos <= b->last);

    for (p = r->pos; p < b->last; p++) {
        ch = *p;
        hdr.messageLength = nthol(ch);
        ch += 4;
        hdr.requestID = ntohl(ch);
        ch += 4;
        hdr.responseTo = ntohl(ch);
        ch +=4;
        hdr.opCode = ntohl(ch);
        switch(hdr.opCode) {
        	case OP_QUERY:
        		r->type = MSG_REQ_MONGO_OP_QUERY;
        		break;
        	case OP_INSERT:
        		r->type= MSG_REQ_MONGO_OP_INSERT
        		break;
        	default:
        		/* ERROR */
        		break;

        }





        switch (state) {
        	case SW_START:

            case SW_SENTINEL:
            default:
                NOT_REACHED();
                break;
         }
    }

}

void
mongo_parse_rsp(struct msg *r)
{
    struct mbuf *b;
    uint8_t *p, *m;
    uint8_t ch;

}

/*
 * Return true, if the mongo command is a delete command, otherwise
 * return false
 */
static bool
mongo_delete(struct msg *r)
{
    if (r->type == MSG_REQ_MONGO_DELETE) {
        return true;
    }

    return false;
}

/*
 * Pre-split copy handler invoked when the request is a multi vector -
 * 'get' or 'gets' request and is about to be split into two requests
 */
void
mongo_pre_splitcopy(struct mbuf *mbuf, void *arg)
{



}


/*
 * Post-split copy handler invoked when the request is a multi vector -
 * GETMORE request and has already been split into two requests
 */
rstatus_t
mongo_post_splitcopy(struct msg *r)
{
	/* Yannis: this is not complete - this an exemplar implementation */
    struct mbuf *mbuf;
    struct string crlf = string(CRLF);

    ASSERT(r->request);
    ASSERT(r->data_store==1);
    ASSERT(!STAILQ_EMPTY(&r->mhdr));

    mbuf = STAILQ_LAST(&r->mhdr, mbuf, next);
    mbuf_copy(mbuf, crlf.data, crlf.len);

    return DN_OK;
}

/*
 * Pre-coalesce handler is invoked when the message is a response to
 * the fragmented multi vector request - 'get' or 'gets' and all the
 * responses to the fragmented request vector hasn't been received
 */
void
mongo_pre_coalesce(struct msg *r)
{
	/* Yannis : this needs development */

}


/*
 * Post-coalesce handler is invoked when the message is a response to
 * the fragmented multi vector request - 'mget' or 'del' and all the
 * responses to the fragmented request vector has been received and
 * the fragmented request is consider to be done
 */
void
mongo_post_coalesce(struct msg *r)
{
	/* Yannis : this needs development */


}
}
