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

    struct MsgHeader hdr;


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

    /* Every one of the fields in the MongoDB header is 4Bytes.
     */
    p = r->pos;
    m = p;

    memcpy(&hdr.messageLength, p, 4);
    hdr.messageLength = ntohl(hdr.messageLength);
    p += 4;
    memcpy(&hdr.requestID, p, 4);
    hdr.requestID = ntohl(hdr.requestID);
    p += 4;
    memcpy(&hdr.responseTo, p, 4);
    hdr.responseTo = ntohl(hdr.responseTo);
    p += 4;
    memcpy(&hdr.opCode, p, 4);
    hdr.opCode = ntohl(hdr.opCode);

    /* Default the type of the message to be unknown */
    r->type = MSG_UNKNOWN;

        switch(hdr.opCode) {
        	case OP_QUERY:
        		r->type = MSG_REQ_MONGO_OP_QUERY;
                r->is_read = 1;
        		break;
        	case OP_UPDATE:
        		r->type = MSG_REQ_MONGO_OP_UPDATE;
                r->is_read = 0;
        		break;
        	case OP_GET_MORE:
        		r->type = MSG_REQ_MONGO_OP_GET_MORE;
                r->is_read = 1;
        		break;
        	case OP_DELETE:
        		r->type = MSG_REQ_MONGO_OP_DELETE;
        		break;
        	case OP_MSG:
        	    r->type = MSG_REQ_MONGO_OP_MSG;
        	    break;
        	case OP_RESERVED:
        		r->type = MSG_REQ_MONGO_RESERVED;
        		break;
        	case OP_KILL_CURSORS:
        		r->type = MSG_REQ_MONGO_KILL_CURSORS;
        		break;
        	case OP_INSERT:
        		r->type= MSG_REQ_MONGO_OP_INSERT;
        		break;
        	default:
        		/* ERROR */
        		break;
        }

        switch (r->type) {
        	case MSG_REQ_MONGO_OP_QUERY:
        	case MSG_REQ_MONGO_OP_UPDATE:
        	case MSG_REQ_MONGO_OP_GET_MORE:
        	case MSG_REQ_MONGO_OP_DELETE:
        	case MSG_REQ_MONGO_OP_MSG:
        	case MSG_REQ_MONGO_RESERVED:
        	case MSG_REQ_MONGO_KILL_CURSORS:
        	case MSG_REQ_MONGO_OP_INSERT:
        	case MSG_UNKNOWN:
        		goto error;

            default:
                NOT_REACHED();
        }

        switch (state) {
        	case SW_START:

            case SW_SENTINEL:
            default:
                NOT_REACHED();
                break;
         }

  r->pos = p;
  r->state = state;
  /* Move b to the end of the message by adding the beginning plus the message length */
  b->last = m + hdr.messageLength;

  if (b->last == b->end && r->token != NULL) {
              r->pos = r->token;
              r->token = NULL;
              r->result = MSG_PARSE_REPAIR;
  } else {
              r->result = MSG_PARSE_AGAIN;
  }

          log_hexdump(LOG_VERB, b->pos, mbuf_length(b), "parsed req %"PRIu64" res %d "
                      "type %d state %d rpos %d of %d", r->id, r->result, r->type,
                      r->state, r->pos - b->pos, b->last - b->pos);
          return;

fragment:
          ASSERT(p != b->last);
          ASSERT(r->token != NULL);
          r->pos = r->token;
          r->token = NULL;
          r->state = state;
          r->result = MSG_PARSE_FRAGMENT;

          log_hexdump(LOG_VERB, b->pos, mbuf_length(b), "parsed req %"PRIu64" res %d "
                      "type %d state %d rpos %d of %d", r->id, r->result, r->type,
                      r->state, r->pos - b->pos, b->last - b->pos);
          return;

done:
          ASSERT(r->type > MSG_UNKNOWN && r->type < MSG_SENTINEL);
          r->pos = p + 1;
          ASSERT(r->pos <= b->last);
          r->state = SW_START;
          r->result = MSG_PARSE_OK;

          log_hexdump(LOG_VERB, b->pos, mbuf_length(b), "parsed req %"PRIu64" res %d "
                      "type %d state %d rpos %d of %d", r->id, r->result, r->type,
                      r->state, r->pos - b->pos, b->last - b->pos);
          return;

