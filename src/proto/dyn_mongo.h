/*
 * Ioannis Papapanagiotou -
 * Copyright (C) 2015
 */

struct mongo_header{
    uint32_t messageLength; // total message size, including this
    uint32_t requestID;     // identifier for this message
    uint32_t responseTo;    // requestID from the original request
    uint32_t opCode;        // request type - see table below
};
