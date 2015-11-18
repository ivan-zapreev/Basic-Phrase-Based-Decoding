/* 
 * File:   C2DHashMapTrie.hpp
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
 * Created on August 14, 2015, 1:53 PM
 */

#ifndef C2DHASHMAPTRIE_HPP
#define	C2DHASHMAPTRIE_HPP

/**
 * We actually have several choices:
 * 
 * Continue to use <ext/hash_map.h> and use -Wno-deprecated to stop the warning
 * 
 * Use <tr1/unordered_map> and std::tr1::unordered_map
 * 
 * Use <unordered_map> and std::unordered_map and -std=c++0x
 * 
 * We will need to test which one runs better, it is an unordered_map for now.
 * http://www.cplusplus.com/reference/unordered_map/unordered_map/
 */
#include <utility>        // std::pair, std::make_pair
#include <unordered_map>  // std::unordered_map

#include "Globals.hpp"
#include "Logger.hpp"

#include "LayeredTrieBase.hpp"

#include "GreedyMemoryAllocator.hpp"
#include "HashingUtils.hpp"
#include "TextPieceReader.hpp"

using namespace std;
using namespace uva::smt::hashing;
using namespace uva::smt::logging;
using namespace uva::smt::tries::alloc;

namespace uva {
    namespace smt {
        namespace tries {