error:
          r->result = MSG_PARSE_ERROR;
          r->state = state;
          errno = EINVAL;

          log_hexdump(LOG_INFO, b->pos, mbuf_length(b), "parsed bad req %"PRIu64" "
                      "res %d type %d state %d Mongo header length %d and OP CODE: %d", r->id, r->result, r->type,
                      r->state, hdr.messageLength, hdr.opCode);


}

void
mongo_parse_rsp(struct msg *r)
{
    struct mbuf *b;
    uint8_t *p, *m;
    uint8_t ch;
    ASSERT(!r->request);
    ASSERT(r->data_store==1);
    struct MsgHeader hdr;

    enum {
            SW_START,
        } state;

   state = r->state;
   b = STAILQ_LAST(&r->mhdr, mbuf, next);

    ASSERT(b != NULL);
    ASSERT(b->pos <= b->last);

    /* validate the parsing marker */
    ASSERT(r->pos != NULL);
    ASSERT(r->pos >= b->pos && r->pos <= b->last);

    /* Every one of the fields in the MongoDB header is 4Bytes.
     */
    p = r->pos;
    m = p;

    memcpy(&hdr.messageLength, p, 4);
    hdr.messageLength = ntohl(hdr.messageLength);
    p += 4;
    memcpy(&hdr.requestID, p, 4);
    hdr.requestID = ntohl(hdr.requestID);
    p += 4;
    memcpy(&hdr.responseTo, p, 4);
    hdr.responseTo = ntohl(hdr.responseTo);
    p += 4;
    memcpy(&hdr.opCode, p, 4);
    hdr.opCode = ntohl(hdr.opCode);

        /* Default the type of the message to be unknown */
        r->type = MSG_UNKNOWN;
        switch(hdr.opCode) {
        	case OP_REPLY:
                r->type= MSG_RSP_MONGO_OP_REPLY;
                break;
            default:
            /* ERROR */
            	break;
        }

        switch (r->type) {
        	case MSG_RSP_MONGO_OP_REPLY:
        	case MSG_UNKNOWN:
        		goto error;

            default:
                NOT_REACHED();
        }

    r->pos = p;
    r->state = state;
    /* Move b to the end of the message by adding the beginning plus the message length */
    b->last = m + hdr.messageLength;


     if (b->last == b->end && r->token != NULL) {
         r->pos = r->token;
         r->token = NULL;
         r->result = MSG_PARSE_REPAIR;
     } else {
         r->result = MSG_PARSE_AGAIN;
     }

     log_hexdump(LOG_VERB, b->pos, mbuf_length(b), "parsed rsp %"PRIu64" res %d "
                 "type %d state %d rpos %d of %d", r->id, r->result, r->type,
                 r->state, r->pos - b->pos, b->last - b->pos);
     return;

 done:
     ASSERT(r->type > MSG_UNKNOWN && r->type < MSG_SENTINEL);
     r->pos = p + 1;
     ASSERT(r->pos <= b->last);
     r->state = SW_START;
     r->token = NULL;
     r->result = MSG_PARSE_OK;

     log_hexdump(LOG_VERB, b->pos, mbuf_length(b), "parsed rsp %"PRIu64" res %d "
                 "type %d state %d rpos %d of %d", r->id, r->result, r->type,
                 r->state, r->pos - b->pos, b->last - b->pos);
     return;

 error:
     r->result = MSG_PARSE_ERROR;
     r->state = state;
     errno = EINVAL;

     log_hexdump(LOG_INFO, b->pos, mbuf_length(b), "parsed bad req %"PRIu64" "
                 "res %d type %d state %d Mongo header length %d and OP CODE: %d", r->id, r->result, r->type,
                 r->state, hdr.messageLength, hdr.opCode);


}

/*
 * Return true, if the mongo command is a delete command, otherwise
 * return false
 */
static bool
mongo_delete(struct msg *r)
{
    if (r->type == MSG_REQ_MONGO_OP_DELETE) {
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

