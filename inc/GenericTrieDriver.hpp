/* 
 * File:   ATrie.hpp
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
 * Created on September 3, 2015, 2:59 PM
 */

#ifndef GENERIC_TRIE_DRIVER_HPP
#define	GENERIC_TRIE_DRIVER_HPP

#include <string>       // std::string
#include <functional>   // std::function

#include "Globals.hpp"
#include "Exceptions.hpp"
#include "Logger.hpp"

#include "MathUtils.hpp"

#include "ModelMGram.hpp"
#include "TextPieceReader.hpp"

#include "BasicWordIndex.hpp"
#include "CountingWordIndex.hpp"
#include "OptimizingWordIndex.hpp"

#include "GenericTrieBase.hpp"

#include "BitmapHashCache.hpp"

#include "C2DHashMapTrie.hpp"
#include "W2CHybridMemoryTrie.hpp"
#include "C2WOrderedArrayTrie.hpp"
#include "W2COrderedArrayTrie.hpp"
#include "C2DMapArrayTrie.hpp"
#include "LayeredTrieDriver.hpp"
#include "G2DHashMapTrie.hpp"

using namespace std;
using namespace uva::smt::logging;
using namespace uva::smt::file;
using namespace uva::smt::tries::caching;
using namespace uva::smt::tries::dictionary;
using namespace uva::smt::tries::m_grams;
using namespace uva::utils::math::bits;

namespace uva {
    namespace smt {
        namespace tries {

            /**
             * This is the common generic trie base class.
             * @param N the maximum level of the considered N-gram, i.e. the N value
             * @param TrieType the type of word index to be used
             */
            template<typename TrieType>
            class GenericTrieDriver : public GenericTrieBase<TrieType::MAX_LEVEL, typename TrieType::WordIndexType> {
            public:
                static constexpr TModelLevel MAX_LEVEL = TrieType::MAX_LEVEL;
                typedef typename TrieType::WordIndexType WordIndexType;
                typedef GenericTrieBase<MAX_LEVEL, WordIndexType> BASE;

                /**
                 * The basic constructor
                 * @param word_index the word index to be used
                 */
                explicit GenericTrieDriver(WordIndexType & word_index)
                : GenericTrieBase<MAX_LEVEL, WordIndexType> (word_index),
                m_trie(word_index) {
                }

                /**
                 * @see GenericTrieBase
                 */
                void pre_allocate(const size_t counts[MAX_LEVEL]) {
                    //Pre-allocate the bitmap-hash caches if needed
                    if (TrieType::needs_bitmap_hash_cache()) {
                        for (size_t idx = 0; idx < BASE::NUM_M_N_GRAM_LEVELS; ++idx) {
                            m_bitmap_hash_cach[idx].pre_allocate(counts[idx + 1]);
                            Logger::updateProgressBar();
                        }
                    }

                    m_trie.pre_allocate(counts);
                };

                /**
                 * @see GenericTrieBase
                 */
                template<TModelLevel CURR_LEVEL>
                inline void add_m_gram(const T_Model_M_Gram<WordIndexType> & gram) {
                    if ((CURR_LEVEL != M_GRAM_LEVEL_1) && TrieType::needs_bitmap_hash_cache()) {
                        //Call the super class first, is needed for caching
                        register_m_gram_cache<CURR_LEVEL>(gram);
                    }

                    m_trie.template add_m_gram<CURR_LEVEL>(gram);
                };

                /**
                 * @see GenericTrieBase
                 */
                inline void log_trie_type_usage_info() {
                    m_trie.log_trie_type_usage_info();
                };

                /**
                 * @see GenericTrieBase
                 */
                template<TModelLevel CURR_LEVEL>
                inline bool is_post_grams() {
                    return m_trie.template is_post_grams<CURR_LEVEL>();
                };

                /**
                 * @see GenericTrieBase
                 */
                template<TModelLevel CURR_LEVEL>
                inline void post_grams() {
                    m_trie.template post_grams<CURR_LEVEL>();
                };