            /**
             * This is a HashMpa based ATrie interface implementation class.
             * Note 1: This implementation uses the unsigned long for the hashes it is not optimal
             * Note 2: the unordered_map might be not as efficient as a hash_map with respect to memory usage but it is supposed to be faster
             * 
             * This implementation is chosen because it resembles the ordered array implementation from:
             *      "Faster and Smaller N -Gram Language Models"
             *      Adam Pauls Dan Klein
             *      Computer Science Division
             *      University of California, Berkeley
             * 
             * and unordered_maps showed good performance in:
             *      "Efficient in-memory data structures for n-grams indexing"
             *       Daniel Robenek, Jan Platoˇs, and V ́aclav Sn ́aˇsel
             *       Department of Computer Science, FEI, VSB – Technical University of Ostrava
             *       17. listopadu 15, 708 33, Ostrava-Poruba, Czech Republic
             *       {daniel.robenek.st, jan.platos, vaclav.snasel}@vsb.cz
             * 
             */
            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            class C2DMapTrie : public LayeredTrieBase<C2DMapTrie<MAX_LEVEL, WordIndexType>, MAX_LEVEL, WordIndexType, __C2DMapTrie::DO_BITMAP_HASH_CACHE> {
            public:
                typedef LayeredTrieBase<C2DMapTrie<MAX_LEVEL, WordIndexType>, MAX_LEVEL, WordIndexType, __C2DMapTrie::DO_BITMAP_HASH_CACHE> BASE;

                /**
                 * The basic class constructor, accepts memory factors that are the
                 * coefficients used when pre-allocating memory for unordered maps.
                 * 
                 * If a factor is equal to 0.0 then no memory is pre-allocated.
                 * If the factor is equal to 1.0 then there is only as much preallocated
                 * as needed to store the gram entries. The latter is typically not enough
                 * as unordered_map needs more memory for internal administration.
                 * If there is not enough memory pre-allocated then additional allocations
                 * will take place but it does not alway lead to more efficient memory
                 * usage. The observed behavior is that it is better to pre-allocate
                 * a bit more memory beforehand, than needed. This leads to less
                 * memory consumption. Depending on the type of unordered_map
                 * key/value pair types the advised factor values are from 2.0 to 2.6.
                 * Because it can not be optimally determined beforehand, these are made
                 * constructor parameters so that they can be configured by the used.
                 * This breaks encapsulation a bit, exposing the internals, but
                 * there is no other better way, for fine tuning the memory usage.
                 * 
                 * @param word_index the word index to be used
                 * @param mgram_mem_factor The M-Gram memory factor needed for
                 * the greedy allocator for the unordered_map
                 * @param ngram_mem_factor The N-Gram memory factor needed for
                 * the greedy allocator for the unordered_map
                 */
                explicit C2DMapTrie(WordIndexType & word_index,
                        const float mgram_mem_factor = __C2DMapTrie::UM_M_GRAM_MEMORY_FACTOR,
                        const float ngram_mem_factor = __C2DMapTrie::UM_N_GRAM_MEMORY_FACTOR);

                /**
                 * Computes the N-Gram context using the previous context and the current word id
                 * 
                 * WARNING: Must only be called for the M-gram level M > 1!
                 * @see LayeredTrieBase
                 * 
                 * @param word_id the current word id
                 * @param ctx_id [in] - the previous context id, [out] - the next context id
                 * @param level the M-gram level we are working with M, default UNDEF_NGRAM_LEVEL
                 * @return the resulting context
                 * @throw nothing
                 */
                template<TModelLevel CURR_LEVEL>
                inline bool get_ctx_id(const TShortId word_id, TLongId & ctx_id) const {
                    //Use the Szudzik algorithm as it outperforms Cantor
                    ctx_id = szudzik(word_id, ctx_id);
                    //The context can always be computed
                    return true;
                }

                /**
                 * Allows to log the information about the instantiated trie type
                 */
                inline void log_trie_type_usage_info() const {
                    LOG_USAGE << "Using the <" << __FILE__ << "> model." << END_LOG;
                }

                /**
                 * This method can be used to provide the N-gram count information
                 * That should allow for pre-allocation of the memory
                 * For more details @see LayeredTrieBase
                 */
                virtual void pre_allocate(const size_t counts[MAX_LEVEL]);

                /**
                 * Allows to retrieve the data storage structure for the M gram
                 * with the given M-gram level Id. M-gram context and last word Id.
                 * If the storage structure does not exist, return a new one.
                 * For more details @see LayeredTrieBase
                 */
                template<TModelLevel CURR_LEVEL>
                inline void add_m_gram(const T_Model_M_Gram<WordIndexType> & gram) {
                    //Register the m-gram in the hash cache
                    this->template register_m_gram_cache<CURR_LEVEL>(gram);

                    const TShortId word_id = gram.get_end_word_id();
                    if (CURR_LEVEL == M_GRAM_LEVEL_1) {
                        //Store the payload
                        m_1_gram_data[word_id] = gram.m_payload;
                    } else {
                        //Define the context id variable
                        TLongId ctx_id = WordIndexType::UNKNOWN_WORD_ID;
                        //Obtain the m-gram context id
                        __LayeredTrieBase::get_context_id<C2DMapTrie<MAX_LEVEL, WordIndexType>, CURR_LEVEL, DebugLevelsEnum::DEBUG2>(*this, gram, ctx_id);

                        //Obtain this m-gram id
                        (void) get_ctx_id<CURR_LEVEL>(word_id, ctx_id);

                        //Store the payload
                        if (CURR_LEVEL == MAX_LEVEL) {
                            pNGramMap->operator[](ctx_id) = gram.m_payload.m_prob;
                        } else {
                            pMGramMap[CURR_LEVEL - BASE::MGRAM_IDX_OFFSET]->operator[](ctx_id) = gram.m_payload;
                        }
                    }
                }

                /**
                 * Allows to attempt the sub-m-gram payload retrieval for m==1.
                 * The retrieval of a uni-gram data is always a success
                 * @see GenericTrieBase
                 */
                inline void get_unigram_payload(typename BASE::T_Query_Exec_Data & query, MGramStatusEnum & status) const {
                    //Get the word index for convenience
                    const TModelLevel & word_idx = query.m_begin_word_idx;
                    //The data is always present.
                    query.m_payloads[word_idx][word_idx] = &m_1_gram_data[query.m_gram[word_idx]];

                    //The resulting status is always a success
                    status = MGramStatusEnum::GOOD_PRESENT_MGS;
                };

                /**
                 * Allows to retrieve the payload for the M-gram defined by the end word_id and ctx_id.
                 * For more details @see LayeredTrieBase
                 */
                inline void get_m_gram_payload(typename BASE::T_Query_Exec_Data & query, MGramStatusEnum & status) const {
                    /*
                    //Get the next context id
                    if (get_ctx_id<CURR_LEVEL>(word_id, ctx_id)) {
                        //Search for the map for that context id
                        constexpr TModelLevel LEVEL_IDX = (CURR_LEVEL - BASE::MGRAM_IDX_OFFSET);
                        TMGramsMap::const_iterator result = pMGramMap[LEVEL_IDX]->find(ctx_id);
                        if (result == pMGramMap[LEVEL_IDX]->end()) {
                            //There is no data found under this context
                            return GPR_Enum::FAILED_GPR;
                        } else {
                            //There is data found under this context
                            payload = result->second;
                            return GPR_Enum::PAYLOAD_GPR;
                        }
                    } else {
                        //The context id could not be found
                        return GPR_Enum::FAILED_GPR;
                    }*/
                }

                /**
                 * Allows to attempt the sub-m-gram payload retrieval for m==n
                 * @see GenericTrieBase
                 */
                inline void get_n_gram_payload(typename BASE::T_Query_Exec_Data & query, MGramStatusEnum & status) const {
                    /*
                    //Get the next context id
                    if (get_ctx_id<MAX_LEVEL>(word_id, ctx_id)) {
                        //Search for the map for that context id
                        TNGramsMap::const_iterator result = pNGramMap->find(ctx_id);
                        if (result == pNGramMap->end()) {
                            //There is no data found under this context
                            status = MGramStatusEnum::BAD_NO_PAYLOAD_MGS;
                        } else {
                            //There is data found under this context
                            query.m_payloads[query.m_begin_word_idx][query.m_end_word_idx] = &result->second;
                            status = MGramStatusEnum::GOOD_PRESENT_MGS;
                        }
                    } else {
                        //The context id could not be found
                        status = MGramStatusEnum::BAD_NO_PAYLOAD_MGS;
                    }
                     */
                }

                /**
                 * The basic destructor
                 */
                virtual ~C2DMapTrie();

            private:

                //The M-Gram memory factor needed for the greedy allocator for the unordered_map
                const float m_mgram_mem_factor;
                //The N-Gram memory factor needed for the greedy allocator for the unordered_map
                const float m_ngram_mem_factor;

                //Stores the 1-gram data
                T_M_Gram_Payload * m_1_gram_data;

                //The type of key,value pairs to be stored in the M Grams map
                typedef pair< const TLongId, T_M_Gram_Payload> TMGramEntry;
                //The typedef for the M Grams map allocator
                typedef GreedyMemoryAllocator< TMGramEntry > TMGramAllocator;
                //The N Grams map type
                typedef unordered_map<TLongId, T_M_Gram_Payload, std::hash<TLongId>, std::equal_to<TLongId>, TMGramAllocator > TMGramsMap;
                //The actual data storage for the M Grams for 1 < M < N
                TMGramAllocator * pMGramAlloc[MAX_LEVEL - BASE::MGRAM_IDX_OFFSET];
                //The array of maps map storing M-grams for 1 < M < N
                TMGramsMap * pMGramMap[MAX_LEVEL - BASE::MGRAM_IDX_OFFSET];

                //The type of key,value pairs to be stored in the N Grams map
                typedef pair< const TLongId, TLogProbBackOff> TNGramEntry;
                //The typedef for the N Grams map allocator
                typedef GreedyMemoryAllocator< TNGramEntry > TNGramAllocator;
                //The N Grams map type
                typedef unordered_map<TLongId, TLogProbBackOff, std::hash<TLongId>, std::equal_to<TLongId>, TNGramAllocator > TNGramsMap;
                //The actual data storage for the N Grams
                TNGramAllocator * pNGramAlloc;
                //The map storing the N-Grams, they do not have back-off values
                TNGramsMap * pNGramMap;

                //The structure for storing the hash key values statistics
                pair<TLongId, TLongId> hashSizes[MAX_LEVEL];

                /**
                 * This method must used to provide the N-gram count information
                 * That should allow for pre-allocation of the memory
                 * @param counts the counts for the number of elements of each gram level
                 */
                void pre_allocate_1_grams(const size_t counts[MAX_LEVEL]);

                /**
                 * This method must used to provide the N-gram count information
                 * That should allow for pre-allocation of the memory
                 * @param counts the counts for the number of elements of each gram level
                 */
                void pre_allocate_m_grams(const size_t counts[MAX_LEVEL]);

                /**
                 * This method must used to provide the N-gram count information
                 * That should allow for pre-allocation of the memory
                 * @param counts the counts for the number of elements of each gram level
                 */
                void pre_allocate_n_grams(const size_t counts[MAX_LEVEL]);
            };

            typedef C2DMapTrie<M_GRAM_LEVEL_MAX, BasicWordIndex > TC2DMapTrieBasic;
            typedef C2DMapTrie<M_GRAM_LEVEL_MAX, CountingWordIndex > TC2DMapTrieCount;
            typedef C2DMapTrie<M_GRAM_LEVEL_MAX, TOptBasicWordIndex > TC2DMapTrieOptBasic;
            typedef C2DMapTrie<M_GRAM_LEVEL_MAX, TOptCountWordIndex > TC2DMapTrieOptCount;
        }
    }
}
#endif	/* CONTEXTMULTIHASHMAPTRIE_HPP */

