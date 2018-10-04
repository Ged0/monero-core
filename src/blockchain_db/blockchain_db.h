typedef struct BlockchainDB {
    bool m_open;  //!< Whether or not the BlockchainDB is open/ready for use
    //   mutable epee::critical_section m_synchronization_lock;  //!< A lock, currently for when BlockchainLMDB needs to resize the backing db file
} BlockchainDB;