                /**
                 * Allows to check if the given sub-m-gram, defined by the BEGIN_WORD_IDX
                 * and END_WORD_IDX template parameters, is potentially present in the trie.
                 * @param BEGIN_WORD_IDX the begin word index in the given m-gram
                 * @param END_WORD_IDX the end word index in the given m-gram
                 * @param gram the m-gram to work with
                 * @return true if the sub-m-gram is potentially present, otherwise false
                 */
                template<TModelLevel BEGIN_WORD_IDX, TModelLevel END_WORD_IDX>
                inline bool is_m_gram_hash_cached(const T_Query_M_Gram<WordIndexType> & gram) const {
                    //Check if the caching is enabled
                    if (TrieType::needs_bitmap_hash_cache()) {
                        //If the caching is enabled

                        //Compute the model level
                        constexpr TModelLevel CURR_LEVEL = (END_WORD_IDX - BEGIN_WORD_IDX) + 1;

                        //Check on which sub-m-gram level it is
                        if (CURR_LEVEL == M_GRAM_LEVEL_1) {
                            //If this is a unigram then we always check the trie
                            //as retrieving the word probability costs nothing.
                            return true;
                        } else {
                            //The higher sub-m-gram levels always require checking
                            const BitmapHashCache & ref = m_bitmap_hash_cach[CURR_LEVEL - BASE::MGRAM_IDX_OFFSET];
                            return ref.is_m_gram_hash_cached<BEGIN_WORD_IDX, END_WORD_IDX>(gram);
                        }
                    } else {
                        //If caching is not enabled then we always check the trie
                        return true;
                    }
                }

                /**
                 * This method allows to get the probability and/or back off weight for the
                 * sub-m-gram defined by the BEGIN_WORD_IDX and END_WORD_IDX template parameters.
                 * @param BEGIN_WORD_IDX the begin word index in the given m-gram
                 * @param END_WORD_IDX the end word index in the given m-gram
                 * @param DO_BACK_OFF true if the back-off payload is to be retrieved
                 * in case the regular payload is not available.
                 * @param gram the m-gram to work with
                 * @param payload the payload structure to put the values in
                 * @param bo_payload the reference to the back-off data container
                 * @return true if the payload has been found, otherwise false
                 */
                template<TModelLevel BEGIN_WORD_IDX, TModelLevel END_WORD_IDX, bool DO_BACK_OFF>
                inline GPR_Enum get_payload(const T_Query_M_Gram<WordIndexType> & gram, T_M_Gram_Payload & payload, T_M_Gram_Payload & bo_payload) const {
                    //Perform sanity checks if needed
                    if (DO_SANITY_CHECKS && (DO_BACK_OFF && (BEGIN_WORD_IDX == END_WORD_IDX))) {
                        stringstream msg;
                        msg << "Requested back-off option for a unigram! " << (string) gram << " @ position: " << SSTR(BEGIN_WORD_IDX);
                        throw Exception(msg.str());
                    }

                    //Check if we need back-off m-gram payload in case the regular is not found
                    if (DO_BACK_OFF) {
                        LOG_DEBUG << "DO_BACK_OFF == true" << END_LOG;
                        //Compute the back-off end word index
                        constexpr TModelLevel BO_END_WORD_IDX = (BEGIN_WORD_IDX < END_WORD_IDX) ? (END_WORD_IDX - 1) : END_WORD_IDX;
                        //Check if the back-off sub-m-gram has been cached
                        if (is_m_gram_hash_cached < BEGIN_WORD_IDX, BO_END_WORD_IDX > (gram)) {
                            LOG_DEBUG << "is_m_gram_hash_cached< " << SSTR(BEGIN_WORD_IDX) << ", " << SSTR(BO_END_WORD_IDX) << ">(gram) == true" << END_LOG;
                            //Check if the sub-m-gram hash has been cached
                            if (is_m_gram_hash_cached< BEGIN_WORD_IDX, END_WORD_IDX>(gram)) {
                                LOG_DEBUG << "is_m_gram_hash_cached< " << SSTR(BEGIN_WORD_IDX) << ", " << SSTR(END_WORD_IDX) << ">(gram) == true" << END_LOG;
                                //Check the trie for this m-gram's payload
                                return m_trie.template get_payload<BEGIN_WORD_IDX, END_WORD_IDX, true>(gram, payload, bo_payload);
                            } else {
                                LOG_DEBUG << "is_m_gram_hash_cached< " << SSTR(BEGIN_WORD_IDX) << ", " << SSTR(END_WORD_IDX) << ">(gram) == false" << END_LOG;
                                //There is definitely no sub-m-gram present, so try to get the back-off sub-m-gram right away
                                if (m_trie.template get_payload<BEGIN_WORD_IDX, BO_END_WORD_IDX, false>(gram, bo_payload, bo_payload) == GPR_Enum::PAYLOAD_GPR) {
                                    return GPR_Enum::BACK_OFF_GPR;
                                }
                            }
                        }
                        LOG_DEBUG << "is_m_gram_hash_cached< " << SSTR(BEGIN_WORD_IDX) << ", " << SSTR(BO_END_WORD_IDX) << ">(gram) == false" << END_LOG;
                    } else {
                        LOG_DEBUG << "DO_BACK_OFF == false" << END_LOG;
                        //Check if the sub-m-gram hash has been cached
                        if (is_m_gram_hash_cached< BEGIN_WORD_IDX, END_WORD_IDX>(gram)) {
                            LOG_DEBUG << "is_m_gram_hash_cached< " << SSTR(BEGIN_WORD_IDX) << ", " << SSTR(END_WORD_IDX) << ">(gram) == true" << END_LOG;
                            //Check the trie for this m-gram's payload
                            return m_trie.template get_payload<BEGIN_WORD_IDX, END_WORD_IDX, false>(gram, payload, payload);
                        }
                        LOG_DEBUG << "is_m_gram_hash_cached< " << SSTR(BEGIN_WORD_IDX) << ", " << SSTR(END_WORD_IDX) << ">(gram) == false" << END_LOG;
                    }

                    //There is no data available: not cached or not found in the trie.
                    return GPR_Enum::FAILED_GPR;
                };

