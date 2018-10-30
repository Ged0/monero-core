#ifndef MONERO_RINGCT_RCTTYPES_H_
#define MONERO_RINGCT_RCTTYPES_H_

//Define this flag when debugging to get additional info on the console
#ifdef DBG
#define DP(x) dp(x)
#else
#define DP(x)
#endif

//atomic units of moneros
#define ATOMS 64

typedef struct key {
    unsigned char bytes[32];
} key;

typedef struct keyV {
    key* keys;
    size_t keysSize;
} keyV;

typedef struct keyM {
    keyV* keys;
    size_t keysSize;
} keyM;

typedef struct ctkey {
    key dest;
    key mask; //C here if public
} ctkey;

typedef struct ctkeyV {
    ctkey* keys;
    size_t keysSize;
} ctkeyV;

typedef struct ctkeyM {
    ctkeyV* keys;
    size_t keysSize;
} ctkeyM;

//used for multisig data
typedef struct multisig_kLRki {
    key k;
    key L;
    key R;
    key ki;
} multisig_kLRki;

typedef struct multisig_out {
    keyV* keys;
    size_t keysSize;
} multisig_out;

//data for passing the amount to the receiver secretly
// If the pedersen commitment to an amount is C = aG + bH,
// "mask" contains a 32 byte key a
// "amount" contains a hex representation (in 32 bytes) of a 64 bit number
// "senderPk" is not the senders actual public key, but a one-time public key generated for
// the purpose of the ECDH exchange
typedef struct ecdhTuple {
    key mask;
    key amount;
    key senderPk;
} ecdhTuple;

//containers for representing amounts
typedef uint64_t xmr_amount;
typedef unsigned int bits[ATOMS];
typedef key key64[64];

typedef struct boroSig {
    key64 s0;
    key64 s1;
    key ee;
} boroSig;

typedef struct rctSigBase {
    uint8_t type;
    key message;
    ctkeyM mixRing; //the set of all pubkeys / copy
    //pairs that you mix with
    keyV pseudoOuts; //C - for simple rct
    ecdhTuple* ecdhInfo;
    size_t ecdhInfoSize;
    ctkeyV outPk;
    xmr_amount txnFee; // contains b
    
} rctSigBase;



#endif //MONERO_RINGCT_RCTTYPES_H_
