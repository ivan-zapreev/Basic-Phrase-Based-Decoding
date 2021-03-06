/* 
 * File:   H2DMapTrie.cpp
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
 * Created on November 23, 2015, 12:24 AM
 */

#include "server/lm/models/h2d_map_trie.hpp"

#include <inttypes.h>   // std::uint32_t
#include <algorithm>    // std::max

#include "server/lm/lm_consts.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/utils/exceptions.hpp"

#include "server/lm/dictionaries/basic_word_index.hpp"
#include "server/lm/dictionaries/counting_word_index.hpp"
#include "server/lm/dictionaries/optimizing_word_index.hpp"

using namespace uva::smt::bpbd::server::lm::dictionary;
using namespace uva::smt::bpbd::server::lm::__H2DMapTrie;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace lm {

                    template<typename WordIndexType>
                    h2d_map_trie<WordIndexType>::h2d_map_trie(WordIndexType & word_index)
                    : generic_trie_base<h2d_map_trie<WordIndexType>, WordIndexType, __H2DMapTrie::BITMAP_HASH_CACHE_BUCKETS_FACTOR>(word_index),
                    m_n_gram_data(NULL) {
                        //Perform an error check! This container has bounds on the supported trie level
                        ASSERT_CONDITION_THROW((LM_M_GRAM_LEVEL_MAX > M_GRAM_LEVEL_6), string("The maximum supported trie level is") + std::to_string(M_GRAM_LEVEL_6));
                        ASSERT_CONDITION_THROW((word_index.is_word_index_continuous()), "This trie can not be used with a continuous word index!");
                        ASSERT_CONDITION_THROW((sizeof (uint32_t) != sizeof (word_uid)) && (sizeof (uint64_t) != sizeof (word_uid)),
                                string("Only works with a 32 or 64 bit word_uid!"));

                        //Clear the M-Gram bucket arrays
                        memset(m_m_gram_data, 0, NUM_M_GRAM_LEVELS * sizeof (TProbBackMap*));

                        LOG_DEBUG << "sizeof(T_M_Gram_PB_Entry)= " << sizeof (T_M_Gram_PB_Entry) << END_LOG;
                        LOG_DEBUG << "sizeof(T_M_Gram_Prob_Entry)= " << sizeof (T_M_Gram_Prob_Entry) << END_LOG;
                        LOG_DEBUG << "sizeof(TProbBackMap)= " << sizeof (TProbBackMap) << END_LOG;
                        LOG_DEBUG << "sizeof(TProbBackMap)= " << sizeof (TProbMap) << END_LOG;
                    };

                    template<typename WordIndexType>
                    void h2d_map_trie<WordIndexType>::pre_allocate(const size_t counts[LM_M_GRAM_LEVEL_MAX]) {
                        //Call the base-class
                        BASE::pre_allocate(counts);

                        //Initialize the m-gram maps
                        for (phrase_length idx = 0; idx < NUM_M_GRAM_LEVELS; idx++) {
                            m_m_gram_data[idx] = new TProbBackMap(__H2DMapTrie::BUCKETS_FACTOR, counts[idx]);
                        }

                        //Initialize the n-gram's map
                        m_n_gram_data = new TProbMap(__H2DMapTrie::BUCKETS_FACTOR, counts[LM_M_GRAM_LEVEL_MAX - 1]);
                    };

                    template<typename WordIndexType>
                    void h2d_map_trie<WordIndexType>::set_def_unk_word_prob(const prob_weight prob) {
                        //Default initialize the unknown word payload data
                        m_unk_data.m_prob = prob;
                        m_unk_data.m_back = 0.0;
                    }

                    template<typename WordIndexType>
                    h2d_map_trie<WordIndexType>::~h2d_map_trie() {
                        //De-allocate M-Grams
                        for (phrase_length idx = 0; idx < NUM_M_GRAM_LEVELS; idx++) {
                            delete m_m_gram_data[idx];
                        }
                        //De-allocate N-Grams
                        delete m_n_gram_data;
                    };

                    INSTANTIATE_TRIE_TEMPLATE_TYPE(h2d_map_trie, basic_word_index);
                    INSTANTIATE_TRIE_TEMPLATE_TYPE(h2d_map_trie, counting_word_index);
                    INSTANTIATE_TRIE_TEMPLATE_TYPE(h2d_map_trie, hashing_word_index);
                    INSTANTIATE_TRIE_TEMPLATE_TYPE(h2d_map_trie, basic_optimizing_word_index);
                    INSTANTIATE_TRIE_TEMPLATE_TYPE(h2d_map_trie, counting_optimizing_word_index);
                }
            }
        }
    }
}