                /**
                 * Allows to retrieve the probability and back-off weight of the unknown word
                 * @param payload the unknown word payload data
                 */
                inline void get_unk_word_payload(T_M_Gram_Payload & payload) const {
                    m_trie.get_unk_word_payload(payload);
                };

                /**
                 * The basic class destructor
                 */
                virtual ~GenericTrieDriver() {
                };

            protected:
                //Stores the trie

                TrieType m_trie;

                //Stores the bitmap hash caches per M-gram level for 1 < M <= N
                BitmapHashCache m_bitmap_hash_cach[BASE::NUM_M_N_GRAM_LEVELS];

                /**
                 * Is to be used from the sub-classes from the add_X_gram methods.
                 * This method allows to register the given M-gram in internal high
                 * level caches if present.
                 * @param gram the M-gram to cache
                 */
                template<TModelLevel CURR_LEVEL>
                inline void register_m_gram_cache(const T_Model_M_Gram<WordIndexType> &gram) {
                    if (TrieType::needs_bitmap_hash_cache() && (CURR_LEVEL > M_GRAM_LEVEL_1)) {
                        m_bitmap_hash_cach[CURR_LEVEL - BASE::MGRAM_IDX_OFFSET].template add_m_gram<WordIndexType, CURR_LEVEL>(gram);
                    }
                }
            };

#define INSTANTIATE_TYPEDEF_TRIE_DRIVERS_TRIE_NAME_WORD_IDX_TYPE(PREFIX, TRIE_NAME, WORD_IDX_TYPE) \
            template class GenericTrieDriver<PREFIX##TRIE_NAME##WORD_IDX_TYPE>; \
            typedef GenericTrieDriver<PREFIX##TRIE_NAME##WORD_IDX_TYPE> TTrieDriver##TRIE_NAME##WORD_IDX_TYPE;

#define INSTANTIATE_TYPEDEF_TRIE_DRIVERS_PREFIX_NAME(PREFIX, TRIE_NAME) \
            INSTANTIATE_TYPEDEF_TRIE_DRIVERS_TRIE_NAME_WORD_IDX_TYPE(PREFIX, TRIE_NAME, Basic); \
            INSTANTIATE_TYPEDEF_TRIE_DRIVERS_TRIE_NAME_WORD_IDX_TYPE(PREFIX, TRIE_NAME, Count); \
            INSTANTIATE_TYPEDEF_TRIE_DRIVERS_TRIE_NAME_WORD_IDX_TYPE(PREFIX, TRIE_NAME, OptBasic); \
            INSTANTIATE_TYPEDEF_TRIE_DRIVERS_TRIE_NAME_WORD_IDX_TYPE(PREFIX, TRIE_NAME, OptCount);

#define INSTANTIATE_TYPEDEF_LAYERED_TRIE_DRIVERS_NAME(TRIE_NAME) \
INSTANTIATE_TYPEDEF_TRIE_DRIVERS_PREFIX_NAME( TLayeredTrieDriver, TRIE_NAME);         

#define INSTANTIATE_TYPEDEF_GENERIC_TRIE_DRIVERS_NAME(TRIE_NAME) \
INSTANTIATE_TYPEDEF_TRIE_DRIVERS_PREFIX_NAME( T, TRIE_NAME);         

            //Make sure that there will be templates instantiated, at least for the given parameter values
            INSTANTIATE_TYPEDEF_LAYERED_TRIE_DRIVERS_NAME(C2DMapTrie);
            INSTANTIATE_TYPEDEF_LAYERED_TRIE_DRIVERS_NAME(C2DHybridTrie);
            INSTANTIATE_TYPEDEF_LAYERED_TRIE_DRIVERS_NAME(C2WArrayTrie);
            INSTANTIATE_TYPEDEF_LAYERED_TRIE_DRIVERS_NAME(W2CHybridTrie);
            INSTANTIATE_TYPEDEF_LAYERED_TRIE_DRIVERS_NAME(W2CArrayTrie);
            INSTANTIATE_TYPEDEF_GENERIC_TRIE_DRIVERS_NAME(G2DMapTrie);
        }
    }
}

#endif	/* ATRIE_HPP */

