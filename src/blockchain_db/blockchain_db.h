typedef struct BlockchainDB {
    bool m_open;  //!< Whether or not the BlockchainDB is open/ready for use
    //   mutable epee::critical_section m_synchronization_lock;  //!< A lock, currently for when BlockchainLMDB needs to resize the backing db file
} BlockchainDB;

#define DBF_SAFE       1
#define DBF_FAST       2
#define DBF_FASTEST    4
#define DBF_RDONLY     8
#define DBF_SALVAGE 0x10
