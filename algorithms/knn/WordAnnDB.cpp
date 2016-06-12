#include "WordAnnDB.h"
#include <fstream>
#include <glog/logging.h>


namespace KNN {

uint32_t WordAnnDB::s_nIdIndex = 0;

std::pair<uint32_t, bool> WordAnnDB::addRecord( const std::string &line )
{
    using namespace std;

    StringPtr       pWord = std::make_shared<string>();
    stringstream    str(line);

    str >> *pWord;
    if (!str)
        THROW_RUNTIME_ERROR("Incorrect format for line: " << line );

    vector<float> values;
    read_into_container( str, values );

    if ( !(str.eof() || str.good()) )
        THROW_RUNTIME_ERROR("Read stream error!");

    if ( values.size() != m_nFields )
        THROW_RUNTIME_ERROR("N_Fields not match");

    // insert to tables
    uint32_t                  id = s_nIdIndex;
    std::pair<uint32_t, bool> ret = insert2WordIdTable( pWord, id );
    if (ret.second) {
        insert2IdWordTable( id, pWord );
        // create Annoy Index
        m_AnnIndex.add_item( id, &values[0] );
        ++s_nIdIndex;
    } // if

    LOG_IF(WARNING, !ret.second) << "add record fail: " << line;

    return ret;
}

void WordAnnDB::saveWordTable( const char *filename )
{
    using namespace std;

    ofstream ofs( filename, ios::out );

    if (!ofs)
        THROW_RUNTIME_ERROR("WordAnnDB::saveWordTable() cannot open " << filename << " for writting!");

    // format: word    id
    for (uint32_t i = 0; i < ID_WORD_HASH_SIZE; ++i) {
        const IdWordTable &table = m_mapId2Word[i];
        for (const auto &v : table)
            ofs << *(v.second) << "\t" << v.first << endl;
    } // for
}

void WordAnnDB::loadWordTable( const char *filename )
{
    using namespace std;

    ifstream ifs( filename, ios::in );

    if (!ifs)
        THROW_RUNTIME_ERROR("WordAnnDB::saveWordTable() cannot open " << filename << " for reading!");

    string line, word;
    uint32_t id, maxId = 0;
    while (getline(ifs, line)) {
        stringstream str(line);
        str >> word >> id;
        addWord( word, id );
        if (id > maxId)
            maxId = id;
    } // while

    s_nIdIndex = maxId + 1;
}

} // namespace KNN
