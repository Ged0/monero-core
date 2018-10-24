#ifndef MONERO_CRYPTO_CRYPTO_H_
#define MONERO_CRYPTO_CRYPTO_H_

#include <glib.h>

//#pragma pack(push, 1)
  typedef struct ec_point {
    char data[32];
  } ec_point;

  typedef struct ec_scalar {
    char data[32];
  } ec_scalar;

typedef ec_point public_key;

//  using secret_key = epee::mlocked<tools::scrubbed<ec_scalar>>;

  typedef struct public_keyV {
    //public_key
    GArray* keys;
    int rows;
  } public_keyV;

//  typedef struct secret_keyV {
//    std::vector<secret_key> keys;
//    int rows;
//  } secret_keyV;

//  POD_CLASS public_keyM {
//    int cols;
//    int rows;
//    std::vector<secret_keyV> column_vectors;
//  };

typedef ec_point key_derivation;
//  POD_CLASS key_derivation: ec_point {
//    friend class crypto_ops;
//  };

typedef ec_point key_image;
//  POD_CLASS key_image: ec_point {
//    friend class crypto_ops;
//  };

   typedef struct signature {
    ec_scalar c, r;
  } signature;
//#pragma pack(pop)


#endif //MONERO_CRYPTO_CRYPTO_H_
