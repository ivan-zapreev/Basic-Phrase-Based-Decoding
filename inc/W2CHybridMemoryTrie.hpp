/* 
 * File:   W2CHybridMemoryTrie.hpp
 * Author: Dr. Ivan S. Zapreev
 *
 * Visit my Linked-in profile:
 *      <https://nl.linkedin.com/in/zapreevis>
 * Visit my GitHub:
 *      <https://github.com/ivan-zapreev>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.#
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Created on August 21, 2015, 4:18 PM
 */

#ifndef W2CHYBRIDMEMORYTRIE_HPP
#define	W2CHYBRIDMEMORYTRIE_HPP

#include <string>   // std::string

#include "Globals.hpp"
#include "Logger.hpp"

#include "LayeredTrieBase.hpp"

#include "AWordIndex.hpp"
#include "W2CHybridMemoryTrieStorage.hpp"

using namespace std;
using namespace uva::smt::tries::dictionary;

namespace uva {
    namespace smt {
        namespace tries {

            /**
             * This is the hybrid memory trie implementation class. It has three template parameters.
             * @param N the maximum number of levelns in the trie.
             * @param StorageFactory the factory to create storage containers
             * @param StorageContainer the storage container type that is created by the factory
             */
            template<TModelLevel N, typename WordIndexType, template<TModelLevel > class StorageFactory, class StorageContainer>
            class W2CHybridTrie : public LayeredTrieBase<N, WordIndexType> {
            public:
                typedef LayeredTrieBase<N, WordIndexType> BASE;

                /**
                 * The basic constructor
                 * @param p_word_index the word index (dictionary) container
                 */
                explicit W2CHybridTrie(WordIndexType & word_index);

                /**
                 * @see LayeredTrieBase
                 */
                inline bool is_bitmap_hash_cache() {
                    return __W2CHybridTrie::DO_BITMAP_HASH_CACHE;
                }

                /**
                 * Computes the N-Gram context using the previous context and the current word id
                 * 
                 * WARNING: Must only be called for the M-gram level 1 < M <= N!
                 * @see LayeredTrieBase
                 * 
                 * @param wordId the current word id
                 * @param ctxId [in] - the previous context id, [out] - the next context id
                 * @param level the M-gram level we are working with M
                 * @return the resulting context
                 * @throw nothing
                 */
                bool get_ctx_id(const TShortId wordId, TLongId & ctxId, const TModelLevel level);
                
                /**
                 * Allows to log the information about the instantiated trie type
                 */
                virtual void log_trie_type_usage_info() {
                    LOG_USAGE << "Using the <" << __FILE__ << "> model." << END_LOG;
                }

                /**
                 * This method can be used to provide the N-gram count information
                 * That should allow for pre-allocation of the memory
                 * For more details @see ATrie
                 */
                virtual void pre_allocate(const size_t counts[N]);

                /**
                 * Allows to retrieve the data storage structure for the One gram with the given Id.
                 * If the storage structure does not exist, return a new one.
                 * For more details @see ATrie
                 */
                TProbBackOffEntry & make_1_gram_data_ref(const TShortId wordId);

                /**
                 * Allows to retrieve the data storage structure for the One gram with the given Id.
                 * For more details @see ATrie
                 */
                bool get_1_gram_data_ref(const TShortId wordId, const TProbBackOffEntry ** ppData);

                /**
                 * Allows to retrieve the data storage structure for the M gram
                 * with the given M-gram level Id. M-gram context and last word Id.
                 * If the storage structure does not exist, return a new one.
                 * For more details @see ATrie
                 */
                TProbBackOffEntry& make_m_gram_data_ref(const TModelLevel level, const TShortId wordId, const TLongId ctxId);

                /**
                 * Allows to retrieve the data storage structure for the M gram
                 * with the given M-gram level Id. M-gram context and last word Id.
                 * For more details @see ATrie
                 */
                bool get_m_gram_data_ref(const TModelLevel level, const TShortId wordId,
                        TLongId ctxId, const TProbBackOffEntry **ppData);

                /**
                 * Allows to retrieve the data storage structure for the N gram.
                 * Given the N-gram context and last word Id.
                 * For more details @see ATrie
                 */
                TLogProbBackOff& make_n_gram_data_ref(const TShortId wordId, const TLongId ctxId);

                /**
                 * Allows to retrieve the probability value for the N gram defined by the end wordId and ctxId.
                 * For more details @see ATrie
                 */
                bool get_n_gram_data_ref(const TShortId wordId, const TLongId ctxId, TLogProbBackOff & prob);
                
                /**
                 * The basic destructor
                 */
                virtual ~W2CHybridTrie();

            private:

                //Stores the number of words
                size_t m_word_arr_size;

                //The factory to produce the storage containers
                StorageFactory<N> * m_storage_factory;

                //M-Gram data for 1 <= M < N. This is a 2D array storing
                //For each M-Gram level M an array of prob-back_off values
                // m_mgram_data[M][0] - probability/back-off pair for the given M-gram
                // m_mgram_data[M][1] --//--
                // ...
                // m_mgram_data[M][#M-Grams - 1] --//--
                // m_mgram_data[M][#M-Grams] --//--
                TProbBackOffEntry * m_mgram_data[N - 1];

                //M-Gram data for 1 < M <= N. This is a 2D array storing
                //For each M-Gram level M an array of #words elements of
                //pointers to C template parameter type:
                // m_mgram_mapping[M][0] -> NULL (if there is no M-gram ending with word 0 in the level)
                // m_mgram_mappin[M]g[1] -> C instance
                // ...
                // m_mgram_mapping[M][#words - 1] -> NULL
                // m_mgram_mapping[M][#words] -> C instance
                //
                //For all 1 < M < N instances of C contain mappings from the
                //context index to the index in the m_mgram_data[M] array -
                //the array storing probability/back-off values.
                //For M = N the pair value stored in the instance of C is the
                //probability itself! Note that that the probabilities are
                //stored as floats - 4 bytes and m_mgram_data[M] array is also a
                //4 byte integer, so we minimize memory usage by storing float
                //probability in place of the index.
                StorageContainer** m_mgram_mapping[N - 1];

                //Will store the next context index counters per M-gram level
                //for 1 < M < N.
                const static TModelLevel NUM_IDX_COUNTERS = N - 2;
                TShortId next_ctx_id[NUM_IDX_COUNTERS];

            };

            template<TModelLevel N, typename WordIndexType> struct TW2CHybridTrie {
                typedef W2CHybridTrie<N, WordIndexType, W2CH_UM_StorageFactory, W2CH_UM_Storage> type;
            };
        }
    }
}

#endif	/* HYBRIDMEMORYTRIE_HPP */

