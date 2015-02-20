/*
 * Ioannis Papapanagiotou - MongoHeader
 * Copyright (C) 2015
 */
#include <arpa/inet.h>

#define OP_REPLY 1
#define OP_MSG 1000
#define OP_UPDATE 2001
#define OP_INSERT 2002
#define RESERVED 2003
#define OP_QUERY 2004
#define OP_GET_MORE 2005
#define OP_DELETE 2006
#define OP_KILL_CURSORS 2007

struct MsgHeader {
    uint32_t messageLength; // total message size, including this
    uint32_t requestID;     // identifier for this message
    uint32_t responseTo;    // requestID from the original request
    uint32_t opCode;        // request type - see table below
};

struct op_update {
    MsgHeader header;             // standard message header
    uint32_t     ZERO;               // 0 - reserved for future use
    cstring   fullCollectionName; // "dbname.collectionname"
    int32     flags;              // bit vector. see below
    document  selector;           // the query to select the document
//    document  update;             // specification of the update to perform
};

struct op_insert {
    MsgHeader header;             // standard message header
    uint32_t     flags;              // bit vector - see below
    cstring   fullCollectionName; // "dbname.collectionname"
//    document* documents;          // one or more documents to insert into the collection
};

struct op_query {
    MsgHeader header;                 // standard message header
    uint32_t     flags;                  // bit vector of query options.  See below for details.
    cstring   fullCollectionName ;    // "dbname.collectionname"
    uint32_t     numberToSkip;           // number of documents to skip
    uint32_t     numberToReturn;         // number of documents to return
                                      //  in the first OP_REPLY batch
//    document  query;                  // query object.  See below for details.
//  [ document  returnFieldsSelector; ] // Optional. Selector indicating the fields
                                      //  to return.  See below for details.
};

struct op_get_more {
    MsgHeader header;             // standard message header
    uint32_t     ZERO;               // 0 - reserved for future use
    cstring   fullCollectionName; // "dbname.collectionname"
    uint32_t     numberToReturn;     // number of documents to return
    uint32_t     cursorID;           // cursorID from the OP_REPLY
};

struct op_delete {
    MsgHeader header;             // standard message header
    uint32_t     ZERO;               // 0 - reserved for future use
    cstring   fullCollectionName; // "dbname.collectionname"
    uint32_t     flags;              // bit vector - see below for details.
//    document  selector;           // query object.  See below for details.
};

struct op_kill_cursors {
    MsgHeader header;             // standard message header
    uint32_t     ZERO;               // 0 - reserved for future use
    cstring   fullCollectionName; // "dbname.collectionname"
    uint32_t     flags;              // bit vector - see below for details.
//    document  selector;           // query object.  See below for details.
};

struct op_reply {
    MsgHeader header;         // standard message header
    uint32_t     responseFlags;  // bit vector - see details below
    uint64_t     cursorID;       // cursor id if client needs to do get more's
    uint32_t     startingFrom;   // where in the cursor this reply is starting
    uint32_t     numberReturned; // number of documents in the reply
//    document* documents;      // documents
};